#include "libbrickbreaker/bullet.hpp"

namespace libbrickbreaker {

void Bullet::initialize() {}

void Bullet::resize(const ImageAsset* bitmap, std::int32_t oldWidth, std::int32_t oldHeight) {
  (void)bitmap;
  (void)oldWidth;
  (void)oldHeight;
}

void Bullet::setScaleFactorY(std::int32_t factorY) {
  (void)factorY;
}

void Bullet::activate(std::int32_t xIn, std::int32_t yIn, std::int32_t dyIn) {
  (void)xIn;
  (void)yIn;
  (void)dyIn;
}

void Bullet::move(std::int32_t scaledElapsed) {
  (void)scaledElapsed;
}

void Bullet::deactivate() {}

bool Bullet::isActive() const {
  return false;
}

void Bullet::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
