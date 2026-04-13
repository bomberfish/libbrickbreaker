#pragma once

#include <array>
#include <cstdint>

#include "pill.hpp"

namespace libbrickbreaker {

class Board;
class Bricks;
class Graphics;
class Paddle;

class Pills {
 public:
  static constexpr std::int32_t MAX_PILLS = 3;

  explicit Pills(Board* board = nullptr);

  void setBoard(Board* board);

  void initialize();
  void resize(std::int32_t oldWidth, std::int32_t oldHeight);
  void move(std::int32_t scaledElapsed);
  void drop(std::int32_t bonusType, std::int32_t x, std::int32_t y, const Bricks& bricks);
  void checkCollisions(Paddle* paddle);
  void render(Graphics* graphics) const;

  std::array<Pill, MAX_PILLS>& pool() { return pool_; }
  const std::array<Pill, MAX_PILLS>& pool() const { return pool_; }

 private:
  std::array<Pill, MAX_PILLS> pool_{};
  Board* board_{nullptr};
};

}  // namespace libbrickbreaker
