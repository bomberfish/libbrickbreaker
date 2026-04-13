#pragma once

#include <cstdint>
#include <string>

namespace libbrickbreaker {

class Graphics;
class Paddle;

class Pill {
 public:
  static constexpr std::int32_t LNG = 1;
  static constexpr std::int32_t GUN = 2;
  static constexpr std::int32_t SHR = 3;
  static constexpr std::int32_t SLW = 4;
  static constexpr std::int32_t NEW = 5;
  static constexpr std::int32_t FLP = 6;
  static constexpr std::int32_t CAT = 7;
  static constexpr std::int32_t LAS = 8;
  static constexpr std::int32_t LIF = 9;
  static constexpr std::int32_t WRP = 11;
  static constexpr std::int32_t BMB = 12;

  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t dy{0};
  bool active{false};
  std::int32_t bonusType{0};
  std::string label{};
  std::int32_t width{12};
  std::int32_t height{8};
  std::int32_t count{0};

  void initialize();
  void resize(std::int32_t oldWidth,
              std::int32_t oldHeight,
              std::int32_t newWidth,
              std::int32_t newHeight);
  void setMotionContext(std::int32_t boardHeight, std::int32_t factorY);
  void activate(std::int32_t x, std::int32_t y);
  void eat();
  bool checkCollision(const Paddle& paddle) const;
  void move(std::int32_t scaledElapsed);
  void setBonusType(std::int32_t id);
  bool isActive() const;
  void render(Graphics* graphics) const;

 private:
  [[maybe_unused]] std::int32_t boardHeight_{195};
  [[maybe_unused]] std::int32_t factorY_{1 << 16};
};

}  // namespace libbrickbreaker
