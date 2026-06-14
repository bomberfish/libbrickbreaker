cmake_minimum_required(VERSION 3.22.1)

project("brickbreaker")

set(BRICKBREAKER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../../../../../..")

add_library(libbrickbreaker_core STATIC
    "${BRICKBREAKER_ROOT}/src/libbrickbreaker.cpp"
    "${BRICKBREAKER_ROOT}/src/sprites.cpp"
    "${BRICKBREAKER_ROOT}/src/sound.cpp"
    "${BRICKBREAKER_ROOT}/src/sounds.cpp"
    "${BRICKBREAKER_ROOT}/src/bullet.cpp"
    "${BRICKBREAKER_ROOT}/src/paddle.cpp"
    "${BRICKBREAKER_ROOT}/src/pill.cpp"
    "${BRICKBREAKER_ROOT}/src/pills.cpp"
    "${BRICKBREAKER_ROOT}/src/bricks.cpp"
    "${BRICKBREAKER_ROOT}/src/ball.cpp"
    "${BRICKBREAKER_ROOT}/src/util.cpp"
    "${BRICKBREAKER_ROOT}/src/board.cpp"
    "${BRICKBREAKER_ROOT}/src/game.cpp"
)

target_include_directories(libbrickbreaker_core
    PUBLIC
    "${BRICKBREAKER_ROOT}/include"
)

target_compile_features(libbrickbreaker_core PUBLIC cxx_std_20)

add_library(brickbreaker SHARED
    main.cpp
    AndroidOut.cpp
    Renderer.cpp
    AudioEngine.cpp
)

target_include_directories(brickbreaker
    PRIVATE
    "${BRICKBREAKER_ROOT}/include"
)

target_compile_features(brickbreaker PRIVATE cxx_std_20)

# Searches for a package provided by the game activity dependency
find_package(game-activity REQUIRED CONFIG)

# Forces the linker to keep the JNI entry point for GameActivity
set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -u Java_com_google_androidgamesdk_GameActivity_initializeNativeCode")

# Configure libraries CMake uses to link your target library.
target_link_libraries(brickbreaker
    libbrickbreaker_core

    # The game activity
    game-activity::game-activity_static

    # EGL and other dependent libraries required for drawing
    # and interacting with Android system
    EGL
    GLESv3
    OpenSLES
    jnigraphics
    android
    log)
