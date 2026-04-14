cmake_minimum_required(VERSION 3.21)

project(
  libbrickbreaker
  VERSION 0.1.0
  DESCRIPTION "Core Brick Breaker game library"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BRICKBREAKER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)

add_library(libbrickbreaker
  src/libbrickbreaker.cpp
  src/sprites.cpp
  src/sound.cpp
  src/sounds.cpp
  src/bullet.cpp
  src/paddle.cpp
  src/pill.cpp
  src/pills.cpp
  src/bricks.cpp
  src/ball.cpp
  src/util.cpp
  src/board.cpp
  src/game.cpp
  src/parity.cpp
)

add_executable(libbrickbreaker_parity_cli
  frontends/parity_cli/main.cpp
)

target_link_libraries(libbrickbreaker_parity_cli
  PRIVATE
    libbrickbreaker
)

set_target_properties(libbrickbreaker PROPERTIES PREFIX "")

target_include_directories(libbrickbreaker
  PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

if(MSVC)
  target_compile_options(libbrickbreaker PRIVATE /W4)
  if(BRICKBREAKER_WARNINGS_AS_ERRORS)
    target_compile_options(libbrickbreaker PRIVATE /WX)
  endif()
else()
  target_compile_options(libbrickbreaker PRIVATE -Wall -Wextra -Wpedantic)
  if(BRICKBREAKER_WARNINGS_AS_ERRORS)
    target_compile_options(libbrickbreaker PRIVATE -Werror)
  endif()
endif()

install(TARGETS libbrickbreaker)

install(
  DIRECTORY ${PROJECT_SOURCE_DIR}/include/
  DESTINATION include
)
