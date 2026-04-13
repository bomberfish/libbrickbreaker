#pragma once

#include <cstdint>
#include <vector>

#include "types.hpp"

namespace libbrickbreaker {

class Board;
class Graphics;

class Paddle {
 public:
  static constexpr std::int32_t MODE_DEFAULT = 0;
  static constexpr std::int32_t MODE_LONG = 1;
  static constexpr std::int32_t MODE_LASER = 2;
  static constexpr std::int32_t MODE_GUN = 3;

  static constexpr std::int32_t INPUT_DEFAULT = 0;
  static constexpr std::int32_t INPUT_TOUCH = 1;
  static constexpr std::int32_t INPUT_TRACKPAD = 2;

  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{0};
  std::int32_t height{0};
  std::int32_t velocity{0};
  bool wrapped{false};
  std::int32_t inputMode{0};
  std::int32_t mode{0};
  bool flipped{false};
  bool sticky{false};
  bool warp{false};

  void setBoardSize(std::int32_t width, std::int32_t height);
  void initialize();
  void resize();

  void setLocation(std::int32_t x, std::int32_t y);
  void setSize(std::int32_t width, std::int32_t height);
  void updateReticle();

  Rect extent() const;
  std::int32_t centerX() const;
  bool intersectsTouch(std::int32_t x) const;
  bool intersectLineSegment(std::int32_t x0,
                            std::int32_t y0,
                            std::int32_t x1,
                            std::int32_t y1,
                            Point* intersection) const;
  bool intersectsWithRect(std::int32_t x, std::int32_t y, std::int32_t width, std::int32_t height) const;

  bool isWrapped() const;
  std::int32_t currentInputMode() const;
  void setInputMode(std::int32_t mode);

  void setMode(std::int32_t mode);
  void moveTo(std::int32_t targetX, const Board& board);
  void move(std::int32_t delta, const Board& board);
  void render(Graphics* graphics) const;

 private:
  std::int32_t boardWidth_{189};
  std::int32_t boardHeight_{195};

  void applyHorizontalBounds();
  std::vector<Rect> getCollisionRects() const;
  static bool rectsIntersect(const Rect& a, const Rect& b);
  static bool segmentIntersectsRect(std::int32_t x0,
                                    std::int32_t y0,
                                    std::int32_t x1,
                                    std::int32_t y1,
                                    const Rect& rect,
                                    Point* hitPoint);
};

}  // namespace libbrickbreaker
