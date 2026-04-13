#include "libbrickbreaker/ball.hpp"

namespace libbrickbreaker {

std::int32_t Ball::speedFactor = 3;
bool Ball::automatedTesting = false;

Ball::Ball(Board* board) : board_(board) {}

void Ball::setBoard(Board* board) {
  board_ = board;
}

void Ball::layoutStatic() {}

void Ball::initialize() {}

void Ball::resize(std::int32_t oldWidth, std::int32_t oldHeight) {
  (void)oldWidth;
  (void)oldHeight;
}

void Ball::makeSticky(std::int32_t centerX) {
  (void)centerX;
}

void Ball::direction(std::int32_t delta) {
  (void)delta;
}

void Ball::setSpeed(std::int32_t dxIn, std::int32_t dyIn) {
  (void)dxIn;
  (void)dyIn;
}

void Ball::calculateCollisions(std::int32_t boardW,
                               std::int32_t boardH,
                               Paddle* paddle,
                               std::int32_t scaledElapsed) {
  (void)boardW;
  (void)boardH;
  (void)paddle;
  (void)scaledElapsed;
}

bool Ball::stopped() const {
  return false;
}

void Ball::activate() {}

void Ball::deactivate() {}

bool Ball::isActive() const {
  return false;
}

bool Ball::checkBallTop(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  return false;
}

bool Ball::checkBallLeft(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  return false;
}

bool Ball::checkBallRight(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  return false;
}

bool Ball::checkBallBottom(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  (void)x0;
  (void)y0;
  (void)x1;
  (void)y1;
  return false;
}

void Ball::hit(std::int32_t xIn, std::int32_t yIn) {
  (void)xIn;
  (void)yIn;
}

bool Ball::checkBrickCollisions() {
  return false;
}

void Ball::move(std::int32_t scaledElapsed) {
  (void)scaledElapsed;
}

void Ball::render(Graphics* graphics) const {
  (void)graphics;
}

bool Ball::checkBrickUpRight(Bricks& bricks,
                             std::int32_t col,
                             std::int32_t row,
                             std::int32_t tileWidth,
                             std::int32_t tileHeight) {
  (void)bricks;
  (void)col;
  (void)row;
  (void)tileWidth;
  (void)tileHeight;
  return false;
}

bool Ball::checkBrickDownRight(Bricks& bricks,
                               std::int32_t col,
                               std::int32_t row,
                               std::int32_t tileWidth,
                               std::int32_t tileHeight) {
  (void)bricks;
  (void)col;
  (void)row;
  (void)tileWidth;
  (void)tileHeight;
  return false;
}

bool Ball::checkBrickDownLeft(Bricks& bricks,
                              std::int32_t col,
                              std::int32_t row,
                              std::int32_t tileWidth,
                              std::int32_t tileHeight) {
  (void)bricks;
  (void)col;
  (void)row;
  (void)tileWidth;
  (void)tileHeight;
  return false;
}

bool Ball::checkBrickUpLeft(Bricks& bricks,
                            std::int32_t col,
                            std::int32_t row,
                            std::int32_t tileWidth,
                            std::int32_t tileHeight) {
  (void)bricks;
  (void)col;
  (void)row;
  (void)tileWidth;
  (void)tileHeight;
  return false;
}

bool Ball::isSolidBrick(const Bricks& bricks, std::int32_t xIn, std::int32_t yIn) {
  (void)bricks;
  (void)xIn;
  (void)yIn;
  return false;
}

std::int32_t Ball::mapPaddleZoneToDx(std::int32_t zone) {
  (void)zone;
  return 0;
}

}  // namespace libbrickbreaker
