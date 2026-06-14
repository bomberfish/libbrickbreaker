# AGENTS.md

C++ reimplementation of BlackBerry's BrickBreaker as a static library. No codegen, no CI, no linter config.

## Build

```bash
cmake -S . -B build          # configure (full: parity_cli + TUI + SDL)
cmake --build build          # compile
```

Two build dirs already exist: `build/` (full) and `build-portable/` (library + parity_cli only, no TUI/SDL).

**CMake options** (all default ON):
- `-DBRICKBREAKER_BUILD_PARITY_CLI=OFF`
- `-DBRICKBREAKER_BUILD_TUI=OFF`
- `-DBRICKBREAKER_BUILD_SDL=OFF`
- `-DBRICKBREAKER_WARNINGS_AS_ERRORS=OFF`

TUI and SDL are silently skipped if ncurses / SDL2 are not installed — `find_package(...QUIET)`.

## Test

The only test suite is the parity CLI (17 behavioral probes, O01–O17):

```bash
ctest --test-dir build                # via CTest
./build/libbrickbreaker_parity_cli   # direct; exits 0 on pass, prints [PASS]/[FAIL] per probe
```

No external test framework (no GoogleTest, Catch2, etc.). All probes are in `src/parity.cpp`.

### Testing quirks

- `Ball::automatedTesting` is a **static bool**. Probes that set it to `true` (bottom rebound instead of drain) must reset it to `false` after use or later probes will break.
- `createSilentGame()` in `parity.cpp` calls `game.setSchedulingEnabled(false)` — required for in-process testing; don't skip it.
- Probes manipulate struct members directly (`ball.x`, `ball.dx`, `board.bricks.cells[row][col]`, etc.) — the API is intentionally inspectable.

## Architecture

- Namespace: `libbrickbreaker` everywhere.
- `Game` owns `Board`; `Board` holds all game objects (`balls[4]`, `lasers[4]`, `paddle`, `pills`, `bricks`).
- **Global/static state:** `Board::WIDTH`, `HEIGHT`, `TILEWIDTH`, `TILEHEIGHT`, `FACTORX`, `FACTORY` are static members updated by `Board::syncMetrics()`. `Ball::speedFactor` and `Ball::automatedTesting` are also static. Be aware of cross-instance contamination.
- `Board::BASE_WIDTH = 189`, `BASE_HEIGHT = 195` — native coordinate space.
- Public API types (`Graphics`, `RenderContext`, `ImageAsset`, `LayoutGroup`, `Menu`) are pure-virtual interfaces in `include/libbrickbreaker/types.hpp`; frontends subclass them.
- `src/util.hpp` is a private internal header — not installed, not part of the public API.
- `src/libbrickbreaker.cpp` is a 5-line anchor file to ensure the static lib has at least one TU.

## Code style (no enforced formatter)

- Use `std::int32_t` / `std::uint8_t` etc. — no bare `int` except in platform/system interfaces.
- `-Wall -Wextra -Wpedantic -Werror` is the de facto style enforcement (WARNINGS_AS_ERRORS=ON by default).
- `[[maybe_unused]]` on private members is common to suppress warnings.

## Level data

`levels.bin` is binary: first byte = number of levels, then 70 bytes per level (10 rows × 7 cols). Each byte: low nibble = brick strength, high nibble = bonus flag. TUI/SDL frontends accept `--levels <path>`. Without it, `Bricks::generateLevelBytes()` produces deterministic procedural levels.

## Frontends

| Target | Binary | Dep |
|---|---|---|
| `libbrickbreaker_parity_cli` | `build/libbrickbreaker_parity_cli` | none |
| `brickbreaker-curses` | TUI | ncurses |
| `brickbreaker-sdl` | SDL2 graphical | SDL2 |
| Android app | `frontends/android/` | Android SDK + NDK; `local.properties` required, git-ignored |

Android: app ID `ca.bomberfish.brickbreaker`, minSdk=21, uses `GameActivity` (not standard `Activity`), OpenGL ES 3.
