// SDL2 frontend for libbrickbreaker.
//
// Builds natively against system SDL2 / SDL2_mixer (auto-detected) and via
// Emscripten using -sUSE_SDL=2 -sUSE_SDL_MIXER=2. Sprites are loaded with the
// vendored stb_image header, so no SDL_image dependency is required.

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#if __has_include(<SDL2/SDL.h>)
#  include <SDL2/SDL.h>
#  define BRICKBREAKER_HAS_SDL 1
#elif __has_include(<SDL.h>)
#  include <SDL.h>
#  define BRICKBREAKER_HAS_SDL 1
#else
#  define BRICKBREAKER_HAS_SDL 0
#endif

#if BRICKBREAKER_HAS_SDL
#  if __has_include(<SDL2/SDL_mixer.h>)
#    include <SDL2/SDL_mixer.h>
#    define BRICKBREAKER_HAS_SDL_MIXER 1
#  elif __has_include(<SDL_mixer.h>)
#    include <SDL_mixer.h>
#    define BRICKBREAKER_HAS_SDL_MIXER 1
#  else
#    define BRICKBREAKER_HAS_SDL_MIXER 0
#  endif
#else
#  define BRICKBREAKER_HAS_SDL_MIXER 0
#endif

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif

#include "libbrickbreaker/libbrickbreaker.hpp"

#if BRICKBREAKER_HAS_SDL

// Vendored stb_image (single header, public domain). The implementation lives
// in stb_image_impl.cpp so the upstream code can be compiled with warnings
// relaxed without affecting this translation unit.
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "external/stb_image.h"

namespace {

using namespace libbrickbreaker;

// ---------------------------------------------------------------------------
// Constants & layout
// ---------------------------------------------------------------------------

constexpr int kHudHeight = 36;
constexpr int kInitialWindowWidth = 540;
constexpr int kInitialWindowHeight = 720;
constexpr std::int32_t kBaseBoardWidth = Board::BASE_WIDTH;
constexpr std::int32_t kBaseBoardHeight = Board::BASE_HEIGHT;
constexpr std::int32_t kBrickFrameRows = 9;

constexpr int kSoundCount = Sounds::kSoundCount;

// Cooldowns mirror the Android frontend (AudioEngine.cpp) so high-rate brick
// hits don't stutter audio playback.
constexpr std::array<std::int32_t, kSoundCount> kSoundCooldownMs = {
    20, 20, 30, 60, 28, 20, 24, 28};
constexpr std::array<float, kSoundCount> kSoundGain = {
    0.78f, 0.78f, 0.85f, 0.92f, 0.72f, 0.64f, 0.55f, 0.86f};
constexpr std::array<const char*, kSoundCount> kSoundFilenames = {
    "sounds/poppill.ogg",   "sounds/eatpill.ogg",     "sounds/laser.ogg",
    "sounds/bomb.ogg",      "sounds/brickdestroy.ogg","sounds/brickhit.ogg",
    "sounds/ceiling.ogg",   "sounds/paddle.ogg"};

// ---------------------------------------------------------------------------
// Asset directory resolution
// ---------------------------------------------------------------------------

std::string g_assetDir;

bool fileExists(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  return in.good();
}

std::string joinPath(const std::string& dir, const std::string& rel) {
  if (dir.empty()) {
    return rel;
  }
  if (dir.back() == '/' || dir.back() == '\\') {
    return dir + rel;
  }
  return dir + "/" + rel;
}

void resolveAssetDir(const char* override) {
  if (override != nullptr && override[0] != '\0' &&
      fileExists(joinPath(override, "levels.bin"))) {
    g_assetDir = override;
    return;
  }

#ifdef __EMSCRIPTEN__
  // Inside the WASM virtual filesystem we preload assets at /assets.
  g_assetDir = "/assets";
  return;
#else
  static const char* const kCandidates[] = {
      "assets",
      "./assets",
      "../assets",
      "../../assets",
      "../../../assets",
  };
  for (const char* c : kCandidates) {
    if (fileExists(joinPath(c, "levels.bin"))) {
      g_assetDir = c;
      return;
    }
  }
  g_assetDir.clear();
#endif
}

std::optional<std::vector<std::uint8_t>> readBinaryFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in.good()) {
    return std::nullopt;
  }
  std::vector<std::uint8_t> bytes(
      (std::istreambuf_iterator<char>(in)),
      std::istreambuf_iterator<char>());
  return bytes;
}

// ---------------------------------------------------------------------------
// Texture helpers (stb_image -> SDL_Texture)
// ---------------------------------------------------------------------------

struct Texture {
  SDL_Texture* sdl{nullptr};
  int width{0};
  int height{0};

  bool valid() const { return sdl != nullptr; }
};

void destroyTexture(Texture& tex) {
  if (tex.sdl != nullptr) {
    SDL_DestroyTexture(tex.sdl);
    tex.sdl = nullptr;
  }
  tex.width = tex.height = 0;
}

Texture loadTexture(SDL_Renderer* renderer, const std::string& path) {
  Texture out{};
  auto bytes = readBinaryFile(path);
  if (!bytes.has_value() || bytes->empty()) {
    SDL_Log("loadTexture: missing or empty asset: %s", path.c_str());
    return out;
  }

  int w = 0, h = 0, channels = 0;
  unsigned char* pixels = stbi_load_from_memory(
      bytes->data(), static_cast<int>(bytes->size()), &w, &h, &channels, STBI_rgb_alpha);
  if (pixels == nullptr) {
    SDL_Log("loadTexture: stbi_load failed for %s: %s", path.c_str(), stbi_failure_reason());
    return out;
  }

  SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
      pixels, w, h, 32, w * 4, SDL_PIXELFORMAT_ABGR8888);
  if (surface == nullptr) {
    SDL_Log("loadTexture: SDL_CreateRGBSurface failed: %s", SDL_GetError());
    stbi_image_free(pixels);
    return out;
  }

  SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, surface);
  SDL_FreeSurface(surface);
  stbi_image_free(pixels);

  if (tex == nullptr) {
    SDL_Log("loadTexture: SDL_CreateTextureFromSurface failed: %s", SDL_GetError());
    return out;
  }
  SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
  SDL_SetTextureScaleMode(tex, SDL_ScaleModeLinear);

  out.sdl = tex;
  out.width = w;
  out.height = h;
  return out;
}

// ---------------------------------------------------------------------------
// Bitmap font: 5x7 glyphs covering A-Z, 0-9, and a few punctuation chars.
// Each row is encoded with bit 4 = leftmost pixel.
// ---------------------------------------------------------------------------

using Glyph = std::array<std::uint8_t, 7>;

const Glyph& glyphFor(char ch) {
  static const Glyph kFallback = {0b01110, 0b10001, 0b00010, 0b00100, 0b00100, 0b00000, 0b00100};
  static const Glyph kSpace    = {0, 0, 0, 0, 0, 0, 0};
  static const Glyph kDigit0   = {0b01110, 0b10001, 0b10011, 0b10101, 0b11001, 0b10001, 0b01110};
  static const Glyph kDigit1   = {0b00100, 0b01100, 0b00100, 0b00100, 0b00100, 0b00100, 0b01110};
  static const Glyph kDigit2   = {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b01000, 0b11111};
  static const Glyph kDigit3   = {0b11110, 0b00001, 0b00001, 0b01110, 0b00001, 0b00001, 0b11110};
  static const Glyph kDigit4   = {0b00010, 0b00110, 0b01010, 0b10010, 0b11111, 0b00010, 0b00010};
  static const Glyph kDigit5   = {0b11111, 0b10000, 0b11110, 0b00001, 0b00001, 0b10001, 0b01110};
  static const Glyph kDigit6   = {0b00110, 0b01000, 0b10000, 0b11110, 0b10001, 0b10001, 0b01110};
  static const Glyph kDigit7   = {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b01000, 0b01000};
  static const Glyph kDigit8   = {0b01110, 0b10001, 0b10001, 0b01110, 0b10001, 0b10001, 0b01110};
  static const Glyph kDigit9   = {0b01110, 0b10001, 0b10001, 0b01111, 0b00001, 0b00010, 0b01100};
  static const Glyph kA        = {0b01110, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
  static const Glyph kB        = {0b11110, 0b10001, 0b10001, 0b11110, 0b10001, 0b10001, 0b11110};
  static const Glyph kC        = {0b01110, 0b10001, 0b10000, 0b10000, 0b10000, 0b10001, 0b01110};
  static const Glyph kD        = {0b11100, 0b10010, 0b10001, 0b10001, 0b10001, 0b10010, 0b11100};
  static const Glyph kE        = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b11111};
  static const Glyph kF        = {0b11111, 0b10000, 0b10000, 0b11110, 0b10000, 0b10000, 0b10000};
  static const Glyph kG        = {0b01110, 0b10001, 0b10000, 0b10111, 0b10001, 0b10001, 0b01110};
  static const Glyph kH        = {0b10001, 0b10001, 0b10001, 0b11111, 0b10001, 0b10001, 0b10001};
  static const Glyph kI        = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b11111};
  static const Glyph kJ        = {0b00111, 0b00010, 0b00010, 0b00010, 0b00010, 0b10010, 0b01100};
  static const Glyph kK        = {0b10001, 0b10010, 0b10100, 0b11000, 0b10100, 0b10010, 0b10001};
  static const Glyph kL        = {0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b10000, 0b11111};
  static const Glyph kM        = {0b10001, 0b11011, 0b10101, 0b10101, 0b10001, 0b10001, 0b10001};
  static const Glyph kN        = {0b10001, 0b10001, 0b11001, 0b10101, 0b10011, 0b10001, 0b10001};
  static const Glyph kO        = {0b01110, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
  static const Glyph kP        = {0b11110, 0b10001, 0b10001, 0b11110, 0b10000, 0b10000, 0b10000};
  static const Glyph kQ        = {0b01110, 0b10001, 0b10001, 0b10001, 0b10101, 0b10010, 0b01101};
  static const Glyph kR        = {0b11110, 0b10001, 0b10001, 0b11110, 0b10100, 0b10010, 0b10001};
  static const Glyph kS        = {0b01111, 0b10000, 0b10000, 0b01110, 0b00001, 0b00001, 0b11110};
  static const Glyph kT        = {0b11111, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00100};
  static const Glyph kU        = {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01110};
  static const Glyph kV        = {0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b01010, 0b00100};
  static const Glyph kW        = {0b10001, 0b10001, 0b10001, 0b10101, 0b10101, 0b10101, 0b01010};
  static const Glyph kX        = {0b10001, 0b10001, 0b01010, 0b00100, 0b01010, 0b10001, 0b10001};
  static const Glyph kY        = {0b10001, 0b10001, 0b10001, 0b01010, 0b00100, 0b00100, 0b00100};
  static const Glyph kZ        = {0b11111, 0b00001, 0b00010, 0b00100, 0b01000, 0b10000, 0b11111};
  static const Glyph kColon    = {0b00000, 0b00100, 0b00100, 0b00000, 0b00100, 0b00100, 0b00000};
  static const Glyph kPlus     = {0b00000, 0b00100, 0b00100, 0b11111, 0b00100, 0b00100, 0b00000};
  static const Glyph kDash     = {0b00000, 0b00000, 0b00000, 0b11111, 0b00000, 0b00000, 0b00000};
  static const Glyph kSlash    = {0b00001, 0b00010, 0b00010, 0b00100, 0b01000, 0b01000, 0b10000};
  static const Glyph kDot      = {0b00000, 0b00000, 0b00000, 0b00000, 0b00000, 0b00100, 0b00100};
  static const Glyph kBang     = {0b00100, 0b00100, 0b00100, 0b00100, 0b00100, 0b00000, 0b00100};
  static const Glyph kQuest    = {0b01110, 0b10001, 0b00001, 0b00010, 0b00100, 0b00000, 0b00100};

  switch (std::toupper(static_cast<unsigned char>(ch))) {
    case ' ': return kSpace;
    case '0': return kDigit0;
    case '1': return kDigit1;
    case '2': return kDigit2;
    case '3': return kDigit3;
    case '4': return kDigit4;
    case '5': return kDigit5;
    case '6': return kDigit6;
    case '7': return kDigit7;
    case '8': return kDigit8;
    case '9': return kDigit9;
    case 'A': return kA;
    case 'B': return kB;
    case 'C': return kC;
    case 'D': return kD;
    case 'E': return kE;
    case 'F': return kF;
    case 'G': return kG;
    case 'H': return kH;
    case 'I': return kI;
    case 'J': return kJ;
    case 'K': return kK;
    case 'L': return kL;
    case 'M': return kM;
    case 'N': return kN;
    case 'O': return kO;
    case 'P': return kP;
    case 'Q': return kQ;
    case 'R': return kR;
    case 'S': return kS;
    case 'T': return kT;
    case 'U': return kU;
    case 'V': return kV;
    case 'W': return kW;
    case 'X': return kX;
    case 'Y': return kY;
    case 'Z': return kZ;
    case ':': return kColon;
    case '+': return kPlus;
    case '-': return kDash;
    case '/': return kSlash;
    case '.': return kDot;
    case '!': return kBang;
    case '?': return kQuest;
    default:  return kFallback;
  }
}

void drawGlyph(SDL_Renderer* renderer, char ch, float x, float y, float scale) {
  const Glyph& glyph = glyphFor(ch);
  for (std::size_t row = 0; row < glyph.size(); ++row) {
    const std::uint8_t bits = glyph[row];
    for (int col = 0; col < 5; ++col) {
      if ((bits & (1u << (4 - col))) == 0u) {
        continue;
      }
      SDL_FRect rect{
          x + static_cast<float>(col) * scale,
          y + static_cast<float>(row) * scale,
          scale,
          scale};
      SDL_RenderFillRectF(renderer, &rect);
    }
  }
}

float textWidth(const std::string& text, float scale) {
  return static_cast<float>(text.size()) * 6.0f * scale;
}

void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, float scale,
              SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  float cursor = x;
  for (char ch : text) {
    drawGlyph(renderer, ch, cursor, y, scale);
    cursor += 6.0f * scale;
  }
}

void drawTextCentered(SDL_Renderer* renderer, const std::string& text, float cx, float y,
                      float scale, SDL_Color color) {
  drawText(renderer, text, cx - textWidth(text, scale) * 0.5f, y, scale, color);
}

// ---------------------------------------------------------------------------
// Audio (SDL_mixer optional)
// ---------------------------------------------------------------------------

#if BRICKBREAKER_HAS_SDL_MIXER
struct AudioState {
  bool initialized{false};
  std::array<Mix_Chunk*, kSoundCount> chunks{};
  std::array<std::int64_t, kSoundCount> lastPlayMs{};
};

AudioState g_audio;

std::int64_t monotonicMs() {
  return static_cast<std::int64_t>(SDL_GetTicks());
}

void onSoundRequested(std::int32_t id, std::int32_t volumePercent) {
  if (!g_audio.initialized) {
    return;
  }
  if (id < 0 || id >= kSoundCount) {
    return;
  }
  Mix_Chunk* chunk = g_audio.chunks[static_cast<std::size_t>(id)];
  if (chunk == nullptr) {
    return;
  }

  const std::int64_t now = monotonicMs();
  if (now - g_audio.lastPlayMs[static_cast<std::size_t>(id)] <
      kSoundCooldownMs[static_cast<std::size_t>(id)]) {
    return;
  }
  g_audio.lastPlayMs[static_cast<std::size_t>(id)] = now;

  const std::int32_t clamped = std::clamp(volumePercent, 0, 100);
  const float gain = kSoundGain[static_cast<std::size_t>(id)];
  const int sdlVol = static_cast<int>(std::round(static_cast<float>(MIX_MAX_VOLUME) *
                                                 (gain * static_cast<float>(clamped) / 100.0f)));
  Mix_VolumeChunk(chunk, std::clamp(sdlVol, 0, MIX_MAX_VOLUME));
  Mix_PlayChannel(-1, chunk, 0);
}

void initAudio() {
  if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
    SDL_Log("Mix_OpenAudio failed: %s", Mix_GetError());
    return;
  }
  Mix_AllocateChannels(16);
  g_audio.initialized = true;

  for (std::size_t i = 0; i < kSoundCount; ++i) {
    const std::string path = joinPath(g_assetDir, kSoundFilenames[i]);
    Mix_Chunk* chunk = Mix_LoadWAV(path.c_str());
    if (chunk == nullptr) {
      SDL_Log("Failed to load %s: %s", path.c_str(), Mix_GetError());
    }
    g_audio.chunks[i] = chunk;
  }

  Sound::setPlayCallback(&onSoundRequested);
}

void shutdownAudio() {
  if (!g_audio.initialized) {
    return;
  }
  Sound::setPlayCallback(nullptr);
  Mix_HaltChannel(-1);
  for (Mix_Chunk* chunk : g_audio.chunks) {
    if (chunk != nullptr) {
      Mix_FreeChunk(chunk);
    }
  }
  g_audio.chunks.fill(nullptr);
  Mix_CloseAudio();
  g_audio.initialized = false;
}
#else
void initAudio() {}
void shutdownAudio() {}
#endif

// ---------------------------------------------------------------------------
// Renderer / game state
// ---------------------------------------------------------------------------

struct Sprites {
  Texture bg;
  Texture bricks;
  Texture paddles;
  Texture paddleLong;
  Texture pills;
  Texture laser;
  Texture bomb;
  Texture balls;
  Texture youLose;
  Texture splash;

  void destroy() {
    destroyTexture(bg);
    destroyTexture(bricks);
    destroyTexture(paddles);
    destroyTexture(paddleLong);
    destroyTexture(pills);
    destroyTexture(laser);
    destroyTexture(bomb);
    destroyTexture(balls);
    destroyTexture(youLose);
    destroyTexture(splash);
  }
};

struct AppState {
  SDL_Window* window{nullptr};
  SDL_Renderer* renderer{nullptr};
  std::unique_ptr<Game> game;
  Sprites sprites{};

  bool running{true};
  bool leftDown{false};
  bool rightDown{false};
  bool mouseDown{false};
  std::int32_t lastMouseBoardX{0};

  std::uint32_t lastTickMs{0};
  std::int32_t ballFrameTick{0};
  std::int32_t pillFrameTick{0};
  std::int32_t flashTick{0};

  // Hold the transient FINISHEDLEVEL / DEATH overlay for a moment before the
  // core advances state (otherwise the overlay only flashes for one frame).
  std::int32_t overlayHoldMs{0};

  std::int32_t currentLevel{1};
  std::int32_t logicalWidth{kInitialWindowWidth};
  std::int32_t logicalHeight{kInitialWindowHeight};
  SDL_FRect boardRect{};
  float boardScale{1.0f};
};

// Map the native 189x195 board into the window with letterboxing under a HUD strip.
void recomputeLayout(AppState& state) {
  int w = 0, h = 0;
  SDL_GetRendererOutputSize(state.renderer, &w, &h);
  state.logicalWidth = std::max(1, w);
  state.logicalHeight = std::max(1, h);

  const float availW = static_cast<float>(state.logicalWidth);
  const float availH = static_cast<float>(state.logicalHeight - kHudHeight);
  const float boardAspect =
      static_cast<float>(kBaseBoardWidth) / static_cast<float>(kBaseBoardHeight);
  const float availAspect = availW / std::max(1.0f, availH);

  float boardW, boardH;
  if (availAspect > boardAspect) {
    boardH = availH;
    boardW = boardH * boardAspect;
  } else {
    boardW = availW;
    boardH = boardW / boardAspect;
  }

  state.boardRect.x = (availW - boardW) * 0.5f;
  state.boardRect.y = static_cast<float>(kHudHeight) + (availH - boardH) * 0.5f;
  state.boardRect.w = boardW;
  state.boardRect.h = boardH;
  state.boardScale = boardW / static_cast<float>(kBaseBoardWidth);
}

SDL_FRect boardRect(const AppState& state, float bx, float by, float bw, float bh) {
  return SDL_FRect{
      state.boardRect.x + bx * state.boardScale,
      state.boardRect.y + by * state.boardScale,
      bw * state.boardScale,
      bh * state.boardScale};
}

// Convert a window x/y (mouse) into native board coords.
std::int32_t windowXToBoard(const AppState& state, int wx) {
  if (state.boardRect.w <= 0.5f) {
    return 0;
  }
  const float relative = (static_cast<float>(wx) - state.boardRect.x) / state.boardRect.w;
  const float clamped = std::clamp(relative, 0.0f, 1.0f);
  return static_cast<std::int32_t>(
      std::round(clamped * static_cast<float>(kBaseBoardWidth - 1)));
}

std::int32_t windowYToBoard(const AppState& state, int wy) {
  if (state.boardRect.h <= 0.5f) {
    return 0;
  }
  const float relative = (static_cast<float>(wy) - state.boardRect.y) / state.boardRect.h;
  const float clamped = std::clamp(relative, 0.0f, 1.0f);
  return static_cast<std::int32_t>(
      std::round(clamped * static_cast<float>(kBaseBoardHeight - 1)));
}

// ---------------------------------------------------------------------------
// Sprite frame helpers
// ---------------------------------------------------------------------------

std::int32_t brickFrame(std::int32_t value) {
  if (value >= Bricks::INDESTRUCTIBLE) return 8;
  if (value >= 8) return 7;
  if (value >= 6) return 6;
  if (value >= 4) return 5;
  if (value >= 2) return 4;
  if (value >= 1) return 3;
  return 8;
}

void drawTexturedSlice(SDL_Renderer* renderer, const Texture& tex, const SDL_Rect& src,
                       const SDL_FRect& dst, std::uint8_t alpha = 255) {
  if (!tex.valid()) {
    return;
  }
  if (alpha != 255) {
    SDL_SetTextureAlphaMod(tex.sdl, alpha);
  }
  SDL_RenderCopyF(renderer, tex.sdl, &src, &dst);
  if (alpha != 255) {
    SDL_SetTextureAlphaMod(tex.sdl, 255);
  }
}

// ---------------------------------------------------------------------------
// Drawing helpers
// ---------------------------------------------------------------------------

void drawFilledRect(SDL_Renderer* renderer, float x, float y, float w, float h,
                    SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  SDL_FRect rect{x, y, w, h};
  SDL_RenderFillRectF(renderer, &rect);
}

void drawCircle(SDL_Renderer* renderer, float cx, float cy, float radius, SDL_Color color) {
  SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
  const int r = std::max(1, static_cast<int>(std::ceil(radius)));
  const float r2 = radius * radius;
  for (int dy = -r; dy <= r; ++dy) {
    const float yy = static_cast<float>(dy);
    for (int dx = -r; dx <= r; ++dx) {
      const float xx = static_cast<float>(dx);
      if (xx * xx + yy * yy <= r2) {
        SDL_RenderDrawPointF(renderer, cx + xx, cy + yy);
      }
    }
  }
}

// ---------------------------------------------------------------------------
// Background
// ---------------------------------------------------------------------------

void drawBackground(AppState& state) {
  // Full-window dark fill.
  SDL_SetRenderDrawColor(state.renderer, 12, 14, 22, 255);
  SDL_RenderClear(state.renderer);

  // Subtle gradient over the board area for depth.
  const int strips = 32;
  for (int i = 0; i < strips; ++i) {
    const float t = static_cast<float>(i) / static_cast<float>(strips);
    SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
    const std::uint8_t r = static_cast<std::uint8_t>(20 + 18.0f * (1.0f - t));
    const std::uint8_t g = static_cast<std::uint8_t>(28 + 24.0f * (1.0f - t));
    const std::uint8_t b = static_cast<std::uint8_t>(48 + 40.0f * (1.0f - t));
    SDL_SetRenderDrawColor(state.renderer, r, g, b, 200);
    SDL_FRect rect{
        state.boardRect.x,
        state.boardRect.y + state.boardRect.h * t,
        state.boardRect.w,
        state.boardRect.h / static_cast<float>(strips) + 1.0f};
    SDL_RenderFillRectF(state.renderer, &rect);
  }

  if (state.sprites.bg.valid()) {
    SDL_SetTextureAlphaMod(state.sprites.bg.sdl, 110);
    SDL_RenderCopyF(state.renderer, state.sprites.bg.sdl, nullptr, &state.boardRect);
    SDL_SetTextureAlphaMod(state.sprites.bg.sdl, 255);
  }
}

// ---------------------------------------------------------------------------
// Drawable game elements
// ---------------------------------------------------------------------------

void drawBricks(AppState& state) {
  const Board& board = state.game->boardRef();
  for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
    for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
      const std::int32_t cell =
          board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
      if (cell <= 0) {
        continue;
      }

      const float bx = static_cast<float>(col * board.tileWidth);
      const float by = static_cast<float>(row * board.tileHeight + board.bricks.amountMoved);
      const float bw = static_cast<float>(std::max(1, board.tileWidth - 1));
      const float bh = static_cast<float>(std::max(1, board.tileHeight - 1));
      const SDL_FRect dst = boardRect(state, bx, by, bw, bh);

      if (state.sprites.bricks.valid()) {
        const std::int32_t frame = brickFrame(cell);
        const int frameH = state.sprites.bricks.height / kBrickFrameRows;
        const SDL_Rect src{0, frame * frameH, state.sprites.bricks.width, frameH};
        drawTexturedSlice(state.renderer, state.sprites.bricks, src, dst);
      } else {
        SDL_Color color;
        if (cell >= Bricks::INDESTRUCTIBLE)
          color = SDL_Color{95, 105, 120, 255};
        else if (cell >= 7)
          color = SDL_Color{242, 114, 44, 255};
        else if (cell >= 5)
          color = SDL_Color{241, 196, 15, 255};
        else if (cell >= 3)
          color = SDL_Color{46, 204, 113, 255};
        else
          color = SDL_Color{66, 165, 245, 255};
        drawFilledRect(state.renderer, dst.x, dst.y, dst.w, dst.h, color);
      }
    }
  }
}

void drawBalls(AppState& state) {
  const Board& board = state.game->boardRef();
  for (const Ball& ball : board.balls) {
    if (!ball.isActive()) {
      continue;
    }
    const float bx = static_cast<float>(ball.x - Ball::RADIUS);
    const float by = static_cast<float>(ball.y - Ball::RADIUS);
    const float size = static_cast<float>(Ball::RADIUS * 2);
    const SDL_FRect dst = boardRect(state, bx, by, size, size);

    if (state.sprites.balls.valid()) {
      const int frameSize = state.sprites.balls.height;
      const int frameCount = std::max(1, state.sprites.balls.width / std::max(1, frameSize));
      const int frame = std::clamp(state.ballFrameTick / 6, 0, frameCount - 1);
      const SDL_Rect src{frame * frameSize, 0, frameSize, frameSize};
      drawTexturedSlice(state.renderer, state.sprites.balls, src, dst);
    } else {
      drawCircle(state.renderer, dst.x + dst.w * 0.5f, dst.y + dst.h * 0.5f,
                 dst.w * 0.55f, SDL_Color{245, 245, 245, 255});
    }
  }
}

void drawAimGuide(AppState& state) {
  const Board& board = state.game->boardRef();
  const Ball& primary = board.balls[0];
  if (!primary.isActive() || !primary.stopped()) {
    return;
  }

  const float dirX = static_cast<float>(primary.aimDx == 0 ? 1 : primary.aimDx);
  const float dirY = -3.0f;
  const float magnitude = std::sqrt(dirX * dirX + dirY * dirY);
  float vx = dirX / std::max(0.0001f, magnitude);
  const float vy = dirY / std::max(0.0001f, magnitude);

  float bx = static_cast<float>(primary.x);
  float by = static_cast<float>(primary.y);
  const float left = static_cast<float>(Ball::RADIUS);
  const float right = static_cast<float>(kBaseBoardWidth - Ball::RADIUS);

  SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
  for (int i = 0; i < 60; ++i) {
    const float nbx = bx + vx * 3.0f;
    const float nby = by + vy * 3.0f;
    if ((nbx <= left && vx < 0.0f) || (nbx >= right && vx > 0.0f)) {
      vx = -vx;
    }
    bx = std::clamp(nbx, left, right);
    by = nby;
    if (by <= 0.0f) {
      break;
    }
    if ((i % 2) == 0) {
      const SDL_FRect dot = boardRect(state, bx - 0.5f, by - 0.5f, 1.0f, 1.0f);
      SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 140);
      SDL_RenderFillRectF(state.renderer, &dot);
    }
  }
}

void drawBomb(AppState& state) {
  const Board& board = state.game->boardRef();
  if (!board.bomb.isActive()) {
    return;
  }
  const float bw = static_cast<float>(std::max(1, board.bomb.width));
  const float bh = static_cast<float>(std::max(1, board.bomb.height));
  const SDL_FRect dst = boardRect(state,
                                  static_cast<float>(board.bomb.x),
                                  static_cast<float>(board.bomb.y),
                                  bw, bh);
  if (state.sprites.bomb.valid()) {
    SDL_RenderCopyF(state.renderer, state.sprites.bomb.sdl, nullptr, &dst);
  } else {
    drawFilledRect(state.renderer, dst.x, dst.y, dst.w, dst.h,
                   SDL_Color{227, 74, 51, 255});
  }
}

void drawLasers(AppState& state) {
  const Board& board = state.game->boardRef();
  for (const Bullet& laser : board.lasers) {
    if (!laser.isActive()) {
      continue;
    }
    const float bw = static_cast<float>(std::max(1, laser.width));
    const float bh = static_cast<float>(std::max(1, laser.height));
    const SDL_FRect dst = boardRect(state,
                                    static_cast<float>(laser.x),
                                    static_cast<float>(laser.y),
                                    bw, bh);
    if (state.sprites.laser.valid()) {
      SDL_RenderCopyF(state.renderer, state.sprites.laser.sdl, nullptr, &dst);
    } else {
      drawFilledRect(state.renderer, dst.x, dst.y, dst.w, dst.h,
                     SDL_Color{255, 80, 0, 255});
    }
  }
}

const char* pillShortLabel(std::int32_t bonusType) {
  switch (bonusType) {
    case Pill::LNG: return "LNG";
    case Pill::GUN: return "GUN";
    case Pill::SHR: return "SHR";
    case Pill::SLW: return "SLW";
    case Pill::NEW: return "NEW";
    case Pill::FLP: return "FLP";
    case Pill::CAT: return "CAT";
    case Pill::LAS: return "LAS";
    case Pill::LIF: return "LIF";
    case Pill::WRP: return "WRP";
    case Pill::BMB: return "BMB";
    default: return "";
  }
}

void drawPills(AppState& state) {
  const Board& board = state.game->boardRef();
  for (const Pill& pill : board.pills.pool()) {
    if (!pill.isActive()) {
      continue;
    }
    const float bw = static_cast<float>(pill.width);
    const float bh = static_cast<float>(pill.height);
    const SDL_FRect dst = boardRect(state,
                                    static_cast<float>(pill.x),
                                    static_cast<float>(pill.y),
                                    bw, bh);

    if (state.sprites.pills.valid()) {
      const int frameW = state.sprites.pills.width / 4;
      const int frame = (state.pillFrameTick / 6) % 4;
      const SDL_Rect src{frame * frameW, 0, frameW, state.sprites.pills.height};
      drawTexturedSlice(state.renderer, state.sprites.pills, src, dst);
    } else {
      drawFilledRect(state.renderer, dst.x, dst.y, dst.w, dst.h,
                     SDL_Color{163, 230, 53, 255});
    }

    const char* label = pillShortLabel(pill.bonusType);
    if (label[0] != '\0') {
      // Pick a scale that fits the pill width.
      const float maxScale = dst.w / (6.0f * 3.0f + 2.0f);
      const float scale = std::max(1.0f, std::min(maxScale, state.boardScale * 0.55f));
      const float labelW = textWidth(label, scale);
      const float labelX = dst.x + (dst.w - labelW) * 0.5f;
      const float labelY = dst.y + (dst.h - 7.0f * scale) * 0.5f;
      // soft shadow for legibility against the pill sprite
      drawText(state.renderer, label, labelX + 1.0f, labelY + 1.0f, scale,
               SDL_Color{0, 0, 0, 180});
      drawText(state.renderer, label, labelX, labelY, scale,
               SDL_Color{20, 24, 8, 255});
    }
  }
}

void drawPaddle(AppState& state) {
  const Board& board = state.game->boardRef();
  const float bw = static_cast<float>(std::max(1, board.paddle.width));
  const float bh = static_cast<float>(std::max(1, board.paddle.height));
  const SDL_FRect dst = boardRect(state,
                                  static_cast<float>(board.paddle.x),
                                  static_cast<float>(board.paddle.y),
                                  bw, bh);

  if (board.paddle.mode == Paddle::MODE_LONG && state.sprites.paddleLong.valid()) {
    SDL_RenderCopyF(state.renderer, state.sprites.paddleLong.sdl, nullptr, &dst);
  } else if (state.sprites.paddles.valid()) {
    const int row = (board.paddle.mode == Paddle::MODE_GUN)   ? 2
                  : (board.paddle.mode == Paddle::MODE_LASER) ? 1
                                                              : 0;
    const int frameH = state.sprites.paddles.height / 3;
    const SDL_Rect src{0, row * frameH, state.sprites.paddles.width, frameH};
    drawTexturedSlice(state.renderer, state.sprites.paddles, src, dst);
  } else {
    SDL_Color color = SDL_Color{32, 201, 151, 255};
    if (board.paddle.mode == Paddle::MODE_LASER) color = SDL_Color{231, 76, 60, 255};
    else if (board.paddle.mode == Paddle::MODE_GUN) color = SDL_Color{241, 196, 15, 255};
    drawFilledRect(state.renderer, dst.x, dst.y, dst.w, dst.h, color);
  }
}

void drawBoardFrame(AppState& state) {
  // Subtle border around the playing field.
  SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 28);
  SDL_FRect frame = state.boardRect;
  SDL_RenderDrawRectF(state.renderer, &frame);
}

// ---------------------------------------------------------------------------
// HUD & overlays
// ---------------------------------------------------------------------------

void drawHud(AppState& state) {
  const Game& game = *state.game;

  // HUD bar background.
  drawFilledRect(state.renderer, 0.0f, 0.0f,
                 static_cast<float>(state.logicalWidth),
                 static_cast<float>(kHudHeight),
                 SDL_Color{6, 8, 14, 255});
  drawFilledRect(state.renderer, 0.0f, static_cast<float>(kHudHeight - 2),
                 static_cast<float>(state.logicalWidth), 2.0f,
                 SDL_Color{32, 201, 151, 200});

  const float scale = std::max(2.0f, std::min(3.0f, state.boardScale * 0.65f));
  const float baseY = std::round((kHudHeight - 7.0f * scale) * 0.5f);

  char buffer[128];

  // Left cluster: SCORE  HIGH
  std::snprintf(buffer, sizeof(buffer), "SCORE %d", game.score);
  drawText(state.renderer, buffer, 12.0f, baseY, scale, SDL_Color{255, 255, 255, 255});
  const float scoreEnd = 12.0f + textWidth(buffer, scale);

  std::snprintf(buffer, sizeof(buffer), "HIGH %d", game.highScore);
  drawText(state.renderer, buffer, scoreEnd + 12.0f, baseY, scale,
           SDL_Color{200, 230, 255, 255});

  // Right cluster: LIVES  (and AMMO if any)
  std::snprintf(buffer, sizeof(buffer), "LIVES %d", std::max(0, game.lives));
  const float livesWidth = textWidth(buffer, scale);
  float rightX = static_cast<float>(state.logicalWidth) - livesWidth - 12.0f;
  drawText(state.renderer, buffer, rightX, baseY, scale, SDL_Color{120, 240, 200, 255});

  if (game.ammoCount() > 0) {
    std::snprintf(buffer, sizeof(buffer), "AMMO %d", game.ammoCount());
    const float ammoWidth = textWidth(buffer, scale);
    rightX -= ammoWidth + 12.0f;
    drawText(state.renderer, buffer, rightX, baseY, scale, SDL_Color{255, 170, 80, 255});
  }

  // Center cluster: LV (only if there's room).
  if (game.superLevelCount() > 0) {
    std::snprintf(buffer, sizeof(buffer), "LV %d+%d", state.currentLevel, game.superLevelCount());
  } else {
    std::snprintf(buffer, sizeof(buffer), "LV %d", state.currentLevel);
  }
  const float centerWidth = textWidth(buffer, scale);
  const float cx = static_cast<float>(state.logicalWidth) * 0.5f;
  if (cx - centerWidth * 0.5f > scoreEnd + 24.0f &&
      cx + centerWidth * 0.5f < rightX - 12.0f) {
    drawTextCentered(state.renderer, buffer, cx, baseY, scale,
                     SDL_Color{255, 220, 120, 255});
  }
}

void drawOverlay(AppState& state, const std::vector<std::string>& lines, SDL_Color text,
                 SDL_Color box) {
  // Dim board area
  SDL_SetRenderDrawBlendMode(state.renderer, SDL_BLENDMODE_BLEND);
  SDL_SetRenderDrawColor(state.renderer, 0, 0, 0, 160);
  SDL_RenderFillRectF(state.renderer, &state.boardRect);

  const float scaleTitle = std::max(3.0f, state.boardScale * 1.4f);
  const float scaleBody = std::max(2.0f, state.boardScale * 0.85f);

  float totalHeight = 0.0f;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    totalHeight += (i == 0 ? 8.0f * scaleTitle : 8.0f * scaleBody) + 6.0f;
  }
  const float cx = state.boardRect.x + state.boardRect.w * 0.5f;
  float cy = state.boardRect.y + (state.boardRect.h - totalHeight) * 0.5f;

  // Box behind text
  const float widest =
      std::max(textWidth(lines.front(), scaleTitle),
               (lines.size() > 1) ? textWidth(lines[1], scaleBody) : 0.0f);
  SDL_FRect outer{cx - widest * 0.5f - 24.0f, cy - 16.0f,
                  widest + 48.0f, totalHeight + 32.0f};
  SDL_SetRenderDrawColor(state.renderer, box.r, box.g, box.b, box.a);
  SDL_RenderFillRectF(state.renderer, &outer);
  SDL_SetRenderDrawColor(state.renderer, 255, 255, 255, 60);
  SDL_RenderDrawRectF(state.renderer, &outer);

  for (std::size_t i = 0; i < lines.size(); ++i) {
    const float scale = (i == 0) ? scaleTitle : scaleBody;
    drawTextCentered(state.renderer, lines[i], cx, cy, scale, text);
    cy += 8.0f * scale + 6.0f;
  }
}

void drawStateOverlays(AppState& state) {
  const Game& game = *state.game;

  if (game.paused) {
    drawOverlay(state,
                {"PAUSED", "PRESS P OR SPACE TO RESUME"},
                SDL_Color{255, 240, 180, 255},
                SDL_Color{12, 16, 28, 230});
    return;
  }
  if (game.gameState() == Game::STATE_NONE ||
      game.gameState() == Game::STATE_GAMEOVER) {
    drawOverlay(state,
                {"GAME OVER", "PRESS R TO RESTART"},
                SDL_Color{255, 120, 120, 255},
                SDL_Color{40, 8, 14, 235});
    return;
  }
  if (game.gameState() == Game::STATE_DEATH) {
    drawOverlay(state,
                {"BALL LOST", "GET READY"},
                SDL_Color{255, 220, 180, 255},
                SDL_Color{36, 16, 8, 230});
    return;
  }
  if (game.gameState() == Game::STATE_FINISHEDLEVEL) {
    drawOverlay(state,
                {"LEVEL CLEAR", "ADVANCING"},
                SDL_Color{180, 255, 200, 255},
                SDL_Color{8, 36, 24, 235});
    return;
  }

  // When the ball is sitting on the paddle waiting for a launch, hint that.
  const Board& board = game.boardRef();
  const Ball& primary = board.balls[0];
  if (game.gameState() == Game::STATE_PLAYING && primary.isActive() && primary.stopped()) {
    const float scale = std::max(1.5f, state.boardScale * 0.55f);
    const std::string hint = "PRESS SPACE TO LAUNCH";
    const float w = textWidth(hint, scale);
    const float cx = state.boardRect.x + state.boardRect.w * 0.5f;
    const float cy = state.boardRect.y + state.boardRect.h - 8.0f * scale - 12.0f;
    // soft background to keep the hint readable over bg.png
    drawFilledRect(state.renderer, cx - w * 0.5f - 8.0f, cy - 4.0f, w + 16.0f,
                   8.0f * scale + 6.0f, SDL_Color{0, 0, 0, 140});
    drawTextCentered(state.renderer, hint, cx, cy, scale,
                     SDL_Color{255, 240, 180, 255});
  }
}

// ---------------------------------------------------------------------------
// Render
// ---------------------------------------------------------------------------

void renderFrame(AppState& state) {
  recomputeLayout(state);
  drawBackground(state);
  drawBricks(state);
  drawAimGuide(state);
  drawPills(state);
  drawLasers(state);
  drawBomb(state);
  drawBalls(state);
  drawPaddle(state);
  drawBoardFrame(state);
  drawHud(state);
  drawStateOverlays(state);
  SDL_RenderPresent(state.renderer);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------

void sendKey(Game& game, std::int32_t code) {
  KeyEvent event{};
  event.keycode = code;
  game.keyDown(event);
}

void handleEvents(AppState& state) {
  SDL_Event event{};
  while (SDL_PollEvent(&event) != 0) {
    switch (event.type) {
      case SDL_QUIT:
        state.running = false;
        break;
      case SDL_WINDOWEVENT:
        if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
            event.window.event == SDL_WINDOWEVENT_RESIZED) {
          recomputeLayout(state);
        }
        break;
      case SDL_KEYDOWN: {
        const SDL_Keycode k = event.key.keysym.sym;
        // Ignore key-repeat for stateful toggles, but always honour the
        // instantaneous arrow key nudge below so even quick taps move the
        // paddle visibly.
        const bool isRepeat = event.key.repeat != 0;
        if (!isRepeat && (k == SDLK_ESCAPE || k == SDLK_q)) {
          state.running = false;
        } else if (k == SDLK_LEFT || k == SDLK_a) {
          state.leftDown = true;
          Board& board = state.game->boardRef();
          if (Paddle* paddle = board.paddleRef(); paddle != nullptr) {
            paddle->move(-2, board);
          }
        } else if (k == SDLK_RIGHT || k == SDLK_d) {
          state.rightDown = true;
          Board& board = state.game->boardRef();
          if (Paddle* paddle = board.paddleRef(); paddle != nullptr) {
            paddle->move(2, board);
          }
        } else if (!isRepeat && k == SDLK_SPACE) {
          sendKey(*state.game, Game::KEY_SPACE);
        } else if (!isRepeat && k == SDLK_p) {
          sendKey(*state.game, Game::KEY_P);
        } else if (!isRepeat && k == SDLK_r) {
          state.game->newGame(1);
          state.currentLevel = 1;
        }
        break;
      }
      case SDL_KEYUP: {
        const SDL_Keycode k = event.key.keysym.sym;
        if (k == SDLK_LEFT || k == SDLK_a) {
          state.leftDown = false;
        } else if (k == SDLK_RIGHT || k == SDLK_d) {
          state.rightDown = false;
        }
        break;
      }
      case SDL_MOUSEBUTTONDOWN:
        if (event.button.button == SDL_BUTTON_LEFT) {
          state.mouseDown = true;
          PointerEvent ev{};
          ev.type = PointerType::kStart;
          ev.x = windowXToBoard(state, event.button.x);
          ev.y = windowYToBoard(state, event.button.y);
          state.lastMouseBoardX = ev.x;
          state.game->touchEvent(ev);
        }
        break;
      case SDL_MOUSEMOTION:
        if (state.mouseDown) {
          PointerEvent ev{};
          ev.type = PointerType::kMove;
          ev.x = windowXToBoard(state, event.motion.x);
          ev.y = windowYToBoard(state, event.motion.y);
          state.lastMouseBoardX = ev.x;
          state.game->touchEvent(ev);
        }
        break;
      case SDL_MOUSEBUTTONUP:
        if (event.button.button == SDL_BUTTON_LEFT) {
          PointerEvent ev{};
          ev.type = PointerType::kEnd;
          ev.x = windowXToBoard(state, event.button.x);
          ev.y = windowYToBoard(state, event.button.y);
          state.game->touchEvent(ev);
          // Tap-to-shoot: tiny drag distance treated as tap.
          if (state.mouseDown && std::abs(ev.x - state.lastMouseBoardX) < 2) {
            PointerEvent tap{};
            tap.type = PointerType::kTap;
            tap.x = ev.x;
            tap.y = ev.y;
            state.game->touchEvent(tap);
          }
          state.mouseDown = false;
        }
        break;
      case SDL_FINGERDOWN: {
        PointerEvent ev{};
        ev.type = PointerType::kStart;
        ev.x = static_cast<std::int32_t>(event.tfinger.x * kBaseBoardWidth);
        ev.y = static_cast<std::int32_t>(event.tfinger.y * kBaseBoardHeight);
        state.game->touchEvent(ev);
        break;
      }
      case SDL_FINGERMOTION: {
        PointerEvent ev{};
        ev.type = PointerType::kMove;
        ev.x = static_cast<std::int32_t>(event.tfinger.x * kBaseBoardWidth);
        ev.y = static_cast<std::int32_t>(event.tfinger.y * kBaseBoardHeight);
        state.game->touchEvent(ev);
        break;
      }
      case SDL_FINGERUP: {
        PointerEvent ev{};
        ev.type = PointerType::kEnd;
        ev.x = static_cast<std::int32_t>(event.tfinger.x * kBaseBoardWidth);
        ev.y = static_cast<std::int32_t>(event.tfinger.y * kBaseBoardHeight);
        state.game->touchEvent(ev);
        break;
      }
      default:
        break;
    }
  }
}

void applyHeldInput(AppState& state) {
  Board& board = state.game->boardRef();
  Paddle* paddle = board.paddleRef();
  if (paddle == nullptr) {
    return;
  }
  if (state.leftDown && !state.rightDown) {
    paddle->move(-2, board);
  } else if (state.rightDown && !state.leftDown) {
    paddle->move(2, board);
  }
}

// ---------------------------------------------------------------------------
// Game tick
// ---------------------------------------------------------------------------

void gameTick(AppState& state) {
  const std::uint32_t now = SDL_GetTicks();
  std::uint32_t elapsed = now - state.lastTickMs;
  if (elapsed > 33u) {
    elapsed = 33u;
  }
  state.lastTickMs = now;
  const std::int32_t e = static_cast<std::int32_t>(elapsed);

  // Drain any held overlay (LEVEL CLEAR / BALL LOST) before doing anything
  // that might advance the state machine past it.
  if (state.overlayHoldMs > 0) {
    state.overlayHoldMs = std::max(0, state.overlayHoldMs - e);
    return;
  }

  bool startedHold = false;
  if (!state.game->paused && state.game->gameState() == Game::STATE_PLAYING) {
    applyHeldInput(state);
    state.game->applyInput();  // apply pending mouse/touch drag to the paddle
    state.game->boardRef().update(e);

    const std::int32_t s = state.game->gameState();
    if (s == Game::STATE_FINISHEDLEVEL) {
      ++state.currentLevel;
      if (state.currentLevel > Bricks::getNumLevels()) {
        state.currentLevel = 1;
      }
      state.overlayHoldMs = 700;
      startedHold = true;
    } else if (s == Game::STATE_DEATH) {
      state.overlayHoldMs = 700;
      startedHold = true;
    }

    state.ballFrameTick = (state.ballFrameTick + 1) % 240;
    state.pillFrameTick = (state.pillFrameTick + 1) % 240;
    state.flashTick = (state.flashTick + 1) & 0xff;
  }

  // Only let the engine progress out of FINISHEDLEVEL/DEATH after the hold
  // expires so the overlay is visible for at least one frame.
  if (!startedHold) {
    state.game->advanceState();
  }
}

void mainLoopOnce(void* opaque) {
  AppState& state = *static_cast<AppState*>(opaque);

  handleEvents(state);

  if (!state.running) {
#ifdef __EMSCRIPTEN__
    emscripten_cancel_main_loop();
#endif
    return;
  }

  gameTick(state);
  renderFrame(state);
}

// ---------------------------------------------------------------------------
// Asset & game initialization
// ---------------------------------------------------------------------------

bool tryLoadLevels(const std::string& path) {
  auto bytes = readBinaryFile(path);
  if (!bytes.has_value() || bytes->empty()) {
    return false;
  }
  Bricks::setLevelData(bytes->data(), static_cast<std::int32_t>(bytes->size()));
  return true;
}

void loadAllSprites(AppState& state) {
  if (g_assetDir.empty()) {
    return;
  }
  state.sprites.bg = loadTexture(state.renderer, joinPath(g_assetDir, "ui/bg.png"));
  state.sprites.bricks = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/bricks.png"));
  state.sprites.paddles = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/paddles.png"));
  state.sprites.paddleLong =
      loadTexture(state.renderer, joinPath(g_assetDir, "sprites/paddlelong.png"));
  state.sprites.pills = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/pills.png"));
  state.sprites.laser = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/laser.png"));
  state.sprites.bomb = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/bomb.png"));
  state.sprites.balls = loadTexture(state.renderer, joinPath(g_assetDir, "sprites/balls.png"));
  state.sprites.youLose = loadTexture(state.renderer, joinPath(g_assetDir, "ui/you_lose.png"));
  state.sprites.splash = loadTexture(state.renderer, joinPath(g_assetDir, "ui/splash.png"));
}

}  // namespace

int main(int argc, char** argv) {
  const char* levelsPathArg = nullptr;
  const char* assetDirArg = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--levels" && i + 1 < argc) {
      levelsPathArg = argv[++i];
    } else if (arg == "--assets" && i + 1 < argc) {
      assetDirArg = argv[++i];
    } else if (levelsPathArg == nullptr) {
      levelsPathArg = argv[i];
    }
  }

  resolveAssetDir(assetDirArg);

  if (levelsPathArg != nullptr) {
    if (!tryLoadLevels(levelsPathArg)) {
      std::cerr << "Warning: failed to load levels file: " << levelsPathArg << "\n";
    }
  } else if (!g_assetDir.empty()) {
    if (!tryLoadLevels(joinPath(g_assetDir, "levels.bin"))) {
      std::cerr << "Warning: failed to load levels.bin from " << g_assetDir << "\n";
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
    std::cerr << "SDL_Init failed: " << SDL_GetError() << "\n";
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
      "BrickBreaker",
      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
      kInitialWindowWidth, kInitialWindowHeight,
      SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
  if (window == nullptr) {
    std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << "\n";
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(
      window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << "\n";
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

  AppState state{};
  state.window = window;
  state.renderer = renderer;
  state.game = std::make_unique<Game>();
  state.game->setSchedulingEnabled(false);
  state.game->setViewport(kBaseBoardWidth, kBaseBoardHeight);
  state.game->resize();
  state.game->newGame(1);
  state.lastTickMs = SDL_GetTicks();

  loadAllSprites(state);
  initAudio();
  recomputeLayout(state);

#ifdef __EMSCRIPTEN__
  emscripten_set_main_loop_arg(&mainLoopOnce, &state, 0, 1);
#else
  while (state.running) {
    mainLoopOnce(&state);
    SDL_Delay(1);
  }
#endif

  shutdownAudio();
  state.sprites.destroy();
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}

#else  // BRICKBREAKER_HAS_SDL

int main() {
  std::cerr << "SDL2 headers not found at compile time; SDL frontend is unavailable.\n";
  return 1;
}

#endif  // BRICKBREAKER_HAS_SDL
