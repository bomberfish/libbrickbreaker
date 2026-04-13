#pragma once

#include <cstdint>
#include <string>

namespace libbrickbreaker {

class Sound {
 public:
  Sound() = default;
  Sound(std::int32_t id, std::string source);

  static std::int32_t numStartedCount();

  void setVolume(std::int32_t level);
  void start(std::int32_t maxStarted, std::int32_t volume);
  void gamePaused(bool paused);

  std::int32_t id() const { return id_; }
  const std::string& source() const { return source_; }

 private:
  static std::int32_t numStarted_;

  std::int32_t id_{0};
  std::string source_{};
  std::int32_t started_{0};
  bool pausedByGame_{false};
  bool playing_{false};
  std::int32_t volume_{100};
};

}  // namespace libbrickbreaker
