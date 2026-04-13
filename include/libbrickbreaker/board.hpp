#pragma once

#include <array>
#include <cstdint>

#include "ball.hpp"
#include "bricks.hpp"
#include "bullet.hpp"
#include "paddle.hpp"
#include "pills.hpp"

namespace libbrickbreaker {

class Game;
class Graphics;

class Board {
 public:
  static constexpr std::int32_t MAX_BALLS = 4;
  static constexpr std::int32_t MAX_LASERS = 4;
  static constexpr std::int32_t MAX_PILLS = 3;
  static constexpr std::int32_t BASE_WIDTH = 189;
  static constexpr std::int32_t BASE_HEIGHT = 195;

  static std::int32_t WIDTH;
  static std::int32_t HEIGHT;
  static std::int32_t TILEWIDTH;
  static std::int32_t TILEHEIGHT;
  static std::int32_t FACTORX;
  static std::int32_t FACTORY;

  Board();
  explicit Board(Game* game);

  void setGame(Game* game);

  void resize(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height);
  void initialize(std::int32_t level, bool reloadBricks);
  void initialize(std::int32_t level);

  std::int32_t stickyCountPercentage() const;

  std::int32_t rand();
  std::int32_t rand(std::int32_t max);

  Paddle* paddleRef();
  const Paddle* paddleRef() const;

  void update(std::int32_t elapsedMs);
  void updateBalls(std::int32_t scaledElapsed);

  std::int32_t increaseBounces(std::int32_t amount);
  void decreaseBalls();

  void setBGColor(std::int32_t color);
  void setFGColor(std::int32_t color);
  void setPointerColor(std::int32_t color);
  void setColors(std::int32_t backgroundColor, std::int32_t foregroundColor, std::int32_t pointerColor);

  std::int32_t widthPx() const;
  std::int32_t xPos() const;
  std::int32_t yPos() const;
  std::int32_t foregroundColorValue() const;
  std::int32_t pointerColorValue() const;

  void gamePaused(bool paused);

  void render(Graphics* graphics);
  void renderProjectiles(Graphics* graphics);

  void powerUp(std::int32_t id);
  void increasePoints(std::int32_t points);
  void applyBrickScore();

  void addBalls();
  void killBallsExceptOne();
  void slowDown();
  void releaseStickyBall();

  void moveDownBricks();
  void shoot();
  void explode();

  const char* statusText() const;

  Game* game{nullptr};
  Paddle paddle;
  Bricks bricks;
  Pills pills;
  Bullet bomb;
  std::array<Ball, MAX_BALLS> balls;
  std::array<Bullet, MAX_LASERS> lasers;

  std::int32_t numActiveBalls{0};
  std::int32_t numBounces{0};
  std::int32_t stickyPaddleCount{0};
  bool gotBomb{false};
  bool gotLaser{false};

  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{BASE_WIDTH};
  std::int32_t height{BASE_HEIGHT};
  std::int32_t tileWidth{BASE_WIDTH / 7};
  std::int32_t tileHeight{BASE_HEIGHT / 15};
  std::int32_t factorX{1 << 16};
  std::int32_t factorY{1 << 16};
  std::int32_t foregroundColor{0};
  std::int32_t backgroundColor{0};
  std::int32_t pointerColor{0};

  bool ghosting{false};
  std::int32_t ghostingCount{0};
  std::int32_t flashCount{0};
  const char* flashingString{nullptr};

 private:
  [[maybe_unused]] std::uint32_t randomSeed_{0x6d2b79f5u};
  [[maybe_unused]] bool lifeDecrementPending_{false};

  void processProjectileCollision(Bullet& projectile, std::int32_t damage, bool isBomb);
  const char* getPowerUpMessage(std::int32_t id) const;
  void syncMetrics();
};

}  // namespace libbrickbreaker
