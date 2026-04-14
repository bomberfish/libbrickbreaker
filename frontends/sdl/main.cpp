#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <cmath>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <SDL2/SDL.h>

#include "libbrickbreaker/libbrickbreaker.hpp"

namespace {

using namespace libbrickbreaker;

constexpr int kWindowWidth = 756;
constexpr int kWindowHeight = 780;
constexpr std::int32_t kFrameMs = 16;

SDL_FRect makeRect(float x, float y, float w, float h) {
  SDL_FRect rect{};
  rect.x = x;
  rect.y = y;
  rect.w = w;
  rect.h = h;
  return rect;
}

void setColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a = 255) {
  SDL_SetRenderDrawColor(renderer, r, g, b, a);
}

void drawFilledRect(SDL_Renderer* renderer, const SDL_FRect& rect) {
  SDL_RenderFillRectF(renderer, &rect);
}

const std::array<const char*, 7>* glyphRows(char ch) {
  static const std::array<const char*, 7> kUnknown = {
      "01110", "10001", "00010", "00100", "00100", "00000", "00100"};
  static const std::array<const char*, 7> kA = {
      "01110", "10001", "10001", "11111", "10001", "10001", "10001"};
  static const std::array<const char*, 7> kB = {
      "11110", "10001", "10001", "11110", "10001", "10001", "11110"};
  static const std::array<const char*, 7> kC = {
      "01110", "10001", "10000", "10000", "10000", "10001", "01110"};
  static const std::array<const char*, 7> kF = {
      "11111", "10000", "10000", "11110", "10000", "10000", "10000"};
  static const std::array<const char*, 7> kG = {
      "01110", "10001", "10000", "10111", "10001", "10001", "01110"};
  static const std::array<const char*, 7> kH = {
      "10001", "10001", "10001", "11111", "10001", "10001", "10001"};
  static const std::array<const char*, 7> kI = {
      "11111", "00100", "00100", "00100", "00100", "00100", "11111"};
  static const std::array<const char*, 7> kK = {
      "10001", "10010", "10100", "11000", "10100", "10010", "10001"};
  static const std::array<const char*, 7> kL = {
      "10000", "10000", "10000", "10000", "10000", "10000", "11111"};
  static const std::array<const char*, 7> kM = {
      "10001", "11011", "10101", "10101", "10001", "10001", "10001"};
  static const std::array<const char*, 7> kN = {
      "10001", "10001", "11001", "10101", "10011", "10001", "10001"};
  static const std::array<const char*, 7> kO = {
      "01110", "10001", "10001", "10001", "10001", "10001", "01110"};
  static const std::array<const char*, 7> kP = {
      "11110", "10001", "10001", "11110", "10000", "10000", "10000"};
  static const std::array<const char*, 7> kR = {
      "11110", "10001", "10001", "11110", "10100", "10010", "10001"};
  static const std::array<const char*, 7> kS = {
      "01111", "10000", "10000", "01110", "00001", "00001", "11110"};
  static const std::array<const char*, 7> kT = {
      "11111", "00100", "00100", "00100", "00100", "00100", "00100"};
  static const std::array<const char*, 7> kU = {
      "10001", "10001", "10001", "10001", "10001", "10001", "01110"};
  static const std::array<const char*, 7> kW = {
      "10001", "10001", "10001", "10101", "10101", "10101", "01010"};

  switch (std::toupper(static_cast<unsigned char>(ch))) {
    case 'A':
      return &kA;
    case 'B':
      return &kB;
    case 'C':
      return &kC;
    case 'F':
      return &kF;
    case 'G':
      return &kG;
    case 'H':
      return &kH;
    case 'I':
      return &kI;
    case 'K':
      return &kK;
    case 'L':
      return &kL;
    case 'M':
      return &kM;
    case 'N':
      return &kN;
    case 'O':
      return &kO;
    case 'P':
      return &kP;
    case 'R':
      return &kR;
    case 'S':
      return &kS;
    case 'T':
      return &kT;
    case 'U':
      return &kU;
    case 'W':
      return &kW;
    default:
      return &kUnknown;
  }
}

void drawGlyph(SDL_Renderer* renderer, char ch, float x, float y, float scale) {
  const auto* rows = glyphRows(ch);
  for (std::size_t row = 0; row < rows->size(); ++row) {
    const char* line = (*rows)[row];
    for (std::int32_t col = 0; col < 5; ++col) {
      if (line[col] != '1') {
        continue;
      }
      drawFilledRect(renderer, makeRect(x + col * scale, y + static_cast<float>(row) * scale, scale, scale));
    }
  }
}

void drawText(SDL_Renderer* renderer, const std::string& text, float x, float y, float scale) {
  float cursor = x;
  for (char ch : text) {
    if (ch == ' ') {
      cursor += 3.0f * scale;
      continue;
    }
    drawGlyph(renderer, ch, cursor, y, scale);
    cursor += 6.0f * scale;
  }
}

void drawBasicScene(SDL_Renderer* renderer, const Game& game) {
  const Board& board = game.boardRef();
  const float scaleX = static_cast<float>(kWindowWidth) / static_cast<float>(std::max(1, board.width));
  const float scaleY = static_cast<float>(kWindowHeight) / static_cast<float>(std::max(1, board.height));

  setColor(renderer, 14, 18, 28);
  SDL_RenderClear(renderer);

  for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
    for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
      const std::int32_t cell = board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
      if (cell <= 0) {
        continue;
      }

      if (cell >= Bricks::INDESTRUCTIBLE) {
        setColor(renderer, 95, 105, 120);
      } else if (cell >= 7) {
        setColor(renderer, 242, 114, 44);
      } else if (cell >= 5) {
        setColor(renderer, 241, 196, 15);
      } else if (cell >= 3) {
        setColor(renderer, 46, 204, 113);
      } else {
        setColor(renderer, 66, 165, 245);
      }

      const float x = static_cast<float>(col * board.tileWidth) * scaleX;
      const float y = static_cast<float>(row * board.tileHeight + board.bricks.amountMoved) * scaleY;
      const float w = static_cast<float>(board.tileWidth - 1) * scaleX;
      const float h = static_cast<float>(board.tileHeight - 1) * scaleY;
      drawFilledRect(renderer, makeRect(x, y, w, h));
    }
  }

  setColor(renderer, 245, 245, 245);
  for (const Ball& ball : board.balls) {
    if (!ball.isActive()) {
      continue;
    }
    const float x = static_cast<float>(ball.x - Ball::RADIUS) * scaleX;
    const float y = static_cast<float>(ball.y - Ball::RADIUS) * scaleY;
    const float size = static_cast<float>(Ball::RADIUS * 2 + 1) * scaleX;
    drawFilledRect(renderer, makeRect(x, y, size, size));
  }

  const Ball& primaryBall = board.balls[0];
  if (primaryBall.isActive() && primaryBall.stopped()) {
    const float startX = static_cast<float>(primaryBall.x) * scaleX;
    const float startY = static_cast<float>(primaryBall.y) * scaleY;
    const float dirX = static_cast<float>(primaryBall.aimDx == 0 ? 1 : primaryBall.aimDx);
    const float dirY = -3.0f;
    const float magnitude = std::sqrt(dirX * dirX + dirY * dirY);
    float vx = dirX / std::max(0.0001f, magnitude);
    const float vy = dirY / std::max(0.0001f, magnitude);

    float x = startX;
    float y = startY;
    const float left = 0.0f;
    const float right = static_cast<float>(kWindowWidth - 1);
    const float top = 0.0f;

    setColor(renderer, 255, 255, 255, 180);
    for (int i = 0; i < 120; ++i) {
      const float nextX = x + vx * 8.0f;
      const float nextY = y + vy * 8.0f;

      if ((nextX <= left && vx < 0.0f) || (nextX >= right && vx > 0.0f)) {
        vx = -vx;
      }

      x = std::clamp(nextX, left, right);
      y = nextY;
      if (y <= top) {
        break;
      }

      if ((i % 2) == 0) {
        SDL_RenderDrawPointF(renderer, x, y);
        SDL_RenderDrawPointF(renderer, x + 1.0f, y);
      }
    }
  }

  if (board.bomb.isActive()) {
    setColor(renderer, 227, 74, 51);
    const float x = static_cast<float>(board.bomb.x) * scaleX;
    const float y = static_cast<float>(board.bomb.y) * scaleY;
    const float w = static_cast<float>(std::max(1, board.bomb.width)) * scaleX;
    const float h = static_cast<float>(std::max(1, board.bomb.height)) * scaleY;
    drawFilledRect(renderer, makeRect(x, y, w, h));
  }

  setColor(renderer, 255, 77, 0);
  for (const Bullet& laser : board.lasers) {
    if (!laser.isActive()) {
      continue;
    }
    const float x = static_cast<float>(laser.x) * scaleX;
    const float y = static_cast<float>(laser.y) * scaleY;
    const float w = static_cast<float>(std::max(1, laser.width)) * scaleX;
    const float h = static_cast<float>(std::max(1, laser.height)) * scaleY;
    drawFilledRect(renderer, makeRect(x, y, w, h));
  }

  setColor(renderer, 163, 230, 53);
  for (const Pill& pill : board.pills.pool()) {
    if (!pill.isActive()) {
      continue;
    }
    const float x = static_cast<float>(pill.x) * scaleX;
    const float y = static_cast<float>(pill.y) * scaleY;
    const float w = static_cast<float>(pill.width) * scaleX;
    const float h = static_cast<float>(pill.height) * scaleY;
    drawFilledRect(renderer, makeRect(x, y, w, h));

    std::string label = pill.label;
    for (char& ch : label) {
      ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    }
    if (!label.empty()) {
      const float textScale = std::max(1.0f, 0.8f * std::min(scaleX, scaleY));
      const float textWidth = static_cast<float>(label.size()) * 6.0f * textScale;
      const float textX = x + (w - textWidth) * 0.5f;
      const float textY = y - (8.0f * textScale) - 2.0f;

      setColor(renderer, 0, 0, 0, 200);
      drawFilledRect(renderer, makeRect(textX - 2.0f, textY - 1.0f, textWidth + 4.0f, 8.0f * textScale));
      setColor(renderer, 255, 255, 255);
      drawText(renderer, label, textX, textY, textScale);
      setColor(renderer, 163, 230, 53);
    }
  }

  setColor(renderer, 32, 201, 151);
  const float paddleX = static_cast<float>(board.paddle.x) * scaleX;
  const float paddleY = static_cast<float>(board.paddle.y) * scaleY;
  const float paddleW = static_cast<float>(board.paddle.width) * scaleX;
  const float paddleH = static_cast<float>(std::max(1, board.paddle.height)) * scaleY;
  drawFilledRect(renderer, makeRect(paddleX, paddleY, paddleW, paddleH));

  SDL_RenderPresent(renderer);
}

}  // namespace

int main(int argc, char** argv) {
  const char* levelsPath = nullptr;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg == "--levels" && i + 1 < argc) {
      levelsPath = argv[++i];
    } else if (levelsPath == nullptr) {
      levelsPath = argv[i];
    }
  }

  if (levelsPath != nullptr) {
    std::ifstream input(levelsPath, std::ios::binary);
    if (input.good()) {
      std::vector<std::uint8_t> bytes(
          (std::istreambuf_iterator<char>(input)),
          std::istreambuf_iterator<char>());
      if (!bytes.empty()) {
        Bricks::setLevelData(bytes.data(), static_cast<std::int32_t>(bytes.size()));
      } else {
        std::cerr << "Warning: levels file is empty: " << levelsPath << "\n";
      }
    } else {
      std::cerr << "Warning: failed to load levels file: " << levelsPath << "\n";
    }
  }

  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    return 1;
  }

  SDL_Window* window = SDL_CreateWindow(
      "BrickBreaker SDL",
      SDL_WINDOWPOS_CENTERED,
      SDL_WINDOWPOS_CENTERED,
      kWindowWidth,
      kWindowHeight,
      SDL_WINDOW_SHOWN);
  if (window == nullptr) {
    SDL_Quit();
    return 1;
  }

  SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == nullptr) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  Game game;
  game.setSchedulingEnabled(false);
  game.setViewport(Board::BASE_WIDTH, Board::BASE_HEIGHT);
  game.resize();
  game.newGame(1);

  bool running = true;
  bool leftDown = false;
  bool rightDown = false;

  std::uint32_t lastTick = SDL_GetTicks();

  while (running && game.gameState() != Game::STATE_NONE) {
    SDL_Event event{};
    while (SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
        case SDL_KEYDOWN:
          if (event.key.repeat != 0) {
            break;
          }
          if (event.key.keysym.sym == SDLK_ESCAPE || event.key.keysym.sym == SDLK_q) {
            running = false;
          } else if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a) {
            leftDown = true;
          } else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) {
            rightDown = true;
          } else if (event.key.keysym.sym == SDLK_SPACE) {
            game.boardRef().shoot();
          } else if (event.key.keysym.sym == SDLK_p) {
            game.pause();
          }
          break;
        case SDL_KEYUP:
          if (event.key.keysym.sym == SDLK_LEFT || event.key.keysym.sym == SDLK_a) {
            leftDown = false;
          } else if (event.key.keysym.sym == SDLK_RIGHT || event.key.keysym.sym == SDLK_d) {
            rightDown = false;
          }
          break;
        default:
          break;
      }
    }

    Board& board = game.boardRef();
    Paddle* paddle = board.paddleRef();
    if (paddle != nullptr) {
      if (leftDown && !rightDown) {
        paddle->move(-2, board);
      } else if (rightDown && !leftDown) {
        paddle->move(2, board);
      }
    }

    const std::uint32_t now = SDL_GetTicks();
    std::uint32_t elapsed = now - lastTick;
    if (elapsed > 33u) {
      elapsed = 33u;
    }
    lastTick = now;

    if (!game.paused) {
      game.boardRef().update(static_cast<std::int32_t>(elapsed));
      game.advanceState();
    }

    drawBasicScene(renderer, game);
    SDL_Delay(kFrameMs);
  }

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
