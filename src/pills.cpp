#include "libbrickbreaker/pills.hpp"

namespace libbrickbreaker {

Pills::Pills(Board* board) : board_(board) {}

void Pills::setBoard(Board* board) {
  board_ = board;
}

void Pills::initialize() {}

void Pills::resize(std::int32_t oldWidth, std::int32_t oldHeight) {
  (void)oldWidth;
  (void)oldHeight;
}

void Pills::move(std::int32_t scaledElapsed) {
  (void)scaledElapsed;
}

void Pills::drop(std::int32_t bonusType, std::int32_t x, std::int32_t y, const Bricks& bricks) {
  (void)bonusType;
  (void)x;
  (void)y;
  (void)bricks;
}

void Pills::checkCollisions(Paddle* paddle) {
  (void)paddle;
}

void Pills::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
