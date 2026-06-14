#include "libbrickbreaker/board.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

#include "libbrickbreaker/game.hpp"
#include "libbrickbreaker/sounds.hpp"
#include "libbrickbreaker/sprites.hpp"

namespace libbrickbreaker {

std::int32_t Board::WIDTH = BASE_WIDTH;
std::int32_t Board::HEIGHT = BASE_HEIGHT;
std::int32_t Board::TILEWIDTH = BASE_WIDTH / 7;
std::int32_t Board::TILEHEIGHT = BASE_HEIGHT / 15;
std::int32_t Board::FACTORX = 1 << 16;
std::int32_t Board::FACTORY = 1 << 16;

Board::Board() : Board(nullptr) {}

Board::Board(Game* gameIn)
    : game(gameIn),
      paddle(),
      bricks(this),
      pills(this),
      bomb(),
      balls{Ball(this), Ball(this), Ball(this), Ball(this)},
      lasers{Bullet(), Bullet(), Bullet(), Bullet()} {
  syncMetrics();
  paddle.setBoardSize(width, height);

  bomb.initialize();
  bomb.setScaleFactorY(factorY);

  for (Bullet& laser : lasers) {
    laser.initialize();
    laser.setScaleFactorY(factorY);
  }
}

void Board::setGame(Game* gameIn) {
  game = gameIn;
}

void Board::resize(std::int32_t xIn, std::int32_t yIn, std::int32_t widthIn, std::int32_t heightIn) {
  const std::int32_t oldWidth = width;
  const std::int32_t oldHeight = height;

  x = xIn;
  y = yIn;
  width = std::max<std::int32_t>(1, widthIn);
  height = std::max<std::int32_t>(1, heightIn);
  syncMetrics();

  paddle.setBoardSize(width, height);
  paddle.resize();

  for (Ball& ball : balls) {
    ball.resize(oldWidth, oldHeight);
  }

  pills.resize(oldWidth, oldHeight);

  bomb.resize(Sprites::getSprite(SpriteKey::kBomb), oldWidth, oldHeight);
  for (Bullet& laser : lasers) {
    laser.resize(Sprites::getSprite(SpriteKey::kLaser), oldWidth, oldHeight);
  }

  bomb.setScaleFactorY(factorY);
  for (Bullet& laser : lasers) {
    laser.setScaleFactorY(factorY);
  }
}

void Board::initialize(std::int32_t level, bool reloadBricks) {
  lifeDecrementPending_ = false;

  bomb.resize(Sprites::getSprite(SpriteKey::kBomb), width, height);
  for (Bullet& laser : lasers) {
    laser.resize(Sprites::getSprite(SpriteKey::kLaser), width, height);
  }

  for (Ball& ball : balls) {
    ball.initialize();
  }

  bomb.initialize();
  for (Bullet& laser : lasers) {
    laser.initialize();
  }

  pills.initialize();

  balls[0].activate();
  numActiveBalls = 1;

  numBounces = 50 * (game != nullptr ? game->superLevelCount() : 0);
  stickyPaddleCount = 0;

  paddle.setBoardSize(width, height);
  paddle.initialize();
  paddle.setMode(Paddle::MODE_DEFAULT);

  for (Ball& ball : balls) {
    ball.maxHeight = paddle.extent().y;
  }

  balls[0].makeSticky(paddle.centerX());

  if (game != nullptr) {
    game->setAmmo(0);
  }
  gotLaser = false;
  gotBomb = false;
  paddle.warp = false;

  if (reloadBricks) {
    bricks.initialize(level);
  }

  ghosting = true;
  ghostingCount = 0;
  flashCount = 0;
  flashingString = "";
  Ball::speedFactor = 3;
}

void Board::initialize(std::int32_t level) {
  initialize(level, true);
}

std::int32_t Board::stickyCountPercentage() const {
  return std::max<std::int32_t>(0, std::min<std::int32_t>(100, (stickyPaddleCount * 100) / 150));
}

std::int32_t Board::rand() {
  randomSeed_ = (1103515245u * randomSeed_ + 12345u) & 0xffffffffu;
  return static_cast<std::int32_t>(randomSeed_ & 0x7fffffffu);
}

std::int32_t Board::rand(std::int32_t max) {
  const std::int32_t clamped = std::max<std::int32_t>(1, max);
  return rand() % clamped;
}

Paddle* Board::paddleRef() {
  return &paddle;
}

const Paddle* Board::paddleRef() const {
  return &paddle;
}

void Board::update(std::int32_t elapsedMs) {
  const std::int32_t scaledElapsed = std::max<std::int32_t>(0, elapsedMs) * 595;
  updateBalls(scaledElapsed);
  pills.move(scaledElapsed);
  bomb.move(scaledElapsed);
  for (Bullet& laser : lasers) {
    laser.move(scaledElapsed);
  }
  pills.checkCollisions(&paddle);
}

void Board::updateBalls(std::int32_t scaledElapsed) {
  for (std::int32_t i = MAX_BALLS - 1; i >= 0; --i) {
    Ball& ball = balls[static_cast<std::size_t>(i)];
    if (ball.isActive()) {
      ball.calculateCollisions(width, height, &paddle, scaledElapsed);
    }
  }

  if (numActiveBalls <= 0) {
    if (game != nullptr && !lifeDecrementPending_) {
      game->state = Game::STATE_DEATH;
      game->decreaseLives();
      lifeDecrementPending_ = true;
    }
    return;
  }

  if (balls[0].stopped()) {
    ++stickyPaddleCount;
    if (stickyPaddleCount > 150) {
      releaseStickyBall();
    }
  } else {
    stickyPaddleCount = 0;
  }

  if (bricks.numBlocks < 1 && game != nullptr) {
    game->state = Game::STATE_FINISHEDLEVEL;
  }
}

std::int32_t Board::increaseBounces(std::int32_t amount) {
  numBounces += std::max<std::int32_t>(0, amount);
  return numBounces;
}

void Board::decreaseBalls() {
  if (numActiveBalls > 0) {
    --numActiveBalls;
  }
}

void Board::setBGColor(std::int32_t color) {
  backgroundColor = color;
}

void Board::setFGColor(std::int32_t color) {
  foregroundColor = color;
}

void Board::setPointerColor(std::int32_t color) {
  pointerColor = color;
}

void Board::setColors(std::int32_t backgroundColorIn,
                      std::int32_t foregroundColorIn,
                      std::int32_t pointerColorIn) {
  backgroundColor = backgroundColorIn;
  foregroundColor = foregroundColorIn;
  pointerColor = pointerColorIn;
}

std::int32_t Board::widthPx() const {
  return width;
}

std::int32_t Board::xPos() const {
  return x;
}

std::int32_t Board::yPos() const {
  return y;
}

std::int32_t Board::foregroundColorValue() const {
  return foregroundColor;
}

std::int32_t Board::pointerColorValue() const {
  return pointerColor;
}

void Board::gamePaused(bool paused) {
  Sounds::instance().gamePaused(paused);
}

void Board::render(Graphics* graphics) {
  bricks.render(graphics);
  pills.render(graphics);
  renderProjectiles(graphics);
  for (std::int32_t i = MAX_BALLS - 1; i >= 0; --i) {
    if (balls[static_cast<std::size_t>(i)].isActive()) {
      balls[static_cast<std::size_t>(i)].render(graphics);
    }
  }
  paddle.render(graphics);

  if (flashCount > 0) {
    --flashCount;
  }
  if (ghosting) {
    ++ghostingCount;
    if (ghostingCount > 20) {
      ghosting = false;
    }
  }
}

void Board::renderProjectiles(Graphics* graphics) {
  bomb.render(graphics);
  for (Bullet& laser : lasers) {
    laser.render(graphics);
  }

  processProjectileCollision(bomb, true);
  for (Bullet& laser : lasers) {
    processProjectileCollision(laser, false);
  }
}

void Board::powerUp(std::int32_t id) {
  flashCount = 4;
  flashingString = getPowerUpMessage(id);

  switch (id) {
    case Pill::LNG:
      // Note: the original LONG case does NOT clear gotBomb (unlike GUN/CATCH/LASER/BOMB).
      if (game != nullptr) {
        game->setAmmo(0);
      }
      gotLaser = false;
      paddle.sticky = false;
      paddle.flipped = false;
      paddle.setMode(Paddle::MODE_LONG);
      break;
    case Pill::GUN:
      gotBomb = false;
      gotLaser = false;
      if (game != nullptr) {
        game->setAmmo(3);
      }
      paddle.sticky = false;
      paddle.flipped = false;
      paddle.setMode(Paddle::MODE_GUN);
      break;
    case Pill::SLW:
      slowDown();
      paddle.flipped = false;
      break;
    case Pill::NEW:
      addBalls();
      paddle.sticky = false;
      paddle.flipped = false;
      break;
    case Pill::FLP:
      paddle.flipped = true;
      break;
    case Pill::CAT:
      gotBomb = false;
      gotLaser = false;
      if (game != nullptr) {
        game->setAmmo(0);
      }
      paddle.sticky = true;
      paddle.flipped = false;
      killBallsExceptOne();
      paddle.setMode(Paddle::MODE_DEFAULT);
      paddle.warp = false;
      break;
    case Pill::LAS:
      gotBomb = false;
      gotLaser = true;
      if (game != nullptr) {
        game->setAmmo(0);
      }
      paddle.sticky = false;
      paddle.flipped = false;
      paddle.setMode(Paddle::MODE_LASER);
      break;
    case Pill::LIF:
      if (game != nullptr) {
        game->increaseLives();
      }
      gotLaser = false;
      if (game != nullptr) {
        game->setAmmo(0);
      }
      paddle.sticky = false;
      paddle.flipped = false;
      paddle.setMode(Paddle::MODE_DEFAULT);
      break;
    case Pill::WRP:
      paddle.warp = true;
      break;
    case Pill::BMB:
      gotBomb = true;
      gotLaser = false;
      if (game != nullptr) {
        game->setAmmo(0);
      }
      paddle.sticky = false;
      paddle.flipped = false;
      paddle.setMode(Paddle::MODE_DEFAULT);
      killBallsExceptOne();
      break;
    default:
      break;
  }

  if (game != nullptr) {
    game->increasePoints(50);
  }
}

void Board::increasePoints(std::int32_t points) {
  if (game != nullptr) {
    game->increasePoints(points);
  }
}

void Board::addBalls() {
  // Mirrors the original addBalls: pick the first ACTIVE ball as the position source
  // (by isActive, not an x sentinel), copy its position into all four balls, assign the
  // fixed launch vectors, and activate every ball.
  numActiveBalls = MAX_BALLS;

  Ball* source = nullptr;
  for (Ball& ball : balls) {
    if (ball.isActive()) {
      source = &ball;
      break;
    }
  }

  if (source != nullptr) {
    const std::int32_t srcX = source->x;
    const std::int32_t srcY = source->y;
    for (Ball& ball : balls) {
      ball.x = srcX;
      ball.oldx = srcX;
      ball.y = srcY;
      ball.oldy = srcY;
    }
  }

  static constexpr std::int32_t kVectors[MAX_BALLS] = {-3, -1, 1, 3};
  for (std::int32_t i = 0; i < MAX_BALLS; ++i) {
    Ball& ball = balls[static_cast<std::size_t>(i)];
    ball.setSpeed(kVectors[static_cast<std::size_t>(i)], -3);
    ball.activate();
  }
}

void Board::killBallsExceptOne() {
  // Mirrors the original killBallsExceptOne: keep the active ball with the smallest y
  // (highest on screen), swap it into slot 0, deactivate the rest, and normalise the
  // active count to 1.
  std::int32_t keep = -1;
  for (std::int32_t i = 0; i < MAX_BALLS; ++i) {
    const Ball& ball = balls[static_cast<std::size_t>(i)];
    if (!ball.isActive()) {
      continue;
    }
    if (keep == -1 || ball.y < balls[static_cast<std::size_t>(keep)].y) {
      keep = i;
    }
  }

  if (keep != -1 && keep != 0) {
    std::swap(balls[0], balls[static_cast<std::size_t>(keep)]);
  }

  for (std::int32_t i = 1; i < MAX_BALLS; ++i) {
    balls[static_cast<std::size_t>(i)].deactivate();
  }
  balls[0].activate();
  numActiveBalls = 1;
}

void Board::slowDown() {
  Ball::speedFactor = 3;
  numBounces = 0;
}

void Board::releaseStickyBall() {
  Ball& primary = balls[0];
  if (!primary.isActive() || !primary.stopped()) {
    stickyPaddleCount = 0;
    return;
  }

  const std::int32_t launchDx = primary.aimDx == 0 ? 1 : primary.aimDx;
  primary.setSpeed(launchDx, -3);
  primary.move(4096);
  stickyPaddleCount = 0;
}

void Board::moveDownBricks() {
  bricks.moveDown();
}

void Board::shoot() {
  releaseStickyBall();

  if (game == nullptr) {
    return;
  }

  const Rect ext = paddle.extent();

  // Bomb (gun ammo): fire from the paddle centre, slightly above it, only when ammo is
  // available and the single bomb slot is free. The original plays no fire sound here.
  if (game->ammoCount() > 0 && !bomb.isActive()) {
    bomb.activate(ext.x + (ext.width / 2) - (bomb.width / 2), ext.y - 7, 10);
    bomb.setScaleFactorY(factorY);
    game->decreaseAmmo();
    if (game->ammoCount() == 0) {
      paddle.setMode(Paddle::MODE_DEFAULT);
    }
    return;
  }

  if (!gotLaser) {
    return;
  }

  // Lasers: left bullet from slots {0,1}, right bullet from slots {2,3}, fired from the
  // paddle's quarter and three-quarter points. A single laser sound plays if any fired.
  std::int32_t leftIndex = -1;
  std::int32_t rightIndex = -1;
  if (!lasers[0].isActive()) {
    leftIndex = 0;
  } else if (!lasers[1].isActive()) {
    leftIndex = 1;
  }
  if (!lasers[2].isActive()) {
    rightIndex = 2;
  } else if (!lasers[3].isActive()) {
    rightIndex = 3;
  }

  const std::int32_t quarter = paddle.width >> 2;
  if (leftIndex >= 0) {
    Bullet& laser = lasers[static_cast<std::size_t>(leftIndex)];
    laser.activate(ext.x + quarter, ext.y, 15);
    laser.setScaleFactorY(factorY);
  }
  if (rightIndex >= 0) {
    Bullet& laser = lasers[static_cast<std::size_t>(rightIndex)];
    laser.activate(ext.x + (3 * quarter), ext.y, 15);
    laser.setScaleFactorY(factorY);
  }
  if (leftIndex >= 0 || rightIndex >= 0) {
    Sounds::instance().play(Sounds::SOUND_LASER);
  }
}

void Board::explode() {
  // Matches the original explode(): trigger the flash, clear any flash message, and
  // crucially reset gotBomb so the explosive ball is consumed after a single detonation.
  flashCount = 4;
  flashingString = "";
  gotBomb = false;
}

const char* Board::statusText() const {
  if (flashCount > 0 && flashingString != nullptr) {
    return flashingString;
  }
  return "";
}

void Board::processProjectileCollision(Bullet& projectile, bool isBomb) {
  if (!projectile.isActive()) {
    return;
  }

  const std::int32_t col = (projectile.x + (projectile.width / 2)) / tileWidth;
  const std::int32_t row = (projectile.y - bricks.amountMoved) / tileHeight;
  if (row < 0 || row >= Bricks::ROWS || col < 0 || col >= Bricks::COLUMNS) {
    return;
  }

  const std::int32_t cell = bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
  if (cell <= 0) {
    return;
  }

  if (isBomb) {
    // Bomb projectile instantly destroys one brick and awards 50 points (Board side);
    // it does not go through the explosive-ball hitBrick path.
    bricks.destroyBrick(col, row);
    increasePoints(50);
  } else {
    // Laser deals one point of damage; the per-hit 10 points are awarded inside hitBrick.
    bricks.hitBrick(col, row, 1);
  }
  projectile.deactivate();
}

const char* Board::getPowerUpMessage(std::int32_t id) const {
  switch (id) {
    case Pill::LNG:
      return "LONG";
    case Pill::GUN:
      return "GUN";
    case Pill::SLW:
      return "SLOW";
    case Pill::NEW:
      return "NEW";
    case Pill::FLP:
      return "FLIP";
    case Pill::CAT:
      return "CATCH";
    case Pill::LAS:
      return "LASER";
    case Pill::LIF:
      return "LIFE";
    case Pill::WRP:
      return "WARP";
    case Pill::BMB:
      return "BOMB";
    default:
      return "";
  }
}

void Board::syncMetrics() {
  tileWidth = std::max<std::int32_t>(1, width / Bricks::COLUMNS);
  tileHeight = std::max<std::int32_t>(1, height / 15);
  factorX = std::max<std::int32_t>(1, (width << 16) / BASE_WIDTH);
  factorY = std::max<std::int32_t>(1, (height << 16) / BASE_HEIGHT);

  WIDTH = width;
  HEIGHT = height;
  TILEWIDTH = tileWidth;
  TILEHEIGHT = tileHeight;
  FACTORX = factorX;
  FACTORY = factorY;
}

}  // namespace libbrickbreaker
