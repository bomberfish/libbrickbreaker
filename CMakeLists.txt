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
)

target_include_directories(libbrickbreaker
  PRIVATE
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
