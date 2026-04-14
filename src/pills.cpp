#include "libbrickbreaker/pills.hpp"

#include "libbrickbreaker/board.hpp"
#include "libbrickbreaker/bricks.hpp"
#include "libbrickbreaker/paddle.hpp"
#include "libbrickbreaker/sounds.hpp"

namespace libbrickbreaker {

Pills::Pills(Board* board) : board_(board) {}

void Pills::setBoard(Board* board) {
  board_ = board;
}

void Pills::initialize() {
  for (Pill& pill : pool_) {
    pill.initialize();
  }
}

void Pills::resize(std::int32_t oldWidth, std::int32_t oldHeight) {
  const std::int32_t newWidth = board_ != nullptr ? board_->width : oldWidth;
  const std::int32_t newHeight = board_ != nullptr ? board_->height : oldHeight;
  const std::int32_t factorY = board_ != nullptr ? board_->factorY : (1 << 16);

  for (Pill& pill : pool_) {
    pill.resize(oldWidth, oldHeight, newWidth, newHeight);
    pill.setMotionContext(newHeight, factorY);
  }
}

void Pills::move(std::int32_t scaledElapsed) {
  for (Pill& pill : pool_) {
    pill.move(scaledElapsed);
  }
}

void Pills::drop(std::int32_t bonusType, std::int32_t x, std::int32_t y, const Bricks& bricks) {
  const std::int32_t tileWidth = board_ != nullptr ? board_->tileWidth : 1;
  const std::int32_t tileHeight = board_ != nullptr ? board_->tileHeight : 1;
  const std::int32_t amountMoved = bricks.amountMoved;
  const std::int32_t factorY = board_ != nullptr ? board_->factorY : (1 << 16);
  const std::int32_t boardHeight = board_ != nullptr ? board_->height : 195;

  for (Pill& pill : pool_) {
    if (!pill.isActive()) {
      pill.setBonusType(bonusType);
      pill.setMotionContext(boardHeight, factorY);
      pill.activate(x * tileWidth + 4, amountMoved + y * tileHeight);
      pill.dy = 6;
      Sounds::instance().play(Sounds::SOUND_POPPILL);
      return;
    }
  }
}

void Pills::checkCollisions(Paddle* paddle) {
  if (paddle == nullptr) {
    return;
  }

  for (Pill& pill : pool_) {
    if (pill.isActive() && pill.checkCollision(*paddle)) {
      const std::int32_t bonusType = pill.bonusType;
      if (board_ != nullptr) {
        board_->powerUp(bonusType);
      }
      pill.eat();
    }
  }
}

void Pills::render(Graphics* graphics) const {
  (void)graphics;
}

}  // namespace libbrickbreaker
