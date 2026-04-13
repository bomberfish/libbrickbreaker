#include "libbrickbreaker/board.hpp"

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
      lasers{Bullet(), Bullet(), Bullet(), Bullet()} {}

void Board::setGame(Game* gameIn) {
  game = gameIn;
}

void Board::resize(std::int32_t xIn, std::int32_t yIn, std::int32_t widthIn, std::int32_t heightIn) {
  (void)xIn;
  (void)yIn;
  (void)widthIn;
  (void)heightIn;
}

void Board::initialize(std::int32_t level, bool reloadBricks) {
  (void)level;
  (void)reloadBricks;
}

void Board::initialize(std::int32_t level) {
  (void)level;
}

std::int32_t Board::stickyCountPercentage() const {
  return 0;
}

std::int32_t Board::rand() {
  return 0;
}

std::int32_t Board::rand(std::int32_t max) {
  (void)max;
  return 0;
}

Paddle* Board::paddleRef() {
  return &paddle;
}

const Paddle* Board::paddleRef() const {
  return &paddle;
}

void Board::update(std::int32_t elapsedMs) {
  (void)elapsedMs;
}

void Board::updateBalls(std::int32_t scaledElapsed) {
  (void)scaledElapsed;
}

std::int32_t Board::increaseBounces(std::int32_t amount) {
  (void)amount;
  return 0;
}

void Board::decreaseBalls() {}

void Board::setBGColor(std::int32_t color) {
  (void)color;
}

void Board::setFGColor(std::int32_t color) {
  (void)color;
}

void Board::setPointerColor(std::int32_t color) {
  (void)color;
}

void Board::setColors(std::int32_t backgroundColorIn,
                      std::int32_t foregroundColorIn,
                      std::int32_t pointerColorIn) {
  (void)backgroundColorIn;
  (void)foregroundColorIn;
  (void)pointerColorIn;
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
  (void)paused;
}

void Board::render(Graphics* graphics) {
  (void)graphics;
}

void Board::renderProjectiles(Graphics* graphics) {
  (void)graphics;
}

void Board::powerUp(std::int32_t id) {
  (void)id;
}

void Board::increasePoints(std::int32_t points) {
  (void)points;
}

void Board::applyBrickScore() {}

void Board::addBalls() {}

void Board::killBallsExceptOne() {}

void Board::slowDown() {}

void Board::releaseStickyBall() {}

void Board::moveDownBricks() {}

void Board::shoot() {}

void Board::explode() {}

const char* Board::statusText() const {
  return "";
}

void Board::processProjectileCollision(Bullet& projectile, std::int32_t damage, bool isBomb) {
  (void)projectile;
  (void)damage;
  (void)isBomb;
}

const char* Board::getPowerUpMessage(std::int32_t id) const {
  (void)id;
  return "";
}

void Board::syncMetrics() {}

}  // namespace libbrickbreaker
