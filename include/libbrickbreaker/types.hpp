#pragma once

#include <cstdint>

namespace libbrickbreaker {

struct Point {
  std::int32_t x{0};
  std::int32_t y{0};
};

struct Rect {
  std::int32_t x{0};
  std::int32_t y{0};
  std::int32_t width{0};
  std::int32_t height{0};
};

enum class PointerType : std::uint8_t {
  kTap,
  kStart,
  kMove,
  kEnd,
};

struct PointerEvent {
  PointerType type{PointerType::kTap};
  std::int32_t x{0};
  std::int32_t y{0};
};

struct KeyEvent {
  std::int32_t keycode{0};
  std::int64_t timeMs{0};
};

struct NavigationEvent {
  std::int32_t status{0};
  std::int64_t timeMs{0};
  std::int32_t deltaX{0};
  std::int32_t deltaY{0};
};

class Graphics {
 public:
  virtual ~Graphics() = default;
};

class RenderContext {
 public:
  virtual ~RenderContext() = default;
};

class ImageAsset {
 public:
  virtual ~ImageAsset() = default;
};

class LayoutGroup {
 public:
  virtual ~LayoutGroup() = default;
};

class Menu {
 public:
  virtual ~Menu() = default;
};

}  // namespace libbrickbreaker
