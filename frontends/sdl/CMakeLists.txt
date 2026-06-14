# SDL2 frontend, native and Emscripten.

set(_BB_SDL_SOURCES
  main.cpp
  stb_image_impl.cpp
)

# stb_image is third-party public-domain code that does not compile clean under
# our strict warning policy. Drop all warnings for that single TU.
if(MSVC)
  set_source_files_properties(stb_image_impl.cpp PROPERTIES COMPILE_OPTIONS "/w")
else()
  set_source_files_properties(stb_image_impl.cpp PROPERTIES COMPILE_OPTIONS "-w")
endif()

if(EMSCRIPTEN)
  add_executable(brickbreaker-sdl ${_BB_SDL_SOURCES})
  target_link_libraries(brickbreaker-sdl PRIVATE libbrickbreaker)

  # Emscripten ships its own SDL2 / SDL2_mixer ports; let the toolchain handle
  # the linkage and we just opt in via -s flags.
  set(_BB_EMSCRIPTEN_LINK_FLAGS
      "-sUSE_SDL=2"
      "-sUSE_SDL_MIXER=2"
      "-sSDL2_MIXER_FORMATS=[\"ogg\"]"
      "-sALLOW_MEMORY_GROWTH=1"
      "-sINITIAL_MEMORY=33554432"
      "-sEXIT_RUNTIME=0"
      "-sASYNCIFY=0"
      "-sFORCE_FILESYSTEM=1"
      "--preload-file=${CMAKE_SOURCE_DIR}/assets@/assets"
      "--shell-file=${CMAKE_CURRENT_SOURCE_DIR}/shell.html")

  set(_BB_EMSCRIPTEN_COMPILE_FLAGS
      "-sUSE_SDL=2"
      "-sUSE_SDL_MIXER=2"
      "-sSDL2_MIXER_FORMATS=[\"ogg\"]")

  target_compile_options(brickbreaker-sdl PRIVATE ${_BB_EMSCRIPTEN_COMPILE_FLAGS})
  target_link_options(brickbreaker-sdl PRIVATE ${_BB_EMSCRIPTEN_LINK_FLAGS})

  set_target_properties(brickbreaker-sdl PROPERTIES
      SUFFIX ".html"
      OUTPUT_NAME "brickbreaker")

  target_compile_features(brickbreaker-sdl PRIVATE cxx_std_20)
  brickbreaker_apply_warnings(brickbreaker-sdl)
  return()
endif()

find_package(SDL2 QUIET)
if(NOT SDL2_FOUND)
  message(STATUS "SDL2 not found; skipping SDL frontend")
  return()
endif()

add_executable(brickbreaker-sdl ${_BB_SDL_SOURCES})
target_link_libraries(brickbreaker-sdl PRIVATE libbrickbreaker)

if(TARGET SDL2::SDL2)
  target_link_libraries(brickbreaker-sdl PRIVATE SDL2::SDL2)
elseif(TARGET SDL2::SDL2-static)
  target_link_libraries(brickbreaker-sdl PRIVATE SDL2::SDL2-static)
elseif(SDL2_LIBRARIES)
  target_include_directories(brickbreaker-sdl PRIVATE ${SDL2_INCLUDE_DIRS})
  target_link_libraries(brickbreaker-sdl PRIVATE ${SDL2_LIBRARIES})
else()
  message(STATUS "No usable SDL2 target exported; skipping SDL frontend")
  return()
endif()

if(TARGET SDL2::SDL2main)
  target_link_libraries(brickbreaker-sdl PRIVATE SDL2::SDL2main)
endif()

# Optional SDL2_mixer for native builds. Skip silently if not found.
find_package(SDL2_mixer QUIET)
if(SDL2_mixer_FOUND OR SDL2MIXER_FOUND OR TARGET SDL2_mixer::SDL2_mixer OR TARGET SDL2::mixer)
  if(TARGET SDL2_mixer::SDL2_mixer)
    target_link_libraries(brickbreaker-sdl PRIVATE SDL2_mixer::SDL2_mixer)
  elseif(TARGET SDL2::mixer)
    target_link_libraries(brickbreaker-sdl PRIVATE SDL2::mixer)
  elseif(SDL2_MIXER_LIBRARIES)
    target_include_directories(brickbreaker-sdl PRIVATE ${SDL2_MIXER_INCLUDE_DIRS})
    target_link_libraries(brickbreaker-sdl PRIVATE ${SDL2_MIXER_LIBRARIES})
  endif()
  message(STATUS "SDL2_mixer found; enabling audio for SDL frontend")
else()
  # Fall back to a manual lookup using pkg-config (Homebrew on macOS exposes
  # SDL2_mixer via the system include paths, but no CMake config is shipped).
  find_path(SDL2_MIXER_INCLUDE_DIR SDL_mixer.h
            HINTS ENV SDL2MIXER_DIR
            PATH_SUFFIXES SDL2 SDL2/SDL2_mixer)
  find_library(SDL2_MIXER_LIBRARY NAMES SDL2_mixer)
  if(SDL2_MIXER_INCLUDE_DIR AND SDL2_MIXER_LIBRARY)
    target_include_directories(brickbreaker-sdl PRIVATE ${SDL2_MIXER_INCLUDE_DIR})
    target_link_libraries(brickbreaker-sdl PRIVATE ${SDL2_MIXER_LIBRARY})
    message(STATUS "SDL2_mixer found via manual lookup at ${SDL2_MIXER_LIBRARY}")
  else()
    message(STATUS "SDL2_mixer not found; SDL frontend audio will be disabled")
  endif()
endif()

target_compile_features(brickbreaker-sdl PRIVATE cxx_std_20)
brickbreaker_apply_warnings(brickbreaker-sdl)

# Convenience: copy assets next to the binary so the binary works when run from
# anywhere on the build tree.
add_custom_command(TARGET brickbreaker-sdl POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          "${CMAKE_SOURCE_DIR}/assets"
          "$<TARGET_FILE_DIR:brickbreaker-sdl>/assets"
  COMMENT "Mirroring assets/ next to the SDL binary")
