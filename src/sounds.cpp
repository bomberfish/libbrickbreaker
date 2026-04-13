#include "libbrickbreaker/sounds.hpp"

#include <algorithm>

namespace libbrickbreaker {

Sounds::Sounds() : sounds_{
                     Sound(SOUND_POPPILL, ""),
                     Sound(SOUND_EATPILL, ""),
                     Sound(SOUND_LASER, ""),
                     Sound(SOUND_BOMB, ""),
                     Sound(SOUND_BRICKDESTROY, ""),
                     Sound(SOUND_BRICKHIT, ""),
                     Sound(SOUND_CEILING, ""),
                     Sound(SOUND_PADDLE, "")
                   } {}

Sounds& Sounds::instance() {
  static Sounds singleton;
  if (!singleton.loaded_) {
    singleton.load();
  }
  return singleton;
}

std::int32_t Sounds::max_started() const {
  return MAX_STARTED;
}

void Sounds::updatePlayerVolumes() {
  for (auto& sound : sounds_) {
    sound.setVolume(volume_);
  }
}

void Sounds::setVolume(std::int32_t level) {
  volume_ = std::clamp(level, 0, 100);
  if (loaded_) {
    updatePlayerVolumes();
  }
}

std::int32_t Sounds::getVolume() const {
  return volume_;
}

void Sounds::setSoundEnabled(bool enabled) {
  soundEnabled_ = enabled;
}

bool Sounds::isSoundEnabled() const {
  return soundEnabled_;
}

void Sounds::play(std::int32_t id) {
  if (!soundEnabled_) {
    return;
  }

  if (!loaded_) {
    if (!load()) {
      return;
    }
  }

  if (id < 0 || id >= kSoundCount) {
    return;
  }

  sounds_[static_cast<std::size_t>(id)].start(MAX_STARTED, volume_);
}

void Sounds::gamePaused(bool paused) {
  if (!loaded_) {
    return;
  }

  for (auto& sound : sounds_) {
    sound.gamePaused(paused);
  }
}

bool Sounds::load() {
  loaded_ = true;
  updatePlayerVolumes();
  return true;
}

}  // namespace libbrickbreaker
