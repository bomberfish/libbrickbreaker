#include "libbrickbreaker/pill.hpp"

namespace libbrickbreaker {

void Pill::initialize() {}

void Pill::resize(std::int32_t oldWidth,
                  std::int32_t oldHeight,
                  std::int32_t newWidth,
                  std::int32_t newHeight) {
  (void)oldWidth;
  (void)oldHeight;
  (void)newWidth;
  (void)newHeight;
}

void Pill::setMotionContext(std::int32_t boardHeight, std::int32_t factorY) {
  (void)boardHeight;
  (void)factorY;
}

void Pill::activate(std::int32_t xIn, std::int32_t yIn) {
  (void)xIn;
  (void)yIn;
}

void Pill::eat() {}

bool Pill::checkCollision(const Paddle& paddle) const {
  (void)paddle;
  return false;
}

void Pill::move(std::int32_t scaledElapsed) {
  (void)scaledElapsed;
}

void Pill::setBonusType(std::int32_t id) {
  (void)id;
}

bool Pill::isActive() const {
  return false;
}

void Pill::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
