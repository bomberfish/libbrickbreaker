#pragma once

#include <array>
#include <cstdint>

#include "sound.hpp"

namespace libbrickbreaker {

class Sounds {
 public:
  static constexpr std::int32_t SOUND_POPPILL = 0;
  static constexpr std::int32_t SOUND_EATPILL = 1;
  static constexpr std::int32_t SOUND_LASER = 2;
  static constexpr std::int32_t SOUND_BOMB = 3;
  static constexpr std::int32_t SOUND_BRICKDESTROY = 4;
  static constexpr std::int32_t SOUND_BRICKHIT = 5;
  static constexpr std::int32_t SOUND_CEILING = 6;
  static constexpr std::int32_t SOUND_PADDLE = 7;

  static constexpr std::int32_t MAX_STARTED = 1;
  static constexpr std::int32_t kSoundCount = 8;

  static Sounds& instance();

  std::int32_t max_started() const;
  void updatePlayerVolumes();
  void setVolume(std::int32_t level);
  std::int32_t getVolume() const;
  void setSoundEnabled(bool enabled);
  bool isSoundEnabled() const;
  void play(std::int32_t id);
  void gamePaused(bool paused);

 private:
  Sounds();

  bool load();

  bool loaded_{false};
  bool soundEnabled_{true};
  std::int32_t volume_{80};
  std::array<Sound, kSoundCount> sounds_{};
};

}  // namespace libbrickbreaker
