#include "libbrickbreaker/sprites.hpp"

#include <array>
#include <cstddef>

namespace libbrickbreaker {

std::array<const ImageAsset*, kNumSpriteKeys> Sprites::store_{};

void Sprites::setSprite(SpriteKey key, const ImageAsset* image) {
  const auto index = static_cast<std::size_t>(key);
  store_[index] = image;
}

const ImageAsset* Sprites::getSprite(SpriteKey key) {
  const auto index = static_cast<std::size_t>(key);
  return store_[index];
}

void Sprites::clear() {
  store_.fill(nullptr);
}

}  // namespace libbrickbreaker
