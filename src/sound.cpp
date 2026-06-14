#include "libbrickbreaker/sound.hpp"

#include <algorithm>

namespace libbrickbreaker {

std::int32_t Sound::numStarted_ = 0;
Sound::PlayCallback Sound::playCallback_ = nullptr;

Sound::Sound(std::int32_t id, std::string source) : id_(id), source_(std::move(source)) {}

std::int32_t Sound::numStartedCount() {
  return numStarted_;
}

void Sound::setPlayCallback(PlayCallback callback) {
  playCallback_ = callback;
}

void Sound::setVolume(std::int32_t level) {
  volume_ = std::clamp(level, 0, 100);
}

void Sound::start(std::int32_t maxStarted, std::int32_t volume) {
  if (maxStarted < 1) {
    return;
  }
  if (numStarted_ >= maxStarted) {
    return;
  }
  if (started_ > 0 || playing_) {
    return;
  }

  setVolume(volume);
  ++numStarted_;
  ++started_;
  playing_ = true;

  if (playCallback_ != nullptr) {
    playCallback_(id_, volume_);
  }

  playing_ = false;
  started_ = 0;
  if (numStarted_ > 0) {
    --numStarted_;
  }
}

void Sound::gamePaused(bool paused) {
  pausedByGame_ = paused;
  if (pausedByGame_) {
    playing_ = false;
  }
}

}  // namespace libbrickbreaker
