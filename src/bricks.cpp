#include "libbrickbreaker/bricks.hpp"

namespace libbrickbreaker {

Bricks::Bricks(Board* board) : board_(board) {}

void Bricks::setBoard(Board* board) {
  board_ = board;
}

void Bricks::setLevelData(const std::uint8_t* bytes, std::int32_t length) {
  (void)bytes;
  (void)length;
}

std::int32_t Bricks::getNumLevels() {
  return NUM_LEVELS;
}

void Bricks::resize() {}

bool Bricks::isDestroyed(std::int32_t x, std::int32_t y) const {
  (void)x;
  (void)y;
  return false;
}

void Bricks::destroyBrick(std::int32_t x, std::int32_t y) {
  (void)x;
  (void)y;
}

void Bricks::hitBrick(std::int32_t x, std::int32_t y, std::int32_t damage) {
  (void)x;
  (void)y;
  (void)damage;
}

std::int16_t Bricks::randomSpecialPill() const {
  return 0;
}

void Bricks::checkForBonus(std::int32_t x, std::int32_t y) {
  (void)x;
  (void)y;
}

void Bricks::paint(Graphics* graphics, std::int32_t x, std::int32_t y, std::int32_t flags) const {
  (void)graphics;
  (void)x;
  (void)y;
  (void)flags;
}

void Bricks::render(Graphics* graphics) const {
  (void)graphics;
}

void Bricks::moveDown() {}

std::int32_t Bricks::movedAmount() const {
  return amountMoved;
}

void Bricks::initialize(std::int32_t level) {
  (void)level;
}

std::int32_t Bricks::consumeDestroyedCount() {
  return 0;
}

}  // namespace libbrickbreaker
