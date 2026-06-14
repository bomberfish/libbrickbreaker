#include "AudioEngine.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

#include <unistd.h>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "libbrickbreaker/sound.hpp"
#include "libbrickbreaker/sounds.hpp"

using namespace libbrickbreaker;

AudioEngine* AudioEngine::activeEngine_ = nullptr;

namespace {

class SoundHooks {
 public:
  static inline bool hooked = false;
};

constexpr std::array<float, 8> kSoundGain = {
    0.78f,
    0.78f,
    0.85f,
    0.92f,
    0.72f,
    0.64f,
    0.55f,
    0.86f,
};

constexpr std::array<std::int32_t, 8> kSoundCooldownMs = {
    20,
    20,
    30,
    60,
    28,
    20,
    24,
    28,
};

std::int64_t nowMs() {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

}  // namespace

AudioEngine::AudioEngine() {
  initializeEngine();
  activeEngine_ = this;
  lastPlayAtMs_.fill(0);

  if (!SoundHooks::hooked) {
    Sound::setPlayCallback(&AudioEngine::onPlayRequest);
    SoundHooks::hooked = true;
  }
}

AudioEngine::~AudioEngine() {
  if (activeEngine_ == this) {
    activeEngine_ = nullptr;
  }
  teardownEngine();
}

bool AudioEngine::initializeEngine() {
  SLresult result = slCreateEngine(&engineObject_, 0, nullptr, 0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    return false;
  }

  result = (*engineObject_)->Realize(engineObject_, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    teardownEngine();
    return false;
  }

  result = (*engineObject_)->GetInterface(engineObject_, SL_IID_ENGINE, &engineEngine_);
  if (result != SL_RESULT_SUCCESS || engineEngine_ == nullptr) {
    teardownEngine();
    return false;
  }

  result = (*engineEngine_)->CreateOutputMix(engineEngine_, &outputMixObject_, 0, nullptr, nullptr);
  if (result != SL_RESULT_SUCCESS) {
    teardownEngine();
    return false;
  }

  result = (*outputMixObject_)->Realize(outputMixObject_, SL_BOOLEAN_FALSE);
  if (result != SL_RESULT_SUCCESS) {
    teardownEngine();
    return false;
  }

  return true;
}

void AudioEngine::teardownEngine() {
  for (LoadedSound& sound : sounds_) {
    if (sound.playerObject != nullptr) {
      (*sound.playerObject)->Destroy(sound.playerObject);
      sound.playerObject = nullptr;
      sound.play = nullptr;
      sound.volume = nullptr;
      sound.seek = nullptr;
      sound.ready = false;
    }
    if (sound.fd >= 0) {
      close(sound.fd);
      sound.fd = -1;
    }
  }

  if (outputMixObject_ != nullptr) {
    (*outputMixObject_)->Destroy(outputMixObject_);
    outputMixObject_ = nullptr;
  }
  if (engineObject_ != nullptr) {
    (*engineObject_)->Destroy(engineObject_);
    engineObject_ = nullptr;
    engineEngine_ = nullptr;
  }
}

bool AudioEngine::loadFromAssets(AAssetManager* assetManager, const std::array<const char*, 8>& relativePaths) {
  if (assetManager == nullptr || engineEngine_ == nullptr || outputMixObject_ == nullptr) {
    return false;
  }

  for (std::size_t i = 0; i < sounds_.size(); ++i) {
    LoadedSound& slot = sounds_[i];
    const char* path = relativePaths[i];
    AAsset* asset = AAssetManager_open(assetManager, path, AASSET_MODE_UNKNOWN);
    if (asset == nullptr) {
      continue;
    }

    off_t start = 0;
    off_t length = 0;
    const int fd = AAsset_openFileDescriptor(asset, &start, &length);
    AAsset_close(asset);
    if (fd < 0) {
      continue;
    }

    SLDataLocator_AndroidFD locator = {SL_DATALOCATOR_ANDROIDFD, fd, static_cast<SLAint64>(start), static_cast<SLAint64>(length)};
    SLDataFormat_MIME format = {SL_DATAFORMAT_MIME, nullptr, SL_CONTAINERTYPE_UNSPECIFIED};
    SLDataSource source = {&locator, &format};

    SLDataLocator_OutputMix outLocator = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject_};
    SLDataSink sink = {&outLocator, nullptr};

    const SLInterfaceID ids[2] = {SL_IID_VOLUME, SL_IID_SEEK};
    const SLboolean req[2] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    SLresult result = (*engineEngine_)
                          ->CreateAudioPlayer(engineEngine_, &slot.playerObject, &source, &sink, 2, ids, req);
    if (result != SL_RESULT_SUCCESS) {
      close(fd);
      continue;
    }

    result = (*slot.playerObject)->Realize(slot.playerObject, SL_BOOLEAN_FALSE);
    if (result != SL_RESULT_SUCCESS) {
      (*slot.playerObject)->Destroy(slot.playerObject);
      slot.playerObject = nullptr;
      close(fd);
      continue;
    }

    (*slot.playerObject)->GetInterface(slot.playerObject, SL_IID_PLAY, &slot.play);
    (*slot.playerObject)->GetInterface(slot.playerObject, SL_IID_VOLUME, &slot.volume);
    (*slot.playerObject)->GetInterface(slot.playerObject, SL_IID_SEEK, &slot.seek);

    slot.fd = fd;
    slot.ready = (slot.play != nullptr);
    if (slot.seek != nullptr) {
      (*slot.seek)->SetLoop(slot.seek, SL_BOOLEAN_FALSE, 0, SL_TIME_UNKNOWN);
    }
  }

  setVolume(80);
  return true;
}

void AudioEngine::setVolume(std::int32_t volumePercent) {
  volumeMillibel_ = percentToMillibel(volumePercent);
  for (LoadedSound& sound : sounds_) {
    if (sound.ready && sound.volume != nullptr) {
      (*sound.volume)->SetVolumeLevel(sound.volume, static_cast<SLmillibel>(volumeMillibel_));
    }
  }
}

void AudioEngine::playSound(std::int32_t id, std::int32_t volumePercent) {
  if (id < 0 || id >= static_cast<std::int32_t>(sounds_.size())) {
    return;
  }

  const std::int64_t currentTime = nowMs();
  const std::int32_t cooldownMs = kSoundCooldownMs[static_cast<std::size_t>(id)];
  if (currentTime - lastPlayAtMs_[static_cast<std::size_t>(id)] < cooldownMs) {
    return;
  }
  lastPlayAtMs_[static_cast<std::size_t>(id)] = currentTime;

  LoadedSound& sound = sounds_[static_cast<std::size_t>(id)];
  if (!sound.ready || sound.play == nullptr) {
    return;
  }

  if (sound.volume != nullptr) {
    const float gain = kSoundGain[static_cast<std::size_t>(id)];
    const std::int32_t adjusted = static_cast<std::int32_t>(std::round(gain * volumePercent));
    (*sound.volume)->SetVolumeLevel(sound.volume, static_cast<SLmillibel>(percentToMillibel(adjusted)));
  }

  (*sound.play)->SetPlayState(sound.play, SL_PLAYSTATE_STOPPED);
  if (sound.seek != nullptr) {
    (*sound.seek)->SetPosition(sound.seek, 0, SL_SEEKMODE_FAST);
  }
  (*sound.play)->SetPlayState(sound.play, SL_PLAYSTATE_PLAYING);
}

void AudioEngine::onPlayRequest(std::int32_t id, std::int32_t volumePercent) {
  if (activeEngine_ != nullptr) {
    activeEngine_->playSound(id, volumePercent);
  }
}

void AudioEngine::syncFromCore() {
  setVolume(Sounds::instance().getVolume());
}

std::int32_t AudioEngine::percentToMillibel(std::int32_t volumePercent) {
  const std::int32_t clamped = std::max<std::int32_t>(0, std::min<std::int32_t>(100, volumePercent));
  if (clamped <= 0) {
    return -3000;
  }
  if (clamped >= 100) {
    return 0;
  }
  return static_cast<std::int32_t>(-3000 + clamped * 30);
}
