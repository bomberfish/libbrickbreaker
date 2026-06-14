#pragma once

#include <cstdint>

#include "board.hpp"
#include "types.hpp"

namespace libbrickbreaker {

class Game {
 public:
  static constexpr std::int32_t STATE_NONE = 0;
  static constexpr std::int32_t STATE_PLAYING = 1;
  static constexpr std::int32_t STATE_DEATH = 3;
  static constexpr std::int32_t STATE_FINISHEDLEVEL = 4;
  static constexpr std::int32_t STATE_GAMEOVER = 5;

  static constexpr std::int32_t KEY_P = 80;
  static constexpr std::int32_t KEY_F = 70;
  static constexpr std::int32_t KEY_T = 84;
  static constexpr std::int32_t KEY_SPACE = 32;
  static constexpr std::int32_t KEY_ESCAPE = 27;
  static constexpr std::int32_t KEY_LEFT = 37;
  static constexpr std::int32_t KEY_RIGHT = 39;

  static std::int32_t targetFPS;

  Game();

  static const ImageAsset* loadBitmap(const char* path);

  void setRenderContext(RenderContext* context);
  void setViewport(std::int32_t width, std::int32_t height);
  void setBackgroundImage(const ImageAsset* image);
  void setSchedulingEnabled(bool enabled);

  Board& boardRef();
  const Board& boardRef() const;

  void frameDone(std::int64_t frameStartMs);
  void displayStoryScreen(std::int32_t storyId);
  void loadSplashData();
  void initializeRuntime();
  void parseField(const char* line, const char* separator);
  LayoutGroup* getLayoutGroup();
  void readConfig();
  const ImageAsset* loadBitmap(LayoutGroup* layoutGroup,
                               const char* key,
                               const char* name,
                               const ImageAsset* fallback);

  void initialize();
  bool isInitialized() const;
  void resize();
  const ImageAsset* loadSplashBitmap() const;
  void loadConfig();

  std::int32_t gameState() const;

  void increasePoints(std::int32_t points);
  void updateHighScore();
  void newGame(std::int32_t level);
  void advanceState();
  void playGame(std::int32_t level);

  void setHighScore(std::int32_t highScore);
  void startTimer();
  void pause();
  void quit();
  void decreaseAmmo();
  void updateTargetFPS();
  void onOptions();
  void decreaseLives();
  void increaseLives();
  void setAmmo(std::int32_t amount);
  std::int32_t ammoCount() const;

  std::int32_t superLevelCount() const;

  void run();
  // Applies pending pointer/trackball input to the paddle (or aims the held ball). This
  // is normally done inside run(); frontends that drive board.update() directly must call
  // this once per frame before updating, otherwise touch/drag input never moves the paddle.
  void applyInput();
  void closeScreen();

  bool touchEvent(const PointerEvent& event);
  bool openProductionBackdoor(std::int32_t keycode);
  bool openDevelopmentBackdoor(std::int32_t keycode);
  bool navigationUnclick(const NavigationEvent& event);
  bool navigationMovement(const NavigationEvent& event);
  bool navigationClick(const NavigationEvent& event);
  bool trackwheelClick(const NavigationEvent& event);
  bool trackwheelRoll(const NavigationEvent& event);
  bool keyControl(char key, std::int32_t status, std::int64_t timeMs);
  bool keyDown(const KeyEvent& event);

  void onUiEngineAttached(bool attached);
  void makeMenu(Menu* menu, std::int32_t instance);
  bool onMenu(std::int32_t instance);
  bool onClose();
  void paint(Graphics* graphics);
  void onObscured();
  void onVisibilityChange(bool visible);

  bool initialized{false};
  bool running{false};
  bool paused{false};
  std::int32_t state{STATE_NONE};
  std::int64_t lastTimeMs{0};

  std::int32_t score{0};
  std::int32_t lives{0};
  std::int32_t ammo{0};
  std::int32_t highScore{0};

  std::int32_t trackBallDelta{0};

 private:
  Board board_{};
  [[maybe_unused]] std::int32_t currentLevel_{1};
  std::int32_t superLevel_{0};

  [[maybe_unused]] std::int32_t targetRenderTimeMs_{1000 / 30};
  [[maybe_unused]] std::int32_t idleTimeMs_{0};
  [[maybe_unused]] std::int64_t frameWindowStartMs_{0};
  [[maybe_unused]] std::int32_t frameWindowCount_{0};
  [[maybe_unused]] std::int64_t renderWindowMs_{0};

  [[maybe_unused]] std::int32_t moveTo_{0};
  [[maybe_unused]] std::int32_t trackingTouchPoint_{0};
  [[maybe_unused]] std::int32_t touchOffset_{0};
  [[maybe_unused]] bool touchTracking_{false};

  [[maybe_unused]] bool showFPSOverlay_{false};
  [[maybe_unused]] bool targetFPSCapEnabled_{true};
  [[maybe_unused]] double lastMeasuredFPS_{0.0};

  [[maybe_unused]] const char* deviceModel_{""};
  [[maybe_unused]] const char* hudText_{""};
  RenderContext* renderContext_{nullptr};
  [[maybe_unused]] std::int32_t viewportWidth_{0};
  [[maybe_unused]] std::int32_t viewportHeight_{0};
  const ImageAsset* backgroundImage_{nullptr};
  bool schedulingEnabled_{true};

  void scheduleNextFrame(std::int32_t delayMs);
  void cancelScheduledFrame();
  std::int64_t nowMs() const;
  PointerEvent extractPointerData(const PointerEvent& event) const;
};

}  // namespace libbrickbreaker
