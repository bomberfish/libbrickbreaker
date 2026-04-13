#include "libbrickbreaker/game.hpp"

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
  (void)widthIn;
  (void)heightIn;
}

void Game::setBackgroundImage(const ImageAsset* image) {
  backgroundImage_ = image;
}

void Game::setSchedulingEnabled(bool enabled) {
  schedulingEnabled_ = enabled;
}

Board& Game::boardRef() {
  return board_;
}

const Board& Game::boardRef() const {
  return board_;
}

void Game::frameDone(std::int64_t frameStartMs) {
  (void)frameStartMs;
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

void Game::initialize() {}

bool Game::isInitialized() const {
  return initialized;
}

void Game::resize() {}

const ImageAsset* Game::loadSplashBitmap() const {
  return nullptr;
}

void Game::loadConfig() {}

std::int32_t Game::gameState() const {
  return state;
}

void Game::increasePoints(std::int32_t points) {
  (void)points;
}

void Game::updateHighScore() {}

void Game::newGame(std::int32_t level) {
  (void)level;
}

void Game::advanceState() {}

void Game::playGame(std::int32_t level) {
  (void)level;
}

void Game::setHighScore(std::int32_t highScoreIn) {
  highScore = highScoreIn;
}

void Game::startTimer() {}

void Game::pause() {}

void Game::quit() {}

void Game::decreaseAmmo() {}

void Game::updateTargetFPS() {}

void Game::onOptions() {}

void Game::decreaseLives() {}

void Game::increaseLives() {}

void Game::setAmmo(std::int32_t amount) {
  ammo = amount;
}

std::int32_t Game::ammoCount() const {
  return ammo;
}

std::int32_t Game::superLevelCount() const {
  return superLevel_;
}

void Game::run() {}

void Game::closeScreen() {}

bool Game::touchEvent(const PointerEvent& event) {
  (void)event;
  return false;
}

bool Game::openProductionBackdoor(std::int32_t keycode) {
  (void)keycode;
  return false;
}

bool Game::openDevelopmentBackdoor(std::int32_t keycode) {
  (void)keycode;
  return false;
}

bool Game::navigationUnclick(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::navigationMovement(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::navigationClick(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::trackwheelClick(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::trackwheelRoll(const NavigationEvent& event) {
  (void)event;
  return false;
}

bool Game::keyControl(char key, std::int32_t status, std::int64_t timeMs) {
  (void)key;
  (void)status;
  (void)timeMs;
  return false;
}

bool Game::keyDown(const KeyEvent& event) {
  (void)event;
  return false;
}

void Game::onUiEngineAttached(bool attached) {
  (void)attached;
}

void Game::makeMenu(Menu* menu, std::int32_t instance) {
  (void)menu;
  (void)instance;
}

bool Game::onMenu(std::int32_t instance) {
  (void)instance;
  return false;
}

bool Game::onClose() {
  return false;
}

void Game::paint(Graphics* graphics) {
  (void)graphics;
}

void Game::onObscured() {}

void Game::onVisibilityChange(bool visible) {
  (void)visible;
}

void Game::scheduleNextFrame(std::int32_t delayMs) {
  (void)delayMs;
}

void Game::cancelScheduledFrame() {}

std::int64_t Game::nowMs() const {
  return 0;
}

PointerEvent Game::extractPointerData(const PointerEvent& event) const {
  return event;
}

}  // namespace libbrickbreaker
