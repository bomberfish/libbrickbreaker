#include "libbrickbreaker/paddle.hpp"

#include <algorithm>
#include <array>
#include <cmath>

#include "libbrickbreaker/board.hpp"

namespace libbrickbreaker {

void Paddle::setBoardSize(std::int32_t widthIn, std::int32_t heightIn) {
  boardWidth_ = std::max<std::int32_t>(1, widthIn);
  boardHeight_ = std::max<std::int32_t>(1, heightIn);
}

void Paddle::initialize() {
  velocity = 0;
  mode = MODE_DEFAULT;
  flipped = false;
  sticky = false;
  warp = false;
  wrapped = false;
  inputMode = INPUT_TRACKPAD;

  width = std::max<std::int32_t>(16, boardWidth_ / 4);
  height = std::max<std::int32_t>(4, boardHeight_ / 28);
  x = (boardWidth_ - width) / 2;
  y = std::max<std::int32_t>(0, boardHeight_ - height - 2);
}

void Paddle::resize() {
  const std::int32_t center = centerX();
  const std::int32_t currentMode = mode;

  if (width <= 0) {
    width = std::max<std::int32_t>(16, boardWidth_ / 4);
  }
  if (height <= 0) {
    height = std::max<std::int32_t>(4, boardHeight_ / 28);
  }

  setMode(currentMode);
  x = center - (width / 2);
  applyHorizontalBounds();
  y = std::max<std::int32_t>(0, std::min<std::int32_t>(y, boardHeight_ - height));
}

void Paddle::setLocation(std::int32_t xIn, std::int32_t yIn) {
  x = xIn;
  y = yIn;
  wrapped = warp && (x < 0 || x + width > boardWidth_);
}

void Paddle::setSize(std::int32_t widthIn, std::int32_t heightIn) {
  width = std::max<std::int32_t>(1, widthIn);
  height = std::max<std::int32_t>(1, heightIn);
  wrapped = warp && (x < 0 || x + width > boardWidth_);
}

void Paddle::updateReticle() {}

Rect Paddle::extent() const {
  return Rect{x, y, width, height};
}

std::int32_t Paddle::centerX() const {
  return x + (width / 2);
}

bool Paddle::intersectsTouch(std::int32_t xIn) const {
  const std::int32_t pointX = xIn;
  for (const Rect& rect : getCollisionRects()) {
    if (pointX >= rect.x && pointX < rect.x + rect.width) {
      return true;
    }
  }

  return false;
}

bool Paddle::intersectLineSegment(std::int32_t x0,
                                  std::int32_t y0,
                                  std::int32_t x1,
                                  std::int32_t y1,
                                  Point* intersection) const {
  if (intersection == nullptr) {
    return false;
  }

  for (const Rect& rect : getCollisionRects()) {
    if (segmentIntersectsRect(x0, y0, x1, y1, rect, intersection)) {
      return true;
    }
  }

  return false;
}

bool Paddle::intersectsWithRect(std::int32_t xIn,
                                std::int32_t yIn,
                                std::int32_t widthIn,
                                std::int32_t heightIn) const {
  const Rect target{xIn, yIn, widthIn, heightIn};
  for (const Rect& rect : getCollisionRects()) {
    if (rectsIntersect(rect, target)) {
      return true;
    }
  }

  return false;
}

bool Paddle::isWrapped() const {
  return wrapped;
}

std::int32_t Paddle::currentInputMode() const {
  return inputMode;
}

void Paddle::setInputMode(std::int32_t modeIn) {
  inputMode = std::max<std::int32_t>(INPUT_DEFAULT, std::min<std::int32_t>(INPUT_TRACKPAD, modeIn));
}

void Paddle::setMode(std::int32_t modeIn) {
  const std::int32_t resolvedMode = std::max<std::int32_t>(MODE_DEFAULT, std::min<std::int32_t>(MODE_GUN, modeIn));
  if (resolvedMode == mode && width > 0) {
    return;
  }

  const std::int32_t center = centerX();
  mode = resolvedMode;
  width = mode == MODE_LONG ? std::max<std::int32_t>(20, boardWidth_ / 3) : std::max<std::int32_t>(16, boardWidth_ / 4);
  height = std::max<std::int32_t>(4, boardHeight_ / 28);
  x = center - (width / 2);
  applyHorizontalBounds();
}

void Paddle::moveTo(std::int32_t targetX, const Board& board) {
  setBoardSize(board.width, board.height);
  std::int32_t resolvedX = targetX - (width / 2);

  if (warp && wrapped) {
    const std::int32_t wrappedCandidate = resolvedX < x ? resolvedX + boardWidth_ : resolvedX - boardWidth_;
    if (std::abs(wrappedCandidate - x) < std::abs(resolvedX - x)) {
      resolvedX = wrappedCandidate;
    }
  }

  setLocation(resolvedX, y);
  applyHorizontalBounds();
  updateReticle();
}

void Paddle::move(std::int32_t delta, const Board& board) {
  setBoardSize(board.width, board.height);

  Ball& primary = const_cast<Ball&>(board.balls[0]);
  if (primary.stopped()) {
    primary.direction(delta);
    return;
  }

  inputMode = INPUT_TRACKPAD;
  velocity += delta * 8;
  velocity = std::max<std::int32_t>(-96, std::min<std::int32_t>(96, velocity));

  std::int32_t step = velocity / 16;
  if (step == 0 && velocity != 0) {
    step = velocity > 0 ? 1 : -1;
  }

  if (flipped) {
    step = -step;
  }

  if (step != 0) {
    setLocation(x + step, y);
    applyHorizontalBounds();
  }

  velocity = (velocity * 7) / 10;
  if (std::abs(velocity) < 2) {
    velocity = 0;
  }

  updateReticle();
}

void Paddle::render(Graphics* graphics) const {
  (void)graphics;
}

void Paddle::applyHorizontalBounds() {
  if (warp) {
    while (x < -width) {
      x += boardWidth_;
    }
    while (x > boardWidth_) {
      x -= boardWidth_;
    }
    wrapped = x < 0 || x + width > boardWidth_;
    return;
  }

  x = std::max<std::int32_t>(0, std::min<std::int32_t>(x, boardWidth_ - width));
  wrapped = false;
}

std::vector<Rect> Paddle::getCollisionRects() const {
  if (!warp || !wrapped) {
    return {extent()};
  }

  std::vector<Rect> rects;
  if (x < 0) {
    rects.push_back(Rect{0, y, x + width, height});
    rects.push_back(Rect{boardWidth_ + x, y, -x, height});
  } else if (x + width > boardWidth_) {
    rects.push_back(Rect{x, y, boardWidth_ - x, height});
    rects.push_back(Rect{0, y, x + width - boardWidth_, height});
  } else {
    rects.push_back(extent());
  }

  std::vector<Rect> filtered;
  filtered.reserve(rects.size());
  for (const Rect& rect : rects) {
    if (rect.width > 0 && rect.height > 0) {
      filtered.push_back(rect);
    }
  }
  return filtered;
}

bool Paddle::rectsIntersect(const Rect& a, const Rect& b) {
  return a.x < b.x + b.width && a.x + a.width > b.x && a.y < b.y + b.height && a.y + a.height > b.y;
}

bool Paddle::segmentIntersectsRect(std::int32_t x0,
                                   std::int32_t y0,
                                   std::int32_t x1,
                                   std::int32_t y1,
                                   const Rect& rect,
                                   Point* hitPoint) {
  if (hitPoint == nullptr) {
    return false;
  }

  const double dx = static_cast<double>(x1 - x0);
  const double dy = static_cast<double>(y1 - y0);
  const std::array<double, 4> p{-dx, dx, -dy, dy};
  const std::array<double, 4> q{
      static_cast<double>(x0 - rect.x),
      static_cast<double>(rect.x + rect.width - x0),
      static_cast<double>(y0 - rect.y),
      static_cast<double>(rect.y + rect.height - y0),
  };

  double t0 = 0.0;
  double t1 = 1.0;

  for (std::size_t i = 0; i < p.size(); ++i) {
    if (p[i] == 0.0) {
      if (q[i] < 0.0) {
        return false;
      }
      continue;
    }

    const double ratio = q[i] / p[i];
    if (p[i] < 0.0) {
      if (ratio > t1) {
        return false;
      }
      t0 = std::max(t0, ratio);
    } else {
      if (ratio < t0) {
        return false;
      }
      t1 = std::min(t1, ratio);
    }
  }

  if (t0 < 0.0 || t0 > 1.0) {
    return false;
  }

  hitPoint->x = static_cast<std::int32_t>(x0 + t0 * dx);
  hitPoint->y = static_cast<std::int32_t>(y0 + t0 * dy);
  return true;
}

}  // namespace libbrickbreaker
