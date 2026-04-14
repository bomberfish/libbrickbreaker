#include "libbrickbreaker/ball.hpp"

#include <algorithm>

#include "libbrickbreaker/board.hpp"
#include "libbrickbreaker/bricks.hpp"
#include "libbrickbreaker/paddle.hpp"
#include "libbrickbreaker/sounds.hpp"
#include "util.hpp"

namespace libbrickbreaker {

namespace {

std::int32_t scaleMotion(std::int32_t elapsed,
                         std::int32_t factor,
                         std::int32_t speed,
                         std::int32_t component) {
  const std::int32_t scaled =
      static_cast<std::int32_t>((static_cast<std::int64_t>(factor) * speed * component) >> 16);
  return static_cast<std::int32_t>((static_cast<std::int64_t>(elapsed) * scaled) >> 16);
}

}  // namespace

std::int32_t Ball::speedFactor = 3;
bool Ball::automatedTesting = false;

Ball::Ball(Board* board) : board_(board) {}

void Ball::setBoard(Board* board) {
  board_ = board;
}

void Ball::layoutStatic() {}

void Ball::initialize() {
  const std::int32_t boardWidth = board_ != nullptr ? board_->width : Board::BASE_WIDTH;
  x = (boardWidth / 2) - RADIUS;
  y = maxHeight > 0 ? maxHeight - RADIUS : 0;
  oldx = x;
  oldy = y;
  dx = 0;
  dy = 0;
  aimDx = 1;
  stickyTimeout = 0;
  maxHeight = 0;
  active = false;
  numNonPaddleHits = 0;
  numNonBrickHits = 0;
  tickPos = 0;
}

void Ball::resize(std::int32_t oldWidth, std::int32_t oldHeight) {
  if (board_ == nullptr || oldWidth <= 0 || oldHeight <= 0) {
    return;
  }

  x = (x * board_->width) / oldWidth;
  y = (y * board_->height) / oldHeight;
  oldx = (oldx * board_->width) / oldWidth;
  oldy = (oldy * board_->height) / oldHeight;
  maxHeight = (maxHeight * board_->height) / oldHeight;
}

void Ball::makeSticky(std::int32_t centerX) {
  setSpeed(0, 0);
  x = centerX;
  if (maxHeight > 0) {
    y = maxHeight - RADIUS;
  }
  oldx = x;
  oldy = y;
  aimDx = 1;
}

void Ball::direction(std::int32_t delta) {
  if (delta > 0) {
    ++aimDx;
    if (aimDx == 0) {
      aimDx = 1;
    }
  }
  if (delta < 0) {
    --aimDx;
    if (aimDx == 0) {
      aimDx = -1;
    }
  }

  if (aimDx > 4) {
    aimDx = 4;
  }
  if (aimDx < -4) {
    aimDx = -4;
  }
}

void Ball::setSpeed(std::int32_t dxIn, std::int32_t dyIn) {
  dx = dxIn;
  dy = dyIn;
}

void Ball::calculateCollisions(std::int32_t boardW,
                               std::int32_t boardH,
                               Paddle* paddle,
                               std::int32_t scaledElapsed) {
  if (stopped() || board_ == nullptr) {
    return;
  }

  move(scaledElapsed);

  const std::int32_t rightBound = boardW - RADIUS;
  const std::int32_t leftBound = RADIUS;
  const std::int32_t topBound = RADIUS;

  if (x >= rightBound) {
    Point hitPoint{rightBound, y};
    const bool hit = oldx >= rightBound
                         ? true
                         : Util::intersect(rightBound, 0, rightBound, boardH, oldx, oldy, x, y, &hitPoint);
    if (hit) {
      dx = -dx;
      x = rightBound;
      y = hitPoint.y;
      ++numNonPaddleHits;
      ++numNonBrickHits;
      Sounds::instance().play(Sounds::SOUND_CEILING);
    }
  }

  if (x <= leftBound) {
    Point hitPoint{leftBound, y};
    const bool hit = oldx <= leftBound
                         ? true
                         : Util::intersect(leftBound, 0, leftBound, boardH, oldx, oldy, x, y, &hitPoint);
    if (hit) {
      dx = -dx;
      x = leftBound;
      y = hitPoint.y;
      ++numNonPaddleHits;
      ++numNonBrickHits;
      Sounds::instance().play(Sounds::SOUND_CEILING);
    }
  }

  if (y < topBound) {
    Point hitPoint{x, topBound};
    const bool hit = oldy < topBound
                         ? true
                         : Util::intersect(0, topBound, boardW, topBound, oldx, oldy, x, y, &hitPoint);
    if (hit) {
      dy = -dy;
      y = topBound;
      x = hitPoint.x;
      const std::int32_t totalBounces = board_->increaseBounces(1);
      ++numNonPaddleHits;
      ++numNonBrickHits;
      Sounds::instance().play(Sounds::SOUND_CEILING);
      if (totalBounces > 50) {
        board_->moveDownBricks();
      }
    }
  }

  checkBrickCollisions();

  if (paddle != nullptr) {
    Point hitPoint{};
    if (paddle->intersectLineSegment(oldx, oldy, x, y, &hitPoint)) {
      const Rect paddleRect = paddle->extent();
      x = hitPoint.x;
      y = paddleRect.y - RADIUS;

      const std::int32_t relativeX = x - paddleRect.x;
      const std::int32_t segmentWidth = paddleRect.width >> 4;
      const std::int32_t zone = segmentWidth <= 0 ? 0 : (relativeX / segmentWidth);
      dx = mapPaddleZoneToDx(zone);

      if (paddle->sticky) {
        dy = 0;
        aimDx = dx == 0 ? 1 : dx;
        dx = 0;
      }

      numNonPaddleHits = 0;
      Sounds::instance().play(Sounds::SOUND_PADDLE);
      dy = -dy;

      const std::int32_t totalBounces = board_->increaseBounces(1);
      numNonPaddleHits = 0;

      if (totalBounces > 50) {
        board_->moveDownBricks();
      }
      if (totalBounces > 30) {
        speedFactor = 3 + (totalBounces / 30);
        if (speedFactor > 5) {
          speedFactor = 3;
        }
      }
    }
  }

  if (y >= boardH) {
    if (automatedTesting) {
      dx = board_->rand(4) + 1;
      if (board_->rand(100) >= 50) {
        dx = -dx;
      }
      dy = -dy;
    } else {
      deactivate();
      board_->decreaseBalls();
    }
  }

  if (numNonBrickHits > 10 || numNonPaddleHits > 27) {
    dx += board_->factorX >> 16;
    if (dx == 0) {
      dx = 3;
    }
    numNonPaddleHits = 0;
    numNonBrickHits = 0;
  }
}

bool Ball::stopped() const {
  return dy == 0;
}

void Ball::activate() {
  active = true;
}

void Ball::deactivate() {
  active = false;
  y = -1;
}

bool Ball::isActive() const {
  return active;
}

bool Ball::checkBallTop(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  const std::int32_t moved = board_ == nullptr ? 0 : board_->bricks.amountMoved;
  Point hitPoint{};
  if (!Util::intersect(x0, y0, x1, y1, oldx, oldy - moved, x, y - moved, &hitPoint)) {
    return false;
  }

  x = hitPoint.x;
  y = moved + y0 - RADIUS;
  dy = -dy;
  return true;
}

bool Ball::checkBallLeft(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  const std::int32_t moved = board_ == nullptr ? 0 : board_->bricks.amountMoved;
  Point hitPoint{};
  if (!Util::intersect(x0, y0, x1, y1, oldx, oldy - moved, x, y - moved, &hitPoint)) {
    return false;
  }

  x = x0 - RADIUS;
  y = moved + hitPoint.y;
  dx = -dx;
  return true;
}

bool Ball::checkBallRight(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  const std::int32_t moved = board_ == nullptr ? 0 : board_->bricks.amountMoved;
  Point hitPoint{};
  if (!Util::intersect(x0, y0, x1, y1, oldx, oldy - moved, x, y - moved, &hitPoint)) {
    return false;
  }

  x = x1 + RADIUS;
  y = moved + hitPoint.y;
  dx = -dx;
  return true;
}

bool Ball::checkBallBottom(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1) {
  const std::int32_t moved = board_ == nullptr ? 0 : board_->bricks.amountMoved;
  Point hitPoint{};
  if (!Util::intersect(x0, y0, x1, y1, oldx, oldy - moved, x, y - moved, &hitPoint)) {
    return false;
  }

  x = hitPoint.x;
  y = moved + y0 + RADIUS;
  dy = -dy;
  return true;
}

void Ball::hit(std::int32_t xIn, std::int32_t yIn) {
  if (board_ == nullptr) {
    return;
  }

  board_->increaseBounces(1);
  ++numNonPaddleHits;
  numNonBrickHits = 0;
  board_->bricks.hitBrick(xIn, yIn, 2);
  board_->applyBrickScore();
}

bool Ball::checkBrickCollisions() {
  if (board_ == nullptr) {
    return false;
  }

  Bricks& bricks = board_->bricks;
  const std::int32_t tileWidth = std::max<std::int32_t>(1, board_->tileWidth);
  const std::int32_t tileHeight = std::max<std::int32_t>(1, board_->tileHeight);
  const std::int32_t amountMoved = bricks.amountMoved;

  const std::int32_t col = x / tileWidth;
  const std::int32_t row = (y - amountMoved) / tileHeight;
  if (col < 0 || col >= Bricks::COLUMNS || row < 0 || row >= Bricks::ROWS) {
    return false;
  }
  if (bricks.isDestroyed(col, row)) {
    return false;
  }

  if (x > oldx && y < oldy) {
    return checkBrickUpRight(bricks, col, row, tileWidth, tileHeight);
  }
  if (x > oldx && y > oldy) {
    return checkBrickDownRight(bricks, col, row, tileWidth, tileHeight);
  }
  if (x < oldx && y > oldy) {
    return checkBrickDownLeft(bricks, col, row, tileWidth, tileHeight);
  }
  if (x < oldx && y < oldy) {
    return checkBrickUpLeft(bricks, col, row, tileWidth, tileHeight);
  }

  return false;
}

void Ball::move(std::int32_t scaledElapsed) {
  const std::int32_t factorX = board_ != nullptr ? board_->factorX : (1 << 16);
  const std::int32_t factorY = board_ != nullptr ? board_->factorY : (1 << 16);
  const std::int32_t tileWidth = board_ != nullptr ? board_->tileWidth : 1;
  const std::int32_t tileHeight = board_ != nullptr ? board_->tileHeight : 1;
  const std::int32_t elapsed = std::max<std::int32_t>(0, scaledElapsed);

  oldx = x;
  oldy = y;

  std::int32_t deltaX = scaleMotion(elapsed, factorX, speedFactor, dx);
  std::int32_t deltaY = scaleMotion(elapsed, factorY, speedFactor, dy);

  if (deltaY == 0 && dy != 0) {
    deltaY = dy > 0 ? 1 : -1;
  }
  if (deltaY != 0 && deltaX == 0) {
    deltaX = dx > 0 ? 1 : -1;
  }

  deltaX = std::max<std::int32_t>(-tileWidth, std::min<std::int32_t>(tileWidth, deltaX));
  deltaY = std::max<std::int32_t>(-tileHeight, std::min<std::int32_t>(tileHeight, deltaY));

  x += deltaX;
  y += deltaY;
}

void Ball::render(Graphics* graphics) const {
  (void)graphics;
}

bool Ball::checkBrickUpRight(Bricks& bricks,
                             std::int32_t col,
                             std::int32_t row,
                             std::int32_t tileWidth,
                             std::int32_t tileHeight) {
  const std::int32_t xm1 = col - 1;
  const std::int32_t yp1 = row + 1;
  const std::int32_t xp1 = col + 1;
  const std::int32_t yp2 = row + 2;

  if (col > 0 && isSolidBrick(bricks, xm1, row)) {
    if (checkBallLeft(xm1 * tileWidth, row * tileHeight, xm1 * tileWidth, yp1 * tileHeight)) {
      hit(xm1, row);
      return true;
    }
  }

  if (row < Bricks::ROWS - 1 && isSolidBrick(bricks, col, yp1)) {
    if (checkBallBottom(col * tileWidth, yp2 * tileHeight, xp1 * tileWidth, yp2 * tileHeight)) {
      hit(col, yp1);
      return true;
    }
  }

  if (col > 0 && row < Bricks::ROWS - 1 && isSolidBrick(bricks, xm1, yp1)) {
    if (checkBallLeft(xm1 * tileWidth, yp1 * tileHeight, xm1 * tileWidth, yp2 * tileHeight)) {
      hit(xm1, yp1);
      return true;
    }
    if (checkBallBottom(xm1 * tileWidth, yp2 * tileHeight, col * tileWidth, yp2 * tileHeight)) {
      hit(xm1, yp1);
      return true;
    }
  }

  if (col > 0 && isSolidBrick(bricks, xm1, row)) {
    if (checkBallBottom(xm1 * tileWidth, yp1 * tileHeight, col * tileWidth, yp1 * tileHeight)) {
      hit(xm1, row);
      return true;
    }
  }

  if (row < Bricks::ROWS - 1 && isSolidBrick(bricks, col, yp1)) {
    if (checkBallLeft(col * tileWidth, yp1 * tileHeight, col * tileWidth, yp2 * tileHeight)) {
      hit(col, yp1);
      return true;
    }
  }

  if (checkBallLeft(col * tileWidth, row * tileHeight, col * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }
  if (checkBallBottom(col * tileWidth, yp1 * tileHeight, xp1 * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }

  return true;
}

bool Ball::checkBrickDownRight(Bricks& bricks,
                               std::int32_t col,
                               std::int32_t row,
                               std::int32_t tileWidth,
                               std::int32_t tileHeight) {
  const std::int32_t xm1 = col - 1;
  const std::int32_t ym1 = row - 1;
  const std::int32_t yp1 = row + 1;
  const std::int32_t xp1 = col + 1;

  if (col > 0 && isSolidBrick(bricks, xm1, row)) {
    if (checkBallLeft(xm1 * tileWidth, row * tileHeight, xm1 * tileWidth, yp1 * tileHeight)) {
      hit(xm1, row);
      return true;
    }
  }

  if (row > 0 && isSolidBrick(bricks, col, ym1)) {
    if (checkBallTop(col * tileWidth, ym1 * tileHeight, xp1 * tileWidth, ym1 * tileHeight)) {
      hit(col, ym1);
      return true;
    }
  }

  if (col > 0 && row > 0 && isSolidBrick(bricks, xm1, ym1)) {
    if (checkBallLeft(xm1 * tileWidth, ym1 * tileHeight, xm1 * tileWidth, row * tileHeight)) {
      hit(xm1, ym1);
      return true;
    }
    if (checkBallTop(xm1 * tileWidth, ym1 * tileHeight, col * tileWidth, ym1 * tileHeight)) {
      hit(xm1, ym1);
      return true;
    }
  }

  if (col > 0 && isSolidBrick(bricks, xm1, row)) {
    if (checkBallTop(xm1 * tileWidth, row * tileHeight, col * tileWidth, row * tileHeight)) {
      hit(xm1, row);
      return true;
    }
  }

  if (row > 0 && isSolidBrick(bricks, col, ym1)) {
    if (checkBallLeft(col * tileWidth, ym1 * tileHeight, col * tileWidth, row * tileHeight)) {
      hit(col, ym1);
      return true;
    }
  }

  if (checkBallLeft(col * tileWidth, row * tileHeight, col * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }
  if (checkBallTop(col * tileWidth, row * tileHeight, xp1 * tileWidth, row * tileHeight)) {
    hit(col, row);
    return true;
  }

  return true;
}

bool Ball::checkBrickDownLeft(Bricks& bricks,
                              std::int32_t col,
                              std::int32_t row,
                              std::int32_t tileWidth,
                              std::int32_t tileHeight) {
  const std::int32_t xp1 = col + 1;
  const std::int32_t xp2 = col + 2;
  const std::int32_t ym1 = row - 1;
  const std::int32_t yp1 = row + 1;

  if (col < Bricks::COLUMNS - 1 && isSolidBrick(bricks, xp1, row)) {
    if (checkBallRight(xp2 * tileWidth, row * tileHeight, xp2 * tileWidth, yp1 * tileHeight)) {
      hit(xp1, row);
      return true;
    }
  }

  if (row > 0 && isSolidBrick(bricks, col, ym1)) {
    if (checkBallTop(col * tileWidth, ym1 * tileHeight, xp1 * tileWidth, ym1 * tileHeight)) {
      hit(col, ym1);
      return true;
    }
  }

  if (col < Bricks::COLUMNS - 1 && row > 0 && isSolidBrick(bricks, xp1, ym1)) {
    if (checkBallRight(xp2 * tileWidth, ym1 * tileHeight, xp2 * tileWidth, row * tileHeight)) {
      hit(xp1, ym1);
      return true;
    }
    if (checkBallTop(xp1 * tileWidth, ym1 * tileHeight, xp2 * tileWidth, ym1 * tileHeight)) {
      hit(xp1, ym1);
      return true;
    }
  }

  if (col < Bricks::COLUMNS - 1 && isSolidBrick(bricks, xp1, row)) {
    if (checkBallTop(xp1 * tileWidth, row * tileHeight, xp2 * tileWidth, row * tileHeight)) {
      hit(xp1, row);
      return true;
    }
  }

  if (row > 0 && isSolidBrick(bricks, col, ym1)) {
    if (checkBallRight(xp1 * tileWidth, ym1 * tileHeight, xp1 * tileWidth, row * tileHeight)) {
      hit(col, ym1);
      return true;
    }
  }

  if (checkBallRight(xp1 * tileWidth, row * tileHeight, xp1 * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }
  if (checkBallTop(col * tileWidth, row * tileHeight, xp1 * tileWidth, row * tileHeight)) {
    hit(col, row);
    return true;
  }

  return false;
}

bool Ball::checkBrickUpLeft(Bricks& bricks,
                            std::int32_t col,
                            std::int32_t row,
                            std::int32_t tileWidth,
                            std::int32_t tileHeight) {
  const std::int32_t xp1 = col + 1;
  const std::int32_t xp2 = col + 2;
  const std::int32_t yp1 = row + 1;
  const std::int32_t yp2 = row + 2;

  if (col < Bricks::COLUMNS - 1 && isSolidBrick(bricks, xp1, row)) {
    if (checkBallRight(xp2 * tileWidth, row * tileHeight, xp2 * tileWidth, yp1 * tileHeight)) {
      hit(xp1, row);
      return true;
    }
  }

  if (row < Bricks::ROWS - 1 && isSolidBrick(bricks, col, yp1)) {
    if (checkBallBottom(col * tileWidth, yp2 * tileHeight, xp1 * tileWidth, yp2 * tileHeight)) {
      hit(col, yp1);
      return true;
    }
  }

  if (col < Bricks::COLUMNS - 1 && row < Bricks::ROWS - 1 && isSolidBrick(bricks, xp1, yp1)) {
    if (checkBallRight(xp2 * tileWidth, yp1 * tileHeight, xp2 * tileWidth, yp2 * tileHeight)) {
      hit(xp1, yp1);
      return true;
    }
    if (checkBallBottom(xp1 * tileWidth, yp2 * tileHeight, xp2 * tileWidth, yp2 * tileHeight)) {
      hit(xp1, yp1);
      return true;
    }
  }

  if (col < Bricks::COLUMNS - 1 && isSolidBrick(bricks, xp1, row)) {
    if (checkBallBottom(xp1 * tileWidth, yp1 * tileHeight, xp2 * tileWidth, yp1 * tileHeight)) {
      hit(xp1, row);
      return true;
    }
  }

  if (row < Bricks::ROWS - 1 && isSolidBrick(bricks, col, yp1)) {
    if (checkBallRight(xp1 * tileWidth, yp1 * tileHeight, xp1 * tileWidth, yp2 * tileHeight)) {
      hit(col, yp1);
      return true;
    }
  }

  if (checkBallRight(xp1 * tileWidth, row * tileHeight, xp1 * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }
  if (checkBallBottom(col * tileWidth, yp1 * tileHeight, xp1 * tileWidth, yp1 * tileHeight)) {
    hit(col, row);
    return true;
  }

  return false;
}

bool Ball::isSolidBrick(const Bricks& bricks, std::int32_t xIn, std::int32_t yIn) {
  if (xIn < 0 || xIn >= Bricks::COLUMNS || yIn < 0 || yIn >= Bricks::ROWS) {
    return false;
  }
  return !bricks.isDestroyed(xIn, yIn);
}

std::int32_t Ball::mapPaddleZoneToDx(std::int32_t zone) {
  const std::int32_t z = std::max<std::int32_t>(0, std::min<std::int32_t>(15, zone));
  if (z <= 1) {
    return -4;
  }
  if (z <= 3) {
    return -3;
  }
  if (z <= 5) {
    return -2;
  }
  if (z <= 7) {
    return -1;
  }
  if (z <= 9) {
    return 1;
  }
  if (z <= 11) {
    return 2;
  }
  if (z <= 13) {
    return 3;
  }
  return 4;
}

}  // namespace libbrickbreaker
