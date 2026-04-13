#pragma once

#include <cstdint>

namespace libbrickbreaker {

class Graphics;
class ImageAsset;

class Bullet {
 public:
  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t dy{-1};
  std::int32_t width{0};
  std::int32_t height{0};
  const ImageAsset* sprite{nullptr};

  void initialize();
  void resize(const ImageAsset* bitmap, std::int32_t oldWidth, std::int32_t oldHeight);
  void setScaleFactorY(std::int32_t factorY);
  void activate(std::int32_t x, std::int32_t y, std::int32_t dy);
  void move(std::int32_t scaledElapsed);
  void deactivate();
  bool isActive() const;
  void render(Graphics* graphics) const;

 private:
  [[maybe_unused]] std::int32_t factorY_{1 << 16};
};

}  // namespace libbrickbreaker
