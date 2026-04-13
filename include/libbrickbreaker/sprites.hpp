#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace libbrickbreaker {

class ImageAsset;

enum class SpriteKey : std::uint8_t {
  kBricks = 0,
  kPaddles = 1,
  kPaddleLong = 2,
  kPills = 3,
  kLaser = 4,
  kBomb = 5,
  kBalls = 6,
};

constexpr std::size_t kNumSpriteKeys = 7;

class Sprites {
 public:
  static void setSprite(SpriteKey key, const ImageAsset* image);
  static const ImageAsset* getSprite(SpriteKey key);
  static void clear();

 private:
  static std::array<const ImageAsset*, kNumSpriteKeys> store_;
};

}  // namespace libbrickbreaker
