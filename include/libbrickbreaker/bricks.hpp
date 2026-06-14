#pragma once

#include <array>
#include <cstdint>

namespace libbrickbreaker {

class Board;
class Graphics;

class Bricks {
 public:
  static constexpr std::int32_t ROWS = 10;
  static constexpr std::int32_t COLUMNS = 7;
  static constexpr std::int32_t EMPTY = -4;
  static constexpr std::int32_t INDESTRUCTIBLE = 99;
  static constexpr std::int32_t NUM_LEVELS = 34;

  explicit Bricks(Board* board = nullptr);

  void setBoard(Board* board);

  static void setLevelData(const std::uint8_t* bytes, std::int32_t length);
  static std::int32_t getNumLevels();

  void resize();
  bool isDestroyed(std::int32_t x, std::int32_t y) const;
  void destroyBrick(std::int32_t x, std::int32_t y);
  void hitBrick(std::int32_t x, std::int32_t y, std::int32_t damage);
  std::int16_t randomSpecialPill() const;
  void checkForBonus(std::int32_t x, std::int32_t y);
  void paint(Graphics* graphics, std::int32_t x, std::int32_t y, std::int32_t flags) const;
  void render(Graphics* graphics) const;
  void moveDown();
  std::int32_t movedAmount() const;
  void initialize(std::int32_t level);

  std::int32_t numBlocks{0};
  std::int32_t amountMoved{0};
  std::array<std::array<std::int32_t, COLUMNS>, ROWS> cells{};
  std::array<std::array<std::int32_t, COLUMNS>, ROWS> bonuses{};

 private:
  Board* board_{nullptr};
};

}  // namespace libbrickbreaker
