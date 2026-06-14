#include "libbrickbreaker/parity.hpp"

#include <array>
#include <sstream>
#include <string>

#include "libbrickbreaker/board.hpp"
#include "libbrickbreaker/game.hpp"

namespace libbrickbreaker {

namespace {

ProbeResult makePass(const char* id, const std::string& details) {
  return ProbeResult{id, true, details};
}

ProbeResult makeFail(const char* id, const std::string& details) {
  return ProbeResult{id, false, details};
}

Game createSilentGame() {
  Game game;
  game.setSchedulingEnabled(false);
  game.setViewport(Board::BASE_WIDTH, Board::BASE_HEIGHT);
  game.resize();
  return game;
}

void clearBricks(Board& board) {
  for (std::int32_t row = 0; row < Bricks::ROWS; ++row) {
    for (std::int32_t col = 0; col < Bricks::COLUMNS; ++col) {
      board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = Bricks::EMPTY;
      board.bricks.bonuses[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = 0;
    }
  }
  board.bricks.numBlocks = 0;
}

void setSingleBrick(Board& board, std::int32_t col, std::int32_t row, std::int32_t value) {
  clearBricks(board);
  board.bricks.cells[static_cast<std::size_t>(row)][static_cast<std::size_t>(col)] = value;
  board.bricks.numBlocks = (value == Bricks::INDESTRUCTIBLE || value == Bricks::EMPTY) ? 0 : 1;
}

void forceBall(Ball& ball,
               std::int32_t x,
               std::int32_t y,
               std::int32_t dx,
               std::int32_t dy) {
  ball.activate();
  ball.x = x;
  ball.y = y;
  ball.oldx = x;
  ball.oldy = y;
  ball.dx = dx;
  ball.dy = dy;
}

void step(Board& board, std::int32_t elapsedMs = 16) {
  board.update(elapsedMs);
}

ProbeResult testO01StickyCountIncrements() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  const std::int32_t before = board.stickyPaddleCount;
  step(board);
  const std::int32_t after = board.stickyPaddleCount;
  if (after == before + 1 && board.balls[0].stopped()) {
    return makePass("O01", "sticky count increments while ball is stopped");
  }

  std::ostringstream ss;
  ss << "expected sticky increment, got " << before << " -> " << after;
  return makeFail("O01", ss.str());
}

ProbeResult testO02StickyReleaseAfterThreshold() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  board.stickyPaddleCount = 151;
  step(board);
  Ball& ball = board.balls[0];
  if (!ball.stopped() && ball.dy < 0 && board.stickyPaddleCount == 0) {
    return makePass("O02", "ball auto-released after sticky timeout");
  }

  return makeFail("O02", "sticky auto-release did not occur");
}

ProbeResult testO03RightWallBounce() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  Ball& ball = board.balls[0];
  forceBall(ball, board.width - Ball::RADIUS, board.height / 2, 3, -1);
  const std::int32_t beforeHits = ball.numNonPaddleHits;
  step(board);

  if (ball.dx < 0 && ball.numNonPaddleHits > beforeHits) {
    return makePass("O03", "right-wall bounce flips dx");
  }
  return makeFail("O03", "right-wall bounce mismatch");
}

ProbeResult testO04TopWallBounce() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  Ball& ball = board.balls[0];
  const std::int32_t beforeBounces = board.numBounces;
  forceBall(ball, board.width / 2, 1, 1, -3);
  step(board);

  if (ball.dy > 0 && board.numBounces > beforeBounces) {
    return makePass("O04", "top-wall bounce flips dy and increments bounces");
  }
  return makeFail("O04", "top-wall bounce mismatch");
}

ProbeResult testO05BrickHitPath() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  setSingleBrick(board, 2, 2, 2);
  const std::int32_t tileWidth = board.tileWidth;
  const std::int32_t tileHeight = board.tileHeight;
  const std::int32_t tileTop = 2 * tileHeight + board.bricks.amountMoved;
  Ball& ball = board.balls[0];
  forceBall(ball, 2 * tileWidth + (tileWidth / 4), tileTop - 1, 0, 3);
  step(board);

  const std::int32_t cell = board.bricks.cells[2][2];
  if (cell == 1 || cell == Bricks::EMPTY) {
    return makePass("O05", "brick durability updates on hit");
  }
  return makeFail("O05", "brick durability did not update");
}

ProbeResult testO06DrainLifeLoss() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  game.lives = 3;
  board.numActiveBalls = 1;
  Ball& ball = board.balls[0];
  forceBall(ball, board.width / 2, board.height + 20, 0, 3);
  step(board);

  if (game.state == Game::STATE_DEATH && game.lives == 2 && board.numActiveBalls == 0) {
    return makePass("O06", "ball drain decrements life and enters death state");
  }
  return makeFail("O06", "drain/life-loss behavior mismatch");
}

ProbeResult testO07AutomatedBottomRebound() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  Ball::automatedTesting = true;
  Ball& ball = board.balls[0];
  forceBall(ball, board.width / 2, board.height + 10, 0, 3);
  step(board);
  Ball::automatedTesting = false;

  if (ball.isActive() && ball.dy < 0) {
    return makePass("O07", "automation rebounds at bottom");
  }
  return makeFail("O07", "automation rebound mismatch");
}

ProbeResult testO08BounceThresholdMovesBricks() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  board.bricks.amountMoved = 0;
  board.numBounces = 50;
  Ball& ball = board.balls[0];
  forceBall(ball, board.width / 2, 1, 1, -3);
  step(board);

  if (board.numBounces > 50 && board.bricks.amountMoved > 0) {
    return makePass("O08", "bounce threshold triggers bricks move-down");
  }
  return makeFail("O08", "bounce threshold move-down mismatch");
}

ProbeResult testO09CatchNewSpawnsFourBalls() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  board.pills.drop(Pill::NEW, 2, 2, board.bricks);
  Pill* candidate = nullptr;
  for (Pill& pill : board.pills.pool()) {
    if (pill.isActive()) {
      candidate = &pill;
      break;
    }
  }
  if (candidate == nullptr) {
    return makeFail("O09", "pill did not spawn");
  }

  candidate->x = board.paddle.centerX() - (candidate->width / 2);
  candidate->y = board.paddle.y;
  board.update(16);

  std::int32_t activeCount = 0;
  bool hasLeft = false;
  bool hasRight = false;
  for (const Ball& ball : board.balls) {
    if (ball.isActive()) {
      ++activeCount;
      hasLeft = hasLeft || (ball.dx == -3 && ball.dy == -3);
      hasRight = hasRight || (ball.dx == 3 && ball.dy == -3);
    }
  }

  if (activeCount == 4 && hasLeft && hasRight) {
    return makePass("O09", "NEW power-up spawns four balls with expected vectors");
  }
  return makeFail("O09", "NEW power-up multiball mismatch");
}

ProbeResult testO10CatchGunSetsAmmoAndMode() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  board.powerUp(Pill::GUN);
  if (!board.gotBomb && game.ammoCount() == 3 && board.paddle.mode == Paddle::MODE_GUN) {
    return makePass("O10", "GUN power-up sets ammo and mode");
  }
  return makeFail("O10", "GUN power-up mismatch");
}

ProbeResult testO11ShootBombConsumesAmmo() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  game.setAmmo(2);
  board.gotLaser = false;
  board.shoot();

  if (board.bomb.isActive() && game.ammoCount() == 1) {
    return makePass("O11", "bomb shot consumes ammo");
  }
  return makeFail("O11", "bomb shooting mismatch");
}

ProbeResult testO12ShootLaserUsesPool() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  game.setAmmo(0);
  board.gotLaser = true;
  board.bomb.deactivate();
  for (Bullet& laser : board.lasers) {
    laser.deactivate();
  }

  board.shoot();
  std::int32_t active = 0;
  for (const Bullet& laser : board.lasers) {
    if (laser.isActive()) {
      ++active;
    }
  }

  if (active >= 1) {
    return makePass("O12", "laser shooting uses pooled bullets");
  }
  return makeFail("O12", "laser shooting mismatch");
}

ProbeResult testO13NoBricksFinishesLevel() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  clearBricks(board);
  step(board);
  if (game.state == Game::STATE_FINISHEDLEVEL) {
    return makePass("O13", "board sets finished-level when no bricks remain");
  }
  return makeFail("O13", "finished-level transition mismatch");
}

ProbeResult testO14DeathWithLivesReentersPlay() {
  Game game = createSilentGame();
  game.newGame(1);
  game.lives = 2;
  game.state = Game::STATE_DEATH;
  game.advanceState();

  if (game.state == Game::STATE_PLAYING) {
    return makePass("O14", "death state returns to playing when lives remain");
  }
  return makeFail("O14", "death-to-playing transition mismatch");
}

ProbeResult testO15GameOverReturnsNone() {
  Game game = createSilentGame();
  game.newGame(1);
  game.state = Game::STATE_GAMEOVER;
  game.advanceState();

  if (game.state == Game::STATE_NONE) {
    return makePass("O15", "game-over returns to none");
  }
  return makeFail("O15", "game-over transition mismatch");
}

ProbeResult testO16DurabilityNeedsMultipleHits() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  board.bricks.cells[2][2] = 8;
  board.bricks.numBlocks = 1;

  board.bricks.hitBrick(2, 2, 2);
  const std::int32_t after1 = board.bricks.cells[2][2];
  board.bricks.hitBrick(2, 2, 2);
  const std::int32_t after2 = board.bricks.cells[2][2];
  board.bricks.hitBrick(2, 2, 2);
  const std::int32_t after3 = board.bricks.cells[2][2];
  board.bricks.hitBrick(2, 2, 2);
  const std::int32_t after4 = board.bricks.cells[2][2];

  if (after1 == 6 && after2 == 4 && after3 == 2 && after4 <= 0 && board.bricks.numBlocks == 0) {
    return makePass("O16", "durability chain matches expected damage steps");
  }
  return makeFail("O16", "durability decrement chain mismatch");
}

ProbeResult testO17AimingSkipsZeroAndClamps() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  Ball& ball = board.balls[0];
  if (!ball.stopped()) {
    return makeFail("O17", "primary ball should start stopped");
  }

  ball.aimDx = 1;
  ball.direction(-1);
  const std::int32_t skipZero = ball.aimDx;
  for (std::int32_t i = 0; i < 10; ++i) {
    ball.direction(1);
  }
  const std::int32_t maxClamp = ball.aimDx;
  for (std::int32_t i = 0; i < 20; ++i) {
    ball.direction(-1);
  }
  const std::int32_t minClamp = ball.aimDx;

  if (skipZero == -1 && maxClamp == 4 && minClamp == -4) {
    return makePass("O17", "aim direction skips zero and clamps to +-4");
  }
  return makeFail("O17", "aim direction clamp/skip behavior mismatch");
}

ProbeResult testO18ScorePerHitNotPerDestroy() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  board.bricks.cells[2][2] = 8;  // needs four damage-2 hits to destroy
  board.bricks.numBlocks = 1;

  const std::int32_t before = game.score;
  board.bricks.hitBrick(2, 2, 2);  // hit 1: durability 8 -> 6 (not destroyed)
  const std::int32_t afterHit1 = game.score;
  board.bricks.hitBrick(2, 2, 2);  // hit 2: 6 -> 4
  board.bricks.hitBrick(2, 2, 2);  // hit 3: 4 -> 2
  board.bricks.hitBrick(2, 2, 2);  // hit 4: 2 -> destroyed
  const std::int32_t afterDestroy = game.score;

  // Original awards 10 points on every hit, even non-destroying ones.
  if (afterHit1 == before + 10 && afterDestroy == before + 40) {
    return makePass("O18", "score increases 10 per hit, not only on destroy");
  }

  std::ostringstream ss;
  ss << "expected +10 then +40, got +" << (afterHit1 - before) << " then +" << (afterDestroy - before);
  return makeFail("O18", ss.str());
}

ProbeResult testO19BombProjectileInstantDestroy() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  board.bricks.cells[3][3] = 8;  // high durability; bomb projectile must clear it outright
  board.bricks.numBlocks = 1;
  board.gotBomb = false;  // ensure the explosive-ball path is not involved

  const std::int32_t before = game.score;
  board.bomb.activate(3 * board.tileWidth + (board.tileWidth / 2),
                      3 * board.tileHeight + board.bricks.amountMoved + 1,
                      10);
  board.renderProjectiles(nullptr);  // processes the bomb-vs-brick collision

  const std::int32_t cell = board.bricks.cells[3][3];
  if (cell == Bricks::EMPTY && game.score == before + 50 && !board.bomb.isActive() &&
      board.bricks.numBlocks == 0) {
    return makePass("O19", "bomb projectile instantly destroys brick and awards 50");
  }
  return makeFail("O19", "bomb projectile destroy/score mismatch");
}

ProbeResult testO20ExplosiveBallChain() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();
  clearBricks(board);

  board.bricks.cells[3][3] = 4;  // center
  board.bricks.cells[2][3] = 8;  // up    (orthogonal, damage 2)
  board.bricks.cells[4][3] = 8;  // down  (orthogonal, damage 2)
  board.bricks.cells[3][2] = 8;  // left  (orthogonal, damage 2)
  board.bricks.cells[3][4] = 8;  // right (orthogonal, damage 2)
  board.bricks.cells[2][2] = 8;  // up-left (diagonal, damage 1)
  board.bricks.numBlocks = 6;
  board.gotBomb = true;

  board.bricks.hitBrick(3, 3, 2);  // explosive contact at the centre

  const bool centerGone = board.bricks.cells[3][3] == Bricks::EMPTY;
  const bool orthoDamaged = board.bricks.cells[2][3] == 6 && board.bricks.cells[4][3] == 6 &&
                            board.bricks.cells[3][2] == 6 && board.bricks.cells[3][4] == 6;
  const bool diagDamaged = board.bricks.cells[2][2] == 7;
  const bool bombConsumed = !board.gotBomb;

  if (centerGone && orthoDamaged && diagDamaged && bombConsumed) {
    return makePass("O20", "explosive ball destroys center, damages neighbors 2/1, resets gotBomb");
  }
  return makeFail("O20", "explosive ball neighbor-chain/gotBomb-reset mismatch");
}

ProbeResult testO21KillBallsKeepsHighestInSlotZero() {
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  for (std::int32_t i = 0; i < Board::MAX_BALLS; ++i) {
    Ball& ball = board.balls[static_cast<std::size_t>(i)];
    ball.activate();
    ball.x = 10 + i;
    ball.y = (i == 2) ? 50 : (100 - i);  // ball index 2 is highest on screen (smallest y)
    ball.setSpeed(1, 1);
  }
  board.numActiveBalls = Board::MAX_BALLS;
  const std::int32_t survivorX = board.balls[2].x;

  board.killBallsExceptOne();

  std::int32_t activeCount = 0;
  for (const Ball& ball : board.balls) {
    if (ball.isActive()) {
      ++activeCount;
    }
  }

  if (activeCount == 1 && board.balls[0].isActive() && board.balls[0].x == survivorX &&
      board.numActiveBalls == 1) {
    return makePass("O21", "killBallsExceptOne swaps highest ball into active slot 0");
  }
  return makeFail("O21", "killBallsExceptOne survivor/slot-0 mismatch");
}

ProbeResult testO22TouchDragMovesPaddle() {
  // Regression guard: frontends drive board.update() directly, so Game::applyInput() must
  // translate a pointer drag into paddle movement (otherwise the paddle is frozen and the
  // ball appears to always bounce the same way regardless of contact point).
  Game game = createSilentGame();
  game.newGame(1);
  Board& board = game.boardRef();

  board.balls[0].setSpeed(1, -3);  // ball in motion -> drag moves paddle (does not aim)
  const std::int32_t startX = board.paddle.x;
  const std::int32_t center = board.paddle.centerX();
  const std::int32_t drag = board.width / 4;

  PointerEvent start{};
  start.type = PointerType::kStart;
  start.x = center;
  start.y = board.paddle.y;
  game.touchEvent(start);

  PointerEvent right{};
  right.type = PointerType::kMove;
  right.x = center + drag;
  right.y = board.paddle.y;
  game.touchEvent(right);
  game.applyInput();
  const bool movedRight = board.paddle.x > startX;

  PointerEvent left{};
  left.type = PointerType::kMove;
  left.x = center - drag;
  left.y = board.paddle.y;
  game.touchEvent(left);
  game.applyInput();
  const bool movedLeft = board.paddle.x < startX;

  if (movedRight && movedLeft) {
    return makePass("O22", "pointer drag moves the paddle through applyInput");
  }
  return makeFail("O22", "pointer drag did not move the paddle");
}

}  // namespace

ParityReport runParityProbes() {
  std::array<ProbeResult, 22> probes = {
      testO01StickyCountIncrements(),
      testO02StickyReleaseAfterThreshold(),
      testO03RightWallBounce(),
      testO04TopWallBounce(),
      testO05BrickHitPath(),
      testO06DrainLifeLoss(),
      testO07AutomatedBottomRebound(),
      testO08BounceThresholdMovesBricks(),
      testO09CatchNewSpawnsFourBalls(),
      testO10CatchGunSetsAmmoAndMode(),
      testO11ShootBombConsumesAmmo(),
      testO12ShootLaserUsesPool(),
      testO13NoBricksFinishesLevel(),
      testO14DeathWithLivesReentersPlay(),
      testO15GameOverReturnsNone(),
      testO16DurabilityNeedsMultipleHits(),
      testO17AimingSkipsZeroAndClamps(),
      testO18ScorePerHitNotPerDestroy(),
      testO19BombProjectileInstantDestroy(),
      testO20ExplosiveBallChain(),
      testO21KillBallsKeepsHighestInSlotZero(),
      testO22TouchDragMovesPaddle(),
  };

  ParityReport report;
  report.total = static_cast<std::int32_t>(probes.size());
  report.results.reserve(probes.size());

  for (const ProbeResult& probe : probes) {
    report.results.push_back(probe);
    if (probe.passed) {
      ++report.passed;
    }
  }

  return report;
}

}  // namespace libbrickbreaker
