#include "libbrickbreaker/pill.hpp"

#include <algorithm>

#include "libbrickbreaker/paddle.hpp"

namespace libbrickbreaker {

void Pill::initialize() {
  x = 0;
  y = 0;
  dy = 0;
  active = false;
  bonusType = 0;
  label.clear();
  count = 0;
}

void Pill::resize(std::int32_t oldWidth,
                  std::int32_t oldHeight,
                  std::int32_t newWidth,
                  std::int32_t newHeight) {
  if (oldWidth > 0 && oldHeight > 0) {
    x = (x * newWidth) / oldWidth;
    y = (y * newHeight) / oldHeight;
  }
  boardHeight_ = std::max<std::int32_t>(1, newHeight);
}

void Pill::setMotionContext(std::int32_t boardHeight, std::int32_t factorY) {
  boardHeight_ = std::max<std::int32_t>(1, boardHeight);
  factorY_ = std::max<std::int32_t>(1, factorY);
}

void Pill::activate(std::int32_t xIn, std::int32_t yIn) {
  x = xIn;
  y = yIn;
  dy = 6;
  active = true;
  count = 0;
}

void Pill::eat() {
  active = false;
  dy = 0;
}

bool Pill::checkCollision(const Paddle& paddle) const {
  if (!active) {
    return false;
  }

  return paddle.intersectsWithRect(x, y, width, height);
}

void Pill::move(std::int32_t scaledElapsed) {
  if (!active) {
    return;
  }

  const std::int32_t elapsed = std::max<std::int32_t>(0, scaledElapsed);
  const std::int32_t velocity = static_cast<std::int32_t>((static_cast<std::int64_t>(factorY_) * dy) >> 16);
  std::int32_t step = static_cast<std::int32_t>((static_cast<std::int64_t>(elapsed) * velocity) >> 16);
  if (step == 0) {
    step = 1;
  }
  y += step;

  if (y > boardHeight_) {
    eat();
  }
}

void Pill::setBonusType(std::int32_t id) {
  bonusType = id;
  switch (bonusType) {
    case LNG:
      label = "Long";
      break;
    case GUN:
      label = "Gun";
      break;
    case SHR:
      label = "Shrink";
      break;
    case SLW:
      label = "Slow";
      break;
    case NEW:
      label = "Multi";
      break;
    case FLP:
      label = "Flip";
      break;
    case CAT:
      label = "Catch";
      break;
    case LAS:
      label = "Laser";
      break;
    case LIF:
      label = "Life";
      break;
    case WRP:
      label = "Wrap";
      break;
    case BMB:
      label = "Bomb";
      break;
    default:
      label.clear();
      break;
  }
}

bool Pill::isActive() const {
  return active;
}

void Pill::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
