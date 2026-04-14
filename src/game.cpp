#include "libbrickbreaker/game.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <string>

namespace libbrickbreaker {

std::int32_t Game::targetFPS = 30;

Game::Game() : board_(this) {}

const ImageAsset* Game::loadBitmap(const char* path) {
  (void)path;
  return nullptr;
}

void Game::setRenderContext(RenderContext* context) {
  renderContext_ = context;
}

void Game::setViewport(std::int32_t widthIn, std::int32_t heightIn) {
  viewportWidth_ = std::max<std::int32_t>(1, widthIn);
  viewportHeight_ = std::max<std::int32_t>(1, heightIn);
}

void Game::setBackgroundImage(const ImageAsset* image) {
  backgroundImage_ = image;
}

void Game::setSchedulingEnabled(bool enabled) {
  schedulingEnabled_ = enabled;
  if (!enabled) {
    running = false;
    cancelScheduledFrame();
  }
}

Board& Game::boardRef() {
  return board_;
}

const Board& Game::boardRef() const {
  return board_;
}

void Game::frameDone(std::int64_t frameStartMs) {
  const std::int64_t now = nowMs();
  const std::int32_t renderMs = static_cast<std::int32_t>(std::max<std::int64_t>(0, now - frameStartMs));

  if (std::string(deviceModel_) == "9310" || std::string(deviceModel_) == "9220" || std::string(deviceModel_) == "9320") {
    const std::int32_t slack = targetRenderTimeMs_ - renderMs;
    idleTimeMs_ = std::max<std::int32_t>(0, slack);
    if (targetFPSCapEnabled_ && idleTimeMs_ < 5) {
      idleTimeMs_ = 5;
    }
    return;
  }

  if (frameWindowStartMs_ <= 0) {
    frameWindowStartMs_ = now;
  }

  ++frameWindowCount_;
  renderWindowMs_ += renderMs;
  const std::int64_t windowElapsed = now - frameWindowStartMs_;
  if (windowElapsed >= 1000) {
    const double fps = (frameWindowCount_ * 1000.0) / std::max<std::int64_t>(1, windowElapsed);
    lastMeasuredFPS_ = fps;
    if (fps < targetFPS - 2) {
      idleTimeMs_ = std::max<std::int32_t>(0, idleTimeMs_ - 1);
    } else if (fps > targetFPS + 3) {
      const double avgRender = static_cast<double>(renderWindowMs_) / std::max(1, frameWindowCount_);
      const std::int32_t slack = targetRenderTimeMs_ - static_cast<std::int32_t>(avgRender);
      idleTimeMs_ = std::max<std::int32_t>(0, slack);
    }

    frameWindowStartMs_ = now;
    frameWindowCount_ = 0;
    renderWindowMs_ = 0;
  }

  if (targetFPSCapEnabled_ && idleTimeMs_ < 5) {
    idleTimeMs_ = 5;
  }
}

void Game::displayStoryScreen(std::int32_t storyId) {
  (void)storyId;
}

void Game::loadSplashData() {}

void Game::initializeRuntime() {}

void Game::parseField(const char* line, const char* separator) {
  (void)line;
  (void)separator;
}

LayoutGroup* Game::getLayoutGroup() {
  return nullptr;
}

void Game::readConfig() {}

const ImageAsset* Game::loadBitmap(LayoutGroup* layoutGroup,
                                   const char* key,
                                   const char* name,
                                   const ImageAsset* fallback) {
  (void)layoutGroup;
  (void)key;
  (void)name;
  return fallback;
}

void Game::initialize() {
  if (initialized) {
    return;
  }

  initializeRuntime();
  initialized = true;
}

bool Game::isInitialized() const {
  return initialized;
}

void Game::resize() {
  const std::int32_t width = viewportWidth_ > 0 ? viewportWidth_ : Board::BASE_WIDTH;
  const std::int32_t height = viewportHeight_ > 0 ? viewportHeight_ : Board::BASE_HEIGHT;

  board_.resize(0, 0, width, height);
  moveTo_ = width / 2;
  trackingTouchPoint_ = moveTo_;
}

const ImageAsset* Game::loadSplashBitmap() const {
  return nullptr;
}

void Game::loadConfig() {
  updateTargetFPS();
}

std::int32_t Game::gameState() const {
  return state;
}

void Game::increasePoints(std::int32_t points) {
  if (points <= 0) {
    return;
  }

  score += points;
  updateHighScore();
}

void Game::updateHighScore() {
  if (score > highScore) {
    highScore = score;
  }
}

void Game::newGame(std::int32_t level) {
  score = 0;
  lives = 3;
  ammo = 0;
  superLevel_ = 0;
  currentLevel_ = std::max<std::int32_t>(1, level);
  trackBallDelta = 0;
  touchTracking_ = false;

  board_.initialize(currentLevel_, true);
  state = STATE_PLAYING;
  startTimer();
}

void Game::advanceState() {
  bool transitioned = true;
  while (transitioned) {
    transitioned = false;
    switch (state) {
      case STATE_FINISHEDLEVEL:
        ++currentLevel_;
        if (currentLevel_ > 34) {
          currentLevel_ = 1;
          ++superLevel_;
        }
        board_.initialize(currentLevel_, true);
        displayStoryScreen(currentLevel_);
        playGame(currentLevel_);
        transitioned = true;
        break;
      case STATE_DEATH:
        if (lives > 0) {
          board_.initialize(currentLevel_, false);
          displayStoryScreen(currentLevel_);
          playGame(currentLevel_);
        } else {
          state = STATE_GAMEOVER;
        }
        transitioned = true;
        break;
      case STATE_GAMEOVER:
        updateHighScore();
        quit();
        transitioned = true;
        break;
      default:
        break;
    }
  }
}

void Game::playGame(std::int32_t level) {
  currentLevel_ = std::max<std::int32_t>(1, level);
  paused = false;
  touchTracking_ = false;
  state = STATE_PLAYING;
  startTimer();
}

void Game::setHighScore(std::int32_t highScoreIn) {
  highScore = highScoreIn;
}

void Game::startTimer() {
  running = true;
  lastTimeMs = nowMs();
  if (!schedulingEnabled_) {
    return;
  }

  cancelScheduledFrame();
  scheduleNextFrame(0);
}

void Game::pause() {
  paused = !paused;
  if (paused) {
    running = false;
    cancelScheduledFrame();
    board_.gamePaused(true);
  } else if (state == STATE_PLAYING) {
    board_.gamePaused(false);
    startTimer();
  }
}

void Game::quit() {
  running = false;
  paused = false;
  state = STATE_NONE;
  cancelScheduledFrame();
}

void Game::decreaseAmmo() {
  if (ammo > 0) {
    --ammo;
  }
}

void Game::updateTargetFPS() {
  targetFPS = std::max<std::int32_t>(10, std::min<std::int32_t>(60, targetFPS));
  targetRenderTimeMs_ = 1000 / std::max<std::int32_t>(1, targetFPS);
}

void Game::onOptions() {
  if (!paused) {
    pause();
  }
}

void Game::decreaseLives() {
  if (lives > 0) {
    --lives;
  }
}

void Game::increaseLives() {
  ++lives;
}

void Game::setAmmo(std::int32_t amount) {
  ammo = std::max<std::int32_t>(0, amount);
}

std::int32_t Game::ammoCount() const {
  return ammo;
}

std::int32_t Game::superLevelCount() const {
  return superLevel_;
}

void Game::run() {
  const std::int64_t frameStart = nowMs();
  const std::int32_t elapsedMs = static_cast<std::int32_t>(std::max<std::int64_t>(0, frameStart - lastTimeMs));
  lastTimeMs = frameStart;

  Paddle* paddle = board_.paddleRef();
  Ball& primary = board_.balls[0];

  if (state != STATE_PLAYING) {
    running = false;
    cancelScheduledFrame();
    advanceState();
    paint(nullptr);
    return;
  }

  if (paused) {
    running = false;
    cancelScheduledFrame();
    paint(nullptr);
    return;
  }

  if (paddle != nullptr && paddle->currentInputMode() == Paddle::INPUT_TOUCH) {
    if (primary.stopped()) {
      const std::int32_t delta = trackingTouchPoint_ - moveTo_;
      const std::int32_t threshold = board_.width >> 5;
      if (std::abs(delta) > threshold) {
        primary.direction(delta > 0 ? -1 : 1);
        trackingTouchPoint_ = moveTo_;
      }
    } else {
      paddle->moveTo(moveTo_, board_);
    }
  }

  if (trackBallDelta != 0 && paddle != nullptr) {
    paddle->move(trackBallDelta, board_);
    trackBallDelta = 0;
  }

  board_.update(elapsedMs);
  advanceState();
  paint(nullptr);
  frameDone(frameStart);

  if (!running) {
    return;
  }

  const std::int32_t delayMs = std::max<std::int32_t>(0, idleTimeMs_);
  idleTimeMs_ = 0;
  scheduleNextFrame(delayMs);
}

void Game::closeScreen() {
  quit();
}

bool Game::touchEvent(const PointerEvent& event) {
  if (paused) {
    paused = false;
    if (state == STATE_PLAYING) {
      startTimer();
    }
    return true;
  }

  if (state != STATE_PLAYING) {
    return false;
  }

  Paddle* paddle = board_.paddleRef();
  if (paddle == nullptr) {
    return false;
  }

  const PointerEvent data = extractPointerData(event);
  const std::int32_t x = data.x;

  if (paddle->currentInputMode() != Paddle::INPUT_TOUCH && paddle->intersectsTouch(x)) {
    paddle->setInputMode(Paddle::INPUT_TOUCH);
  }

  if (data.type == PointerType::kStart) {
    trackingTouchPoint_ = x;
    if (paddle->currentInputMode() == Paddle::INPUT_TOUCH) {
      std::int32_t center = paddle->centerX();
      if (paddle->flipped) {
        center = board_.width - center;
      }

      if (paddle->isWrapped() && std::abs(x - center) > (board_.width >> 1)) {
        if (x < (board_.width >> 1)) {
          touchOffset_ = center - board_.width - x;
        } else {
          touchOffset_ = center + board_.width - x;
        }
      } else {
        touchOffset_ = center - x;
      }
    }

    touchTracking_ = paddle->currentInputMode() == Paddle::INPUT_TOUCH;
    if (touchTracking_) {
      moveTo_ = x + touchOffset_;
    }
    return true;
  }

  if (data.type == PointerType::kMove) {
    touchTracking_ = paddle->currentInputMode() == Paddle::INPUT_TOUCH;
    if (touchTracking_) {
      moveTo_ = x + touchOffset_;
    }
    return true;
  }

  if (data.type == PointerType::kEnd) {
    if (paddle->currentInputMode() == Paddle::INPUT_TOUCH) {
      paddle->setInputMode(Paddle::INPUT_DEFAULT);
    }
    touchTracking_ = false;
    return true;
  }

  board_.shoot();
  return true;
}

bool Game::openProductionBackdoor(std::int32_t keycode) {
  if (keycode != KEY_F) {
    return false;
  }

  showFPSOverlay_ = !showFPSOverlay_;
  return true;
}

bool Game::openDevelopmentBackdoor(std::int32_t keycode) {
  if (keycode != KEY_T) {
    return false;
  }

  targetFPSCapEnabled_ = !targetFPSCapEnabled_;
  if (!targetFPSCapEnabled_) {
    idleTimeMs_ = 0;
  }
  return true;
}

bool Game::navigationUnclick(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::navigationMovement(const NavigationEvent& event) {
  if (!paused) {
    Ball& primary = board_.balls[0];
    if (primary.stopped()) {
      trackBallDelta += event.deltaX;
      return true;
    }

    Paddle* paddle = board_.paddleRef();
    if (paddle != nullptr) {
      if (paddle->currentInputMode() == Paddle::INPUT_DEFAULT) {
        paddle->setInputMode(Paddle::INPUT_TRACKPAD);
      }
      if (paddle->currentInputMode() == Paddle::INPUT_TRACKPAD) {
        trackBallDelta += event.deltaX;
      }
    }
  }
  return true;
}

bool Game::navigationClick(const NavigationEvent& event) {
  (void)event;
  if (paused) {
    paused = false;
    if (state == STATE_PLAYING) {
      startTimer();
    }
    return true;
  }

  if (state == STATE_PLAYING) {
    board_.shoot();
  }
  return true;
}

bool Game::trackwheelClick(const NavigationEvent& event) {
  return navigationClick(event);
}

bool Game::trackwheelRoll(const NavigationEvent& event) {
  if (!paused) {
    Paddle* paddle = board_.paddleRef();
    if (paddle != nullptr) {
      paddle->move(event.deltaX, board_);
    }
  }
  return true;
}

bool Game::keyControl(char key, std::int32_t status, std::int64_t timeMs) {
  (void)status;
  (void)timeMs;

  if (key == ' ') {
    if (paused) {
      paused = false;
      startTimer();
    } else if (state == STATE_PLAYING) {
      board_.shoot();
    }
    return true;
  }

  return false;
}

bool Game::keyDown(const KeyEvent& event) {
  const std::int32_t code = event.keycode;
  if (openProductionBackdoor(code) || openDevelopmentBackdoor(code)) {
    return true;
  }

  switch (code) {
    case KEY_P:
      pause();
      return true;
    case KEY_SPACE:
      if (paused) {
        paused = false;
        startTimer();
      } else if (state == STATE_PLAYING) {
        board_.shoot();
      }
      return true;
    case KEY_LEFT:
      trackBallDelta -= 2;
      return true;
    case KEY_RIGHT:
      trackBallDelta += 2;
      return true;
    case KEY_ESCAPE:
      quit();
      return true;
    default:
      return false;
  }
}

void Game::onUiEngineAttached(bool attached) {
  if (attached && state == STATE_PLAYING && !paused) {
    startTimer();
  }
}

void Game::makeMenu(Menu* menu, std::int32_t instance) {
  (void)menu;
  (void)instance;
}

bool Game::onMenu(std::int32_t instance) {
  if (instance == 1) {
    pause();
    return true;
  }
  return false;
}

bool Game::onClose() {
  quit();
  return true;
}

void Game::paint(Graphics* graphics) {
  (void)graphics;
  board_.render(graphics);
  hudText_ = "";
}

void Game::onObscured() {
  paused = true;
}

void Game::onVisibilityChange(bool visible) {
  if (!visible) {
    paused = true;
  }
}

void Game::scheduleNextFrame(std::int32_t delayMs) {
  (void)delayMs;
}

void Game::cancelScheduledFrame() {}

std::int64_t Game::nowMs() const {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
}

PointerEvent Game::extractPointerData(const PointerEvent& event) const {
  PointerEvent out = event;
  out.x = std::max<std::int32_t>(0, std::min<std::int32_t>(board_.width, out.x));
  return out;
}

}  // namespace libbrickbreaker
