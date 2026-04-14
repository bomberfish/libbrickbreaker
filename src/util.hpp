#pragma once

#include <cstdint>

#include "libbrickbreaker/types.hpp"

namespace libbrickbreaker {

class Util {
 public:
  static bool intersect(std::int32_t ax0,
                        std::int32_t ay0,
                        std::int32_t ax1,
                        std::int32_t ay1,
                        std::int32_t bx0,
                        std::int32_t by0,
                        std::int32_t bx1,
                        std::int32_t by1,
                        Point* hitPoint);
};

}  // namespace libbrickbreaker