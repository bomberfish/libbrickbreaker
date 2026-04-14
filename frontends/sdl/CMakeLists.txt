find_package(SDL2 QUIET)

if(NOT SDL2_FOUND)
  message(STATUS "SDL2 not found; skipping SDL frontend")
  return()
endif()

add_executable(brickbreaker-sdl
  main.cpp
)

target_link_libraries(brickbreaker-sdl
  PRIVATE
    libbrickbreaker
    SDL2::SDL2
)

if(TARGET SDL2::SDL2main)
  target_link_libraries(brickbreaker-sdl PRIVATE SDL2::SDL2main)
endif()
