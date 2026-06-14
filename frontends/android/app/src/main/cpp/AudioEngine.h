#ifndef BRICKBREAKER_AUDIO_ENGINE_H
#define BRICKBREAKER_AUDIO_ENGINE_H

#include <array>
#include <cstdint>

#include <SLES/OpenSLES.h>

struct AAssetManager;

class AudioEngine {
 public:
  AudioEngine();
  ~AudioEngine();

  bool loadFromAssets(AAssetManager* assetManager, const std::array<const char*, 8>& relativePaths);
  void setVolume(std::int32_t volumePercent);
  void syncFromCore();

 private:
  struct LoadedSound {
    int fd{-1};
    SLObjectItf playerObject{nullptr};
    SLPlayItf play{nullptr};
    SLVolumeItf volume{nullptr};
    SLSeekItf seek{nullptr};
    bool ready{false};
  };

  std::array<LoadedSound, 8> sounds_{};
  std::array<std::int64_t, 8> lastPlayAtMs_{};
  SLObjectItf engineObject_{nullptr};
  SLEngineItf engineEngine_{nullptr};
  SLObjectItf outputMixObject_{nullptr};
  std::int32_t volumeMillibel_{0};

  bool initializeEngine();
  void teardownEngine();
  void playSound(std::int32_t id, std::int32_t volumePercent);
  static void onPlayRequest(std::int32_t id, std::int32_t volumePercent);
  static std::int32_t percentToMillibel(std::int32_t volumePercent);

  static AudioEngine* activeEngine_;
};

#endif
