#include <algorithm>
#include <chrono>
#include <cstdint>
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

std::int32_t mapX(std::int32_t x, std::int32_t boardWidth) {
  return (x * (kCanvasWidth - 1)) / std::max<std::int32_t>(1, boardWidth);
}

std::int32_t mapY(std::int32_t y, std::int32_t boardHeight) {
  return (y * (kCanvasHeight - 1)) / std::max<std::int32_t>(1, boardHeight);
}

void drawRect(std::vector<std::string>& canvas,
              std::int32_t x,
              std::int32_t y,
              std::int32_t w,
              std::int32_t h,
              char symbol) {
  for (std::int32_t yy = y; yy < y + h; ++yy) {
    if (yy < 0 || yy >= static_cast<std::int32_t>(canvas.size())) {
      continue;
    }
    for (std::int32_t xx = x; xx < x + w; ++xx) {
      if (xx < 0 || xx >= static_cast<std::int32_t>(canvas[yy].size())) {
        continue;
      }
      canvas[yy][xx] = symbol;
    }
  }
}

char brickSymbol(std::int32_t value) {
  if (value == Bricks::EMPTY || value <= 0) {
    return ' ';
  }
  if (value >= Bricks::INDESTRUCTIBLE) {
    return '#';
  }
  if (value >= 7) {
    return '@';
  }
  if (value >= 5) {
    return '%';
  }
  if (value >= 3) {
    return 'x';
  }
  return '+';
}

void renderFrame(const Game& game, std::int32_t fpsEstimate) {
  const Board& board = game.boardRef();
  std::vector<std::string> canvas(
      static_cast<std::size_t>(kCanvasHeight),
      std::string(static_cast<std::size_t>(kCanvasWidth), ' '));

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
      drawRect(canvas, x, y, w, h, brickSymbol(cell));
    }
  }

  for (const Pill& pill : board.pills.pool()) {
    if (!pill.isActive()) {
      continue;
    }
    drawRect(canvas, mapX(pill.x, board.width), mapY(pill.y, board.height), 1, 1, 'P');
  }

  for (const Bullet& laser : board.lasers) {
    if (!laser.isActive()) {
      continue;
    }
    drawRect(canvas, mapX(laser.x, board.width), mapY(laser.y, board.height), 1, 1, '|');
  }

  if (board.bomb.isActive()) {
    drawRect(canvas, mapX(board.bomb.x, board.width), mapY(board.bomb.y, board.height), 1, 1, 'B');
  }

  for (const Ball& ball : board.balls) {
    if (!ball.isActive()) {
      continue;
    }
    drawRect(canvas, mapX(ball.x, board.width), mapY(ball.y, board.height), 1, 1, 'o');
  }

  const std::int32_t paddleX = mapX(board.paddle.x, board.width);
  const std::int32_t paddleY = mapY(board.paddle.y, board.height);
  const std::int32_t paddleW = std::max<std::int32_t>(
      2,
      board.paddle.width * kCanvasWidth / std::max<std::int32_t>(1, board.width));
  drawRect(canvas, paddleX, paddleY, paddleW, 1, '=');

  erase();
  mvprintw(0,
           0,
           "BrickBreaker TUI | score=%d lives=%d ammo=%d state=%d fps~%d",
           game.score,
           game.lives,
           game.ammoCount(),
           game.gameState(),
           fpsEstimate);
  mvprintw(1, 0, "Controls: A/D or arrows move, Space shoot, P pause, Q quit");

  mvaddch(2, 0, '+');
  for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
    mvaddch(2, x + 1, '-');
  }
  mvaddch(2, kCanvasWidth + 1, '+');

  for (std::int32_t y = 0; y < kCanvasHeight; ++y) {
    mvaddch(y + 3, 0, '|');
    for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
      mvaddch(y + 3, x + 1, canvas[static_cast<std::size_t>(y)][static_cast<std::size_t>(x)]);
    }
    mvaddch(y + 3, kCanvasWidth + 1, '|');
  }

  const std::int32_t bottom = kCanvasHeight + 3;
  mvaddch(bottom, 0, '+');
  for (std::int32_t x = 0; x < kCanvasWidth; ++x) {
    mvaddch(bottom, x + 1, '-');
  }
  mvaddch(bottom, kCanvasWidth + 1, '+');

  refresh();
}

void movePaddle(Game& game, std::int32_t delta) {
  Board& board = game.boardRef();
  Paddle* paddle = board.paddleRef();
  if (paddle != nullptr) {
    paddle->move(delta, board);
  }
}

void handleInput(Game& game, int key) {
  switch (key) {
    case 'a':
    case 'A':
    case 'h':
    case KEY_LEFT:
      movePaddle(game, -2);
      break;
    case 'd':
    case 'D':
    case 'l':
    case KEY_RIGHT:
      movePaddle(game, 2);
      break;
    case ' ':
      game.boardRef().shoot();
      break;
    case 'p':
    case 'P':
      game.pause();
      break;
    default:
      break;
  }
}

bool consumeInput(Game& game) {
  bool quit = false;
  int key = getch();
  while (key != ERR) {
    if (key == 'q' || key == 'Q') {
      quit = true;
      break;
    }
    handleInput(game, key);
    key = getch();
  }
  return quit;
}

bool loadLevelsFile(const char* path) {
  if (path == nullptr || path[0] == '\0') {
    return false;
  }

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

  if (levelsPath != nullptr && !loadLevelsFile(levelsPath)) {
    std::cerr << "Warning: failed to load levels file: " << levelsPath << "\n";
  }

  initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  nodelay(stdscr, TRUE);
  curs_set(0);

  Game game;
  game.setSchedulingEnabled(false);
  game.setViewport(Board::BASE_WIDTH, Board::BASE_HEIGHT);
  game.resize();
  game.newGame(1);

  bool quit = false;
  std::int32_t fpsEstimate = 30;
  auto lastFrame = std::chrono::steady_clock::now();

  while (!quit && game.gameState() != Game::STATE_NONE) {
    const auto frameStart = std::chrono::steady_clock::now();
    quit = consumeInput(game);

    if (!quit && !game.paused) {
      game.boardRef().update(kFrameMs);
      game.advanceState();
    }

    const auto now = std::chrono::steady_clock::now();
    const auto frameDeltaMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFrame).count();
    if (frameDeltaMs > 0) {
      fpsEstimate = static_cast<std::int32_t>(1000 / frameDeltaMs);
    }
    lastFrame = now;

    renderFrame(game, fpsEstimate);

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
