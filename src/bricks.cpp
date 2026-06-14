#include "libbrickbreaker/bricks.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <optional>
#include <vector>

#include "libbrickbreaker/board.hpp"
#include "libbrickbreaker/sounds.hpp"

namespace libbrickbreaker {

namespace {

constexpr std::int32_t kLevelHeaderSize = 1;
constexpr std::int32_t kLevelBytes = Bricks::ROWS * Bricks::COLUMNS;

std::vector<std::uint8_t> g_levelData;

std::int32_t fallbackRandomPercent() {
  static std::uint32_t seed = 0x4f1bbcddu;
  seed = 1103515245u * seed + 12345u;
  return static_cast<std::int32_t>((seed >> 16) % 100u);
}

std::int32_t mapDurability(std::int32_t brickNibble) {
  const std::int32_t nibble = std::max<std::int32_t>(1, std::min<std::int32_t>(14, brickNibble));
  const std::int32_t tier = std::min<std::int32_t>(4, nibble);
  return tier * 2;
}

std::array<std::uint8_t, kLevelBytes> generateLevelBytes(std::int32_t level) {
  std::array<std::uint8_t, kLevelBytes> bytes{};
  const std::int32_t normalizedLevel = ((std::max<std::int32_t>(1, level) - 1) % Bricks::NUM_LEVELS) + 1;

  for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
    for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
      std::int32_t brickNibble = (row * Bricks::COLUMNS + col + normalizedLevel) % 15;
      if (row >= 6 && ((col + normalizedLevel) % 3) == 0) {
        brickNibble = 0;
      }
      if (row == 0 && ((col + normalizedLevel) % 4) == 0) {
        brickNibble = 0x0f;
      }

      const std::int32_t bonusNibble =
          brickNibble > 0 && brickNibble < 0x0f && ((row + col + normalizedLevel) % 5 == 0) ? 1 : 0;
      bytes[static_cast<std::size_t>(row * Bricks::COLUMNS + col)] =
          static_cast<std::uint8_t>((bonusNibble << 4) | (brickNibble & 0x0f));
    }
  }

  return bytes;
}

std::optional<std::array<std::uint8_t, kLevelBytes>> decodeLevel(std::int32_t level) {
  if (g_levelData.size() < static_cast<std::size_t>(kLevelHeaderSize + kLevelBytes)) {
    return std::nullopt;
  }

  const std::int32_t numLevels = std::max<std::int32_t>(1, g_levelData[0]);
  const std::int32_t normalizedLevel = ((std::max<std::int32_t>(1, level) - 1) % numLevels);
  const std::size_t levelOffset = static_cast<std::size_t>(kLevelHeaderSize + normalizedLevel * kLevelBytes);
  const std::size_t end = levelOffset + static_cast<std::size_t>(kLevelBytes);
  if (end > g_levelData.size()) {
    return std::nullopt;
  }

  std::array<std::uint8_t, kLevelBytes> output{};
  std::copy(g_levelData.begin() + static_cast<std::ptrdiff_t>(levelOffset),
            g_levelData.begin() + static_cast<std::ptrdiff_t>(end),
            output.begin());
  return output;
}

}  // namespace

Bricks::Bricks(Board* board) : board_(board) {}

void Bricks::setBoard(Board* board) {
  board_ = board;
}

void Bricks::setLevelData(const std::uint8_t* bytes, std::int32_t length) {
  if (bytes == nullptr || length <= 0) {
    g_levelData.clear();
    return;
  }

  g_levelData.assign(bytes, bytes + length);
}

std::int32_t Bricks::getNumLevels() {
  if (!g_levelData.empty()) {
    return std::max<std::int32_t>(1, g_levelData[0]);
  }

  return NUM_LEVELS;
}

void Bricks::resize() {}

bool Bricks::isDestroyed(std::int32_t x, std::int32_t y) const {
  if (y < 0 || y >= ROWS || x < 0 || x >= COLUMNS) {
    return true;
  }

  const std::int32_t value = cells[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
  return value == EMPTY || value <= 0;
}

void Bricks::destroyBrick(std::int32_t x, std::int32_t y) {
  if (y < 0 || y >= ROWS || x < 0 || x >= COLUMNS) {
    return;
  }

  const std::int32_t value = cells[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
  if (value == EMPTY || value == INDESTRUCTIBLE) {
    return;
  }

  checkForBonus(x, y);
  if (value < 90) {
    numBlocks = std::max<std::int32_t>(0, numBlocks - 1);
    ++lastDestroyedCount_;
  }

  Sounds::instance().play(Sounds::SOUND_BRICKDESTROY);
  cells[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = EMPTY;
  bonuses[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = 0;
}

void Bricks::hitBrick(std::int32_t x, std::int32_t y, std::int32_t damage) {
  if (y < 0 || y >= ROWS || x < 0 || x >= COLUMNS) {
    return;
  }

  const std::int32_t current = cells[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
  if (current == EMPTY || current == INDESTRUCTIBLE) {
    return;
  }

  if (board_ != nullptr && board_->gotBomb) {
    for (std::int32_t yy = y - 1; yy <= y + 1; ++yy) {
      for (std::int32_t xx = x - 1; xx <= x + 1; ++xx) {
        destroyBrick(xx, yy);
      }
    }
    return;
  }

  const std::int32_t next = current - std::max<std::int32_t>(1, damage);
  if (next <= 0) {
    destroyBrick(x, y);
  } else {
    cells[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = next;
    Sounds::instance().play(Sounds::SOUND_BRICKHIT);
  }
}

std::int16_t Bricks::randomSpecialPill() const {
  const std::int32_t roll = board_ != nullptr ? board_->rand(100) : fallbackRandomPercent();
  if (roll < 10) {
    return 1;
  }
  if (roll < 15) {
    return 12;
  }
  if (roll < 30) {
    return 9;
  }
  if (roll < 35) {
    return 11;
  }
  if (roll < 45) {
    return 2;
  }
  if (roll < 60) {
    return 7;
  }
  if (roll < 65) {
    return 4;
  }
  if (roll < 75) {
    return 5;
  }
  if (roll < 80) {
    return 6;
  }
  return 8;
}

void Bricks::checkForBonus(std::int32_t x, std::int32_t y) {
  if (y < 0 || y >= ROWS || x < 0 || x >= COLUMNS) {
    return;
  }

  const std::int32_t bonus = bonuses[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
  if (bonus <= 0) {
    return;
  }

  if (board_ != nullptr) {
    board_->pills.drop(bonus, x, y, *this);
  }
  bonuses[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)] = 0;
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

void Bricks::moveDown() {
  const std::int32_t tileHeight = board_ != nullptr ? board_->tileHeight : std::max<std::int32_t>(1, ROWS + 5);
  const std::int32_t cap = 3 * tileHeight;
  amountMoved = std::min<std::int32_t>(cap, amountMoved + 5);
}

std::int32_t Bricks::movedAmount() const {
  return amountMoved;
}

void Bricks::initialize(std::int32_t level) {
  amountMoved = 0;
  numBlocks = 0;
  lastDestroyedCount_ = 0;

  for (auto& row : cells) {
    row.fill(EMPTY);
  }
  for (auto& row : bonuses) {
    row.fill(0);
  }

  std::array<std::uint8_t, kLevelBytes> levelData = generateLevelBytes(level);
  if (const std::optional<std::array<std::uint8_t, kLevelBytes>> decoded = decodeLevel(level); decoded.has_value()) {
    levelData = *decoded;
  }

  for (std::int32_t row = 0; row < ROWS; ++row) {
    for (std::int32_t col = 0; col < COLUMNS; ++col) {
      const std::uint8_t byteValue = levelData[static_cast<std::size_t>(row * COLUMNS + col)];
      const std::int32_t brickNibble = byteValue & 0x0f;
      const std::int32_t bonusNibble = (byteValue >> 4) & 0x0f;

      std::int32_t mapped = EMPTY;
      if (brickNibble == 0) {
        mapped = EMPTY;
      } else if (brickNibble == 0x0f) {
        mapped = INDESTRUCTIBLE;
      } else {
        mapped = mapDurability(brickNibble);
        ++numBlocks;
      }

      cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = mapped;
      bonuses[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] =
          bonusNibble == 0 ? 0 : randomSpecialPill();
    }
  }
}

std::int32_t Bricks::consumeDestroyedCount() {
  const std::int32_t destroyed = lastDestroyedCount_;
  lastDestroyedCount_ = 0;
  return destroyed;
}

}  // namespace libbrickbreaker
