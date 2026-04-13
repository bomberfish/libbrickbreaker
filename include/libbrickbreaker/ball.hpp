#pragma once

#include <cstdint>

namespace libbrickbreaker {

class Board;
class Bricks;
class Graphics;
class Paddle;

class Ball {
 public:
  static std::int32_t speedFactor;
  static bool automatedTesting;

  static constexpr std::int32_t RADIUS = 3;

  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t oldx{0};
  std::int32_t oldy{0};
  std::int32_t dx{0};
  std::int32_t dy{0};
  std::int32_t aimDx{1};
  std::int32_t maxHeight{0};
  std::int32_t stickyTimeout{0};
  bool active{false};
  std::int32_t numNonPaddleHits{0};
  std::int32_t numNonBrickHits{0};
  std::int32_t tickPos{0};

  Ball(Board* board = nullptr);

  void setBoard(Board* board);

  static void layoutStatic();
  void initialize();
  void resize(std::int32_t oldWidth, std::int32_t oldHeight);
  void makeSticky(std::int32_t centerX);
  void direction(std::int32_t delta);
  void setSpeed(std::int32_t dx, std::int32_t dy);
  void calculateCollisions(std::int32_t boardW,
                           std::int32_t boardH,
                           Paddle* paddle,
                           std::int32_t scaledElapsed);

  bool stopped() const;
  void activate();
  void deactivate();
  bool isActive() const;

  bool checkBallTop(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1);
  bool checkBallLeft(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1);
  bool checkBallRight(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1);
  bool checkBallBottom(std::int32_t x0, std::int32_t y0, std::int32_t x1, std::int32_t y1);

  void hit(std::int32_t x, std::int32_t y);
  bool checkBrickCollisions();
  void move(std::int32_t scaledElapsed);
  void render(Graphics* graphics) const;

 private:
  Board* board_{nullptr};

  bool checkBrickUpRight(Bricks& bricks,
                         std::int32_t col,
                         std::int32_t row,
                         std::int32_t tileWidth,
                         std::int32_t tileHeight);
  bool checkBrickDownRight(Bricks& bricks,
                           std::int32_t col,
                           std::int32_t row,
                           std::int32_t tileWidth,
                           std::int32_t tileHeight);
  bool checkBrickDownLeft(Bricks& bricks,
                          std::int32_t col,
                          std::int32_t row,
                          std::int32_t tileWidth,
                          std::int32_t tileHeight);
  bool checkBrickUpLeft(Bricks& bricks,
                        std::int32_t col,
                        std::int32_t row,
                        std::int32_t tileWidth,
                        std::int32_t tileHeight);
  static bool isSolidBrick(const Bricks& bricks, std::int32_t x, std::int32_t y);
  static std::int32_t mapPaddleZoneToDx(std::int32_t zone);
};

}  // namespace libbrickbreaker
