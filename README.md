# libbrickbreaker

A from-scratch C++ reimplementation of the BlackBerry "BrickBreaker" game,
packaged as a portable static library with multiple ready-to-run frontends:

| Frontend       | Target                                  | Path                  |
| -------------- | --------------------------------------- | --------------------- |
| `parity_cli`   | Headless behavioural test runner        | `frontends/parity_cli/` |
| `tui`          | Curses TUI (color, real-time)           | `frontends/tui/`      |
| `sdl`          | SDL2 desktop port (native or Emscripten) | `frontends/sdl/`      |
| `android`      | Android GameActivity / OpenGL ES 3       | `frontends/android/`  |

The SDL frontend ships with sprite rendering, HUD, pause / game-over overlays,
audio via SDL2_mixer and a single-source Emscripten port that runs the same
code in the browser.

## Build (desktop)

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build           # runs the 17 parity probes
```

Targets produced under `build/`:

- `libbrickbreaker.a` — static library
- `libbrickbreaker_parity_cli` — behavioural probe runner
- `frontends/tui/brickbreaker-curses` — TUI (needs `ncurses`)
- `frontends/sdl/brickbreaker-sdl` — SDL2 desktop client (needs `SDL2`, optional `SDL2_mixer`)

The TUI / SDL targets are silently skipped if their dependencies are missing.

### Optional CMake flags

```
-DBRICKBREAKER_BUILD_PARITY_CLI=ON|OFF   (default ON)
-DBRICKBREAKER_BUILD_TUI=ON|OFF          (default ON)
-DBRICKBREAKER_BUILD_SDL=ON|OFF          (default ON)
-DBRICKBREAKER_WARNINGS_AS_ERRORS=ON|OFF (default ON)
```

## Build (WebAssembly / Emscripten)

The SDL frontend doubles as the web port. The build pulls in `USE_SDL=2` and
`USE_SDL_MIXER=2` from the Emscripten ports cache and embeds the contents of
`assets/` into the WASM filesystem via `--preload-file`.

```bash
source /path/to/emsdk/emsdk_env.sh        # activate emcc
emcmake cmake -S . -B build-web
cmake --build build-web
```

Outputs (under `build-web/frontends/sdl/`):

```
brickbreaker.html   # entry point (uses the custom shell.html template)
brickbreaker.js     # WASM bootstrap
brickbreaker.wasm   # core library + frontend
brickbreaker.data   # preloaded sprites / sounds / levels.bin
```

Serve the directory with any static file server and open `brickbreaker.html`:

```bash
python3 -m http.server --directory build-web/frontends/sdl 8000
open http://localhost:8000/brickbreaker.html
```

## Build (Android)

```bash
cd frontends/android
./gradlew assembleDebug
```

`local.properties` is required (git-ignored). The Android build re-compiles
the libbrickbreaker core sources directly via its embedded CMake, and reads
sprites / sounds / `levels.bin` from the top-level `assets/` directory shared
with the SDL and web frontends.

## Controls (SDL & web)

| Input                       | Action                       |
| --------------------------- | ---------------------------- |
| `←` / `→`, `A` / `D`        | Move paddle                  |
| `Space`                     | Launch ball / shoot          |
| `P`                         | Pause / resume               |
| `R`                         | Restart                      |
| `Esc` / `Q`                 | Quit (desktop only)          |
| Mouse / touch drag          | Drag the paddle              |
| Click / tap                 | Shoot                        |

TUI bindings are similar (`A`/`D`/`H`/`L`/arrows for movement, `Space` shoot,
`P` pause, `R` reset, `Q` or `Esc` to quit).

## Assets

All sprites, sound effects and the level data live in the top-level `assets/`
directory and are shared between the SDL, web and Android frontends:

```
assets/
├── levels.bin            # 34 levels × 70 bytes (plus 1-byte header)
├── sprites/              # ball / brick / paddle / pill / laser / bomb sheets
├── sounds/               # 8 OGG sound effects (matches Sounds::SOUND_* ids)
├── ui/                   # bg / splash / you_lose / trophy
└── fonts/                # DejaVu Sans (reserved for future text rendering)
```

The SDL build copies `assets/` next to the binary on every build so the
binary works from anywhere on the build tree. Use `--levels <path>` and
`--assets <dir>` CLI flags to override the defaults.

## Library public API

The umbrella include is `include/libbrickbreaker/libbrickbreaker.hpp`.
Frontends interact with the engine through:

- `Game` — top-level state machine (`newGame`, `pause`, `advanceState`,
  `keyDown`, `touchEvent`, `boardRef`)
- `Board` — owns the paddle, four-ball pool, pills, bricks, laser pool and
  bomb. All fields are public for direct inspection by frontends.
- `Bricks::setLevelData(bytes, len)` — load an alternative level pack.
- `Sound::setPlayCallback(cb)` — frontends register a function pointer so the
  core can request sound playback (this is what the SDL + Android audio
  engines hook into).

## Install

```bash
cmake -S . -B build -DBRICKBREAKER_BUILD_TUI=OFF -DBRICKBREAKER_BUILD_SDL=OFF
cmake --build build
cmake --install build --prefix /your/install/prefix
```

Headers are installed under `include/` and libraries under the platform's
`lib` directory.
