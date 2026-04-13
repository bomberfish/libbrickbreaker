#include "libbrickbreaker/paddle.hpp"

namespace libbrickbreaker {

void Paddle::setBoardSize(std::int32_t widthIn, std::int32_t heightIn) {
  boardWidth_ = widthIn;
  boardHeight_ = heightIn;
}

void Paddle::initialize() {}

void Paddle::resize() {}

void Paddle::setLocation(std::int32_t xIn, std::int32_t yIn) {
  x = xIn;
  y = yIn;
}

void Paddle::setSize(std::int32_t widthIn, std::int32_t heightIn) {
  width = widthIn;
  height = heightIn;
}

void Paddle::updateReticle() {}

Rect Paddle::extent() const {
  return Rect{x, y, width, height};
}

std::int32_t Paddle::centerX() const {
  return x + (width / 2);
}

bool Paddle::intersectsTouch(std::int32_t xIn) const {
  (void)xIn;
  return false;
}

bool Paddle::intersectLineSegment(std::int32_t x0,
                                  std::int32_t y0,
                                  std::int32_t x1,
                                  std::int32_t y1,
                                  Point* intersection) const {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  (void)intersection;
  return false;
}

bool Paddle::intersectsWithRect(std::int32_t xIn,
                                std::int32_t yIn,
                                std::int32_t widthIn,
                                std::int32_t heightIn) const {
  (void)xIn;
  (void)yIn;
  (void)widthIn;
  (void)heightIn;
  return false;
}

bool Paddle::isWrapped() const {
  return wrapped;
}

std::int32_t Paddle::currentInputMode() const {
  return inputMode;
}

void Paddle::setInputMode(std::int32_t modeIn) {
  inputMode = modeIn;
}

void Paddle::setMode(std::int32_t modeIn) {
  mode = modeIn;
}

void Paddle::moveTo(std::int32_t targetX, const Board& board) {
  (void)targetX;
  (void)board;
}

void Paddle::move(std::int32_t delta, const Board& board) {
  (void)delta;
  (void)board;
}

void Paddle::render(Graphics* graphics) const {
  (void)graphics;
}

void Paddle::applyHorizontalBounds() {}

std::vector<Rect> Paddle::getCollisionRects() const {
  return {};
}

bool Paddle::rectsIntersect(const Rect& a, const Rect& b) {
  (void)a;
  (void)b;
  return false;
}

bool Paddle::segmentIntersectsRect(std::int32_t x0,
                                   std::int32_t y0,
                                   std::int32_t x1,
                                   std::int32_t y1,
                                   const Rect& rect,
                                   Point* hitPoint) {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  (void)rect;
  (void)hitPoint;
  return false;
}

}  // namespace libbrickbreaker
