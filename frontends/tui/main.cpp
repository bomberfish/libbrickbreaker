#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "libbrickbreaker/libbrickbreaker.hpp"

#include <ncurses.h>

namespace {

using namespace libbrickbreaker;

constexpr std::int32_t kCanvasWidth = 56;
constexpr std::int32_t kCanvasHeight = 28;
constexpr std::int32_t kFrameMs = 33;

// ncurses color pair ids
constexpr short kPairFrame = 1;
constexpr short kPairHud = 2;
constexpr short kPairOverlay = 3;
constexpr short kPairBrickBlue = 4;
constexpr short kPairBrickGreen = 5;
constexpr short kPairBrickYellow = 6;
constexpr short kPairBrickOrange = 7;
constexpr short kPairBrickGray = 8;
constexpr short kPairBall = 9;
constexpr short kPairPaddle = 10;
constexpr short kPairPill = 11;
constexpr short kPairLaser = 12;
constexpr short kPairBomb = 13;

bool g_useColor = false;

void setupColors() {
  if (!has_colors()) {
    return;
  }
  start_color();
  use_default_colors();
  init_pair(kPairFrame, COLOR_WHITE, -1);
  init_pair(kPairHud, COLOR_CYAN, -1);
  init_pair(kPairOverlay, COLOR_BLACK, COLOR_YELLOW);
  init_pair(kPairBrickBlue, COLOR_BLUE, -1);
  init_pair(kPairBrickGreen, COLOR_GREEN, -1);
  init_pair(kPairBrickYellow, COLOR_YELLOW, -1);
  init_pair(kPairBrickOrange, COLOR_RED, -1);
  init_pair(kPairBrickGray, COLOR_WHITE, -1);
  init_pair(kPairBall, COLOR_WHITE, -1);
  init_pair(kPairPaddle, COLOR_GREEN, -1);
  init_pair(kPairPill, COLOR_MAGENTA, -1);
  init_pair(kPairLaser, COLOR_RED, -1);
  init_pair(kPairBomb, COLOR_RED, -1);
  g_useColor = true;
}

struct Cell {
  char glyph{' '};
  short pair{0};
  bool bold{false};
};

std::int32_t mapX(std::int32_t x, std::int32_t boardWidth) {
  return (x * (kCanvasWidth - 1)) / std::max<std::int32_t>(1, boardWidth);
}

std::int32_t mapY(std::int32_t y, std::int32_t boardHeight) {
  return (y * (kCanvasHeight - 1)) / std::max<std::int32_t>(1, boardHeight);
}

void drawCell(std::vector<std::vector<Cell>>& canvas,
              std::int32_t x,
              std::int32_t y,
              std::int32_t w,
              std::int32_t h,
              char glyph,
              short pair,
              bool bold) {
  for (std::int32_t yy = y; yy < y + h; ++yy) {
    if (yy < 0 || yy >= static_cast<std::int32_t>(canvas.size())) {
      continue;
    }
    for (std::int32_t xx = x; xx < x + w; ++xx) {
      if (xx < 0 || xx >= static_cast<std::int32_t>(canvas[yy].size())) {
        continue;
      }
      canvas[yy][xx] = Cell{glyph, pair, bold};
    }
  }
}

struct BrickStyle {
  char glyph;
  short pair;
  bool bold;
};

BrickStyle brickStyle(std::int32_t value) {
  if (value == Bricks::EMPTY || value <= 0) {
    return {' ', 0, false};
  }
  if (value >= Bricks::INDESTRUCTIBLE) {
    return {'#', kPairBrickGray, true};
  }
  if (value >= 7) {
    return {'@', kPairBrickOrange, true};
  }
  if (value >= 5) {
    return {'%', kPairBrickYellow, true};
  }
  if (value >= 3) {
    return {'x', kPairBrickGreen, false};
  }
  return {'+', kPairBrickBlue, false};
}

void putCharStyled(std::int32_t y, std::int32_t x, char ch, short pair, bool bold) {
  attr_t attr = A_NORMAL;
  if (bold) {
    attr |= A_BOLD;
  }
  if (g_useColor && pair != 0) {
    attron(COLOR_PAIR(pair) | attr);
    mvaddch(y, x, static_cast<chtype>(ch));
    attroff(COLOR_PAIR(pair) | attr);
  } else {
    if (bold) {
      attron(A_BOLD);
    }
    mvaddch(y, x, static_cast<chtype>(ch));
    if (bold) {
      attroff(A_BOLD);
    }
  }
}

void putStringStyled(std::int32_t y, std::int32_t x, const std::string& text, short pair, bool bold) {
  attr_t attr = A_NORMAL;
  if (bold) {
    attr |= A_BOLD;
  }
  if (g_useColor && pair != 0) {
    attron(COLOR_PAIR(pair) | attr);
    mvprintw(y, x, "%s", text.c_str());
    attroff(COLOR_PAIR(pair) | attr);
  } else {
    if (bold) {
      attron(A_BOLD);
    }
    mvprintw(y, x, "%s", text.c_str());
    if (bold) {
      attroff(A_BOLD);
    }
  }
}

void renderFrame(const Game& game,
                 std::int32_t fpsEstimate,
                 std::int32_t levelDisplay,
                 std::int32_t superLevel,
                 const std::string& statusLine) {
  const Board& board = game.boardRef();
  std::vector<std::vector<Cell>> canvas(
      static_cast<std::size_t>(kCanvasHeight),
      std::vector<Cell>(static_cast<std::size_t>(kCanvasWidth), Cell{}));

  for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
    for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
      const std::int32_t cell = board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)];
      if (cell == Bricks::EMPTY || cell <= 0) {
        continue;
      }

      const std::int32_t x = mapX(col * board.tileWidth, board.width);
      const std::int32_t y = mapY(row * board.tileHeight + board.bricks.amountMoved, board.height);
      const std::int32_t w = std::max<std::int32_t>(
          1,
          board.tileWidth * kCanvasWidth / std::max<std::int32_t>(1, board.width));
      const std::int32_t h = std::max<std::int32_t>(
          1,
          board.tileHeight * kCanvasHeight / std::max<std::int32_t>(1, board.height));
      const BrickStyle style = brickStyle(cell);
      drawCell(canvas, x, y, w, h, style.glyph, style.pair, style.bold);
    }
  }

  for (const Pill& pill : board.pills.pool()) {
    if (!pill.isActive()) {
      continue;
    }
    drawCell(canvas, mapX(pill.x, board.width), mapY(pill.y, board.height), 1, 1, 'P', kPairPill, true);
  }

  for (const Bullet& laser : board.lasers) {
    if (!laser.isActive()) {
      continue;
    }
    drawCell(canvas, mapX(laser.x, board.width), mapY(laser.y, board.height), 1, 1, '|', kPairLaser, true);
  }

  if (board.bomb.isActive()) {
    drawCell(canvas, mapX(board.bomb.x, board.width), mapY(board.bomb.y, board.height), 1, 1, 'B', kPairBomb, true);
  }

  for (const Ball& ball : board.balls) {
    if (!ball.isActive()) {
      continue;
    }
    drawCell(canvas, mapX(ball.x, board.width), mapY(ball.y, board.height), 1, 1, 'o', kPairBall, true);
  }

  const std::int32_t paddleX = mapX(board.paddle.x, board.width);
  const std::int32_t paddleY = mapY(board.paddle.y, board.height);
  const std::int32_t paddleW = std::max<std::int32_t>(
      2,
      board.paddle.width * kCanvasWidth / std::max<std::int32_t>(1, board.width));
  drawCell(canvas, paddleX, paddleY, paddleW, 1, '=', kPairPaddle, true);

  erase();

  const std::string title = "BrickBreaker  TUI";
  putStringStyled(0, 0, title, kPairHud, true);

  char buffer[256];
  if (superLevel > 0) {
    std::snprintf(buffer, sizeof(buffer),
                  "score=%d  high=%d  lives=%d  ammo=%d  level=%d+%d  fps~%d",
                  game.score, game.highScore, game.lives, game.ammoCount(),
                  levelDisplay, superLevel, fpsEstimate);
  } else {
    std::snprintf(buffer, sizeof(buffer),
                  "score=%d  high=%d  lives=%d  ammo=%d  level=%d  fps~%d",
                  game.score, game.highScore, game.lives, game.ammoCount(),
                  levelDisplay, fpsEstimate);
  }
  putStringStyled(0, static_cast<std::int32_t>(title.size()) + 2, buffer, kPairHud, false);
  putStringStyled(1, 0, "A/D or arrows move  Space shoot  P pause  Q quit", kPairHud, false);

  putCharStyled(2, 0, '+', kPairFrame, true);
  for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
    putCharStyled(2, x + 1, '-', kPairFrame, true);
  }
  putCharStyled(2, kCanvasWidth + 1, '+', kPairFrame, true);

  for (std::int32_t y = 0; y < kCanvasHeight; ++y) {
    putCharStyled(y + 3, 0, '|', kPairFrame, true);
    for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
      const Cell& cell = canvas[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)];
      putCharStyled(y + 3, x + 1, cell.glyph, cell.pair, cell.bold);
    }
    putCharStyled(y + 3, kCanvasWidth + 1, '|', kPairFrame, true);
  }

  const std::int32_t bottom = kCanvasHeight + 3;
  putCharStyled(bottom, 0, '+', kPairFrame, true);
  for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
    putCharStyled(bottom, x + 1, '-', kPairFrame, true);
  }
  putCharStyled(bottom, kCanvasWidth + 1, '+', kPairFrame, true);

  if (!statusLine.empty()) {
    putStringStyled(bottom + 1, 0, statusLine, kPairHud, true);
  }

  refresh();
}

void movePaddle(Game& game, std::int32_t delta) {
  Board& board = game.boardRef();
  Paddle* paddle = board.paddleRef();
  if (paddle != nullptr) {
    paddle->move(delta, board);
  }
}

bool handleInput(Game& game, int key) {
  switch (key) {
    case 'a':
    case 'A':
    case 'h':
    case KEY_LEFT:
      movePaddle(game, -2);
      return false;
    case 'd':
    case 'D':
    case 'l':
    case KEY_RIGHT:
      movePaddle(game, 2);
      return false;
    case ' ': {
      KeyEvent event{};
      event.keycode = Game::KEY_SPACE;
      game.keyDown(event);
      return false;
    }
    case 'p':
    case 'P': {
      KeyEvent event{};
      event.keycode = Game::KEY_P;
      game.keyDown(event);
      return false;
    }
    case 'r':
    case 'R':
      game.newGame(1);
      return false;
    case 'q':
    case 'Q':
    case 27:  // Escape
      return true;
    default:
      return false;
  }
}

bool consumeInput(Game& game) {
  bool quit = false;
  int key = getch();
  while (key != ERR) {
    if (handleInput(game, key)) {
      quit = true;
      break;
    }
    key = getch();
  }
  return quit;
}

bool tryLoadLevels(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input.good()) {
    return false;
  }

  std::vector<std::uint8_t> bytes(
      (std::istreambuf_iterator<char>(input)),
      std::istreambuf_iterator<char>());
  if (bytes.empty()) {
    return false;
  }

  Bricks::setLevelData(bytes.data(), static_cast<std::int32_t>(bytes.size()));
  return true;
}

bool autoLoadLevels() {
  static const char* const candidates[] = {
      "levels.bin",
      "assets/levels.bin",
      "../assets/levels.bin",
      "../../assets/levels.bin",
  };
  for (const char* candidate : candidates) {
    if (tryLoadLevels(candidate)) {
      return true;
    }
  }
  return false;
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
    if (!tryLoadLevels(levelsPath)) {
      std::cerr << "Warning: failed to load levels file: " << levelsPath << "\n";
    }
  } else {
    autoLoadLevels();
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);
  setupColors();

  Game game;
  game.setSchedulingEnabled(false);
  game.setViewport(Board::BASE_WIDTH, Board::BASE_HEIGHT);
  game.resize();
  game.newGame(1);

  bool quit = false;
  std::int32_t fpsEstimate = 30;
  auto lastFrame = std::chrono::steady_clock::now();
  std::int32_t currentLevel = 1;

  while (!quit && game.gameState() != Game::STATE_NONE) {
    const auto frameStart = std::chrono::steady_clock::now();
    quit = consumeInput(game);

    const auto now = std::chrono::steady_clock::now();
    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame).count();
    const std::int32_t boundedElapsed = static_cast<std::int32_t>(std::clamp<std::int64_t>(elapsedMs, 0, 33));
    if (elapsedMs > 0) {
      fpsEstimate = static_cast<std::int32_t>(1000 / std::max<std::int64_t>(1, elapsedMs));
    }
    lastFrame = now;

    if (!quit && !game.paused) {
      game.applyInput();
      game.boardRef().update(boundedElapsed);
      const std::int32_t prevState = game.gameState();
      game.advanceState();
      if (prevState == Game::STATE_FINISHEDLEVEL) {
        ++currentLevel;
        if (currentLevel > Bricks::getNumLevels()) {
          currentLevel = 1;
        }
      }
    }

    std::string overlay;
    if (game.paused) {
      overlay = "[PAUSED]  press P or Space to resume";
    } else if (game.gameState() == Game::STATE_GAMEOVER) {
      overlay = "GAME OVER  press R to restart, Q to quit";
    } else if (game.gameState() == Game::STATE_DEATH) {
      overlay = "BALL LOST  preparing next ball...";
    } else if (game.gameState() == Game::STATE_FINISHEDLEVEL) {
      overlay = "LEVEL CLEAR";
    }

    renderFrame(game, fpsEstimate, currentLevel, game.superLevelCount(), overlay);

    const auto workMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - frameStart)
            .count();
    if (workMs < kFrameMs) {
      std::this_thread::sleep_for(std::chrono::milliseconds(kFrameMs - workMs));
    }
  }

  endwin();
  return 0;
}
