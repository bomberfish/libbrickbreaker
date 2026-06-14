cmake_minimum_required(VERSION 3.21)

project(
  libbrickbreaker
  VERSION 0.1.0
  DESCRIPTION "Core Brick Breaker game library"
  LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

option(BRICKBREAKER_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BRICKBREAKER_BUILD_PARITY_CLI "Build parity CLI frontend" ON)
option(BRICKBREAKER_BUILD_TUI "Build ncurses TUI frontend when dependencies are available" ON)
option(BRICKBREAKER_BUILD_SDL "Build SDL frontend when dependencies are available" ON)

# When targeting Emscripten the only meaningful frontend is the SDL one; force
# the others off so we never try to find ncurses / register native CTest runs.
if(EMSCRIPTEN)
  set(BRICKBREAKER_BUILD_TUI OFF CACHE BOOL "" FORCE)
  set(BRICKBREAKER_BUILD_PARITY_CLI OFF CACHE BOOL "" FORCE)
endif()

function(brickbreaker_apply_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
    if(BRICKBREAKER_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    if(BRICKBREAKER_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()

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

set_target_properties(libbrickbreaker PROPERTIES PREFIX "")
target_compile_features(libbrickbreaker PUBLIC cxx_std_20)
brickbreaker_apply_warnings(libbrickbreaker)

target_include_directories(libbrickbreaker
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

if(BRICKBREAKER_BUILD_PARITY_CLI)
  add_executable(libbrickbreaker_parity_cli
    frontends/parity_cli/main.cpp
  )

  target_link_libraries(libbrickbreaker_parity_cli
    PRIVATE
      libbrickbreaker
  )
  target_compile_features(libbrickbreaker_parity_cli PRIVATE cxx_std_20)
  brickbreaker_apply_warnings(libbrickbreaker_parity_cli)

  add_test(
    NAME libbrickbreaker_parity
    COMMAND libbrickbreaker_parity_cli
  )
endif()

if(BRICKBREAKER_BUILD_TUI)
  add_subdirectory(frontends/tui)
endif()

if(BRICKBREAKER_BUILD_SDL)
  add_subdirectory(frontends/sdl)
endif()

install(
  TARGETS libbrickbreaker
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
  DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
