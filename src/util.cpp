#include "util.hpp"

namespace libbrickbreaker {

namespace {

class SegmentIntersectionRunner {
 public:
  SegmentIntersectionRunner(std::int32_t ax0,
                            std::int32_t ay0,
                            std::int32_t ax1,
                            std::int32_t ay1,
                            std::int32_t bx0,
                            std::int32_t by0,
                            std::int32_t bx1,
                            std::int32_t by1,
                            Point* hitPoint)
      : ax0_(ax0),
        ay0_(ay0),
        ax1_(ax1),
        ay1_(ay1),
        bx0_(bx0),
        by0_(by0),
        bx1_(bx1),
        by1_(by1),
        hitPoint_(hitPoint) {}

  bool run() {
    if (hitPoint_ == nullptr) {
      return false;
    }

    if (ax0_ == ax1_) {
      return intersectVertical();
    }

    if (ay0_ == ay1_) {
      return intersectHorizontal();
    }

    return false;
  }

 private:
  bool intersectVertical() {
    const std::int32_t lineX = ax0_;
    const std::int32_t minY = ay0_ < ay1_ ? ay0_ : ay1_;
    const std::int32_t maxY = ay0_ > ay1_ ? ay0_ : ay1_;
    const std::int32_t deltaX = bx1_ - bx0_;

    if (deltaX == 0) {
      if (bx0_ != lineX) {
        return false;
      }

      const std::int32_t overlapMin = (minY > by0_ ? minY : by0_) < by1_ ? (minY > by0_ ? minY : by0_) : by1_;
      const std::int32_t overlapMax = (maxY < by0_ ? maxY : by0_) > by1_ ? (maxY < by0_ ? maxY : by0_) : by1_;
      if (overlapMin > overlapMax) {
        return false;
      }

      hitPoint_->x = lineX;
      hitPoint_->y = overlapMin;
      return true;
    }

    const double t = static_cast<double>(lineX - bx0_) / static_cast<double>(deltaX);
    if (t < 0.0 || t > 1.0) {
      return false;
    }

    const double y = static_cast<double>(by0_) + static_cast<double>(by1_ - by0_) * t;
    if (y < minY || y > maxY) {
      return false;
    }

    hitPoint_->x = lineX;
    hitPoint_->y = static_cast<std::int32_t>(y);
    return true;
  }

  bool intersectHorizontal() {
    const std::int32_t lineY = ay0_;
    const std::int32_t minX = ax0_ < ax1_ ? ax0_ : ax1_;
    const std::int32_t maxX = ax0_ > ax1_ ? ax0_ : ax1_;
    const std::int32_t deltaY = by1_ - by0_;

    if (deltaY == 0) {
      if (by0_ != lineY) {
        return false;
      }

      const std::int32_t overlapMin = (minX > bx0_ ? minX : bx0_) < bx1_ ? (minX > bx0_ ? minX : bx0_) : bx1_;
      const std::int32_t overlapMax = (maxX < bx0_ ? maxX : bx0_) > bx1_ ? (maxX < bx0_ ? maxX : bx0_) : bx1_;
      if (overlapMin > overlapMax) {
        return false;
      }

      hitPoint_->x = overlapMin;
      hitPoint_->y = lineY;
      return true;
    }

    const double t = static_cast<double>(lineY - by0_) / static_cast<double>(deltaY);
    if (t < 0.0 || t > 1.0) {
      return false;
    }

    const double x = static_cast<double>(bx0_) + static_cast<double>(bx1_ - bx0_) * t;
    if (x < minX || x > maxX) {
      return false;
    }

    hitPoint_->x = static_cast<std::int32_t>(x);
    hitPoint_->y = lineY;
    return true;
  }

  std::int32_t ax0_;
  std::int32_t ay0_;
  std::int32_t ax1_;
  std::int32_t ay1_;
  std::int32_t bx0_;
  std::int32_t by0_;
  std::int32_t bx1_;
  std::int32_t by1_;
  Point* hitPoint_;
};

}  // namespace

bool Util::intersect(std::int32_t ax0,
                     std::int32_t ay0,
                     std::int32_t ax1,
                     std::int32_t ay1,
                     std::int32_t bx0,
                     std::int32_t by0,
                     std::int32_t bx1,
                     std::int32_t by1,
                     Point* hitPoint) {
  SegmentIntersectionRunner runner(ax0, ay0, ax1, ay1, bx0, by0, bx1, by1, hitPoint);
  return runner.run();
}

}  // namespace libbrickbreaker