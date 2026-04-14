#include "libbrickbreaker/bullet.hpp"

#include <algorithm>

#include "libbrickbreaker/types.hpp"

namespace libbrickbreaker {

void Bullet::initialize() {
  x = 0;
  y = 0;
  width = 2;
  height = 6;
  deactivate();
}

void Bullet::resize(const ImageAsset* bitmap, std::int32_t oldWidth, std::int32_t oldHeight) {
  sprite = bitmap;
  if (oldWidth > 0 && oldHeight > 0 && dy > 0) {
    x = (x * oldWidth) / oldWidth;
    y = (y * oldHeight) / oldHeight;
  }
}

void Bullet::setScaleFactorY(std::int32_t factorY) {
  factorY_ = std::max<std::int32_t>(1, factorY);
}

void Bullet::activate(std::int32_t xIn, std::int32_t yIn, std::int32_t dyIn) {
  x = xIn;
  y = yIn;
  dy = std::max<std::int32_t>(1, dyIn);
}

void Bullet::move(std::int32_t scaledElapsed) {
  if (dy <= 0) {
    return;
  }

  const std::int32_t elapsed = std::max<std::int32_t>(0, scaledElapsed);
  const std::int32_t velocity = static_cast<std::int32_t>((static_cast<std::int64_t>(factorY_) * dy) >> 16);
  const std::int32_t step = static_cast<std::int32_t>((static_cast<std::int64_t>(elapsed) * velocity) >> 16);
  y -= step;
  if (y < 0) {
    deactivate();
  }
}

void Bullet::deactivate() {
  dy = -1;
}

bool Bullet::isActive() const {
  return dy > 0;
}

void Bullet::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
