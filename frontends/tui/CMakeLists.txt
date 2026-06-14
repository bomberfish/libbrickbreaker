find_package(Curses QUIET)

if(NOT CURSES_FOUND)
  message(STATUS "Curses not found; skipping TUI frontend")
  return()
endif()

add_executable(brickbreaker-curses
  main.cpp
)

target_link_libraries(brickbreaker-curses
  PRIVATE
    libbrickbreaker
)

if(TARGET Curses::Curses)
  target_link_libraries(brickbreaker-curses PRIVATE Curses::Curses)
else()
  target_include_directories(brickbreaker-curses PRIVATE ${CURSES_INCLUDE_DIRS})
  target_link_libraries(brickbreaker-curses PRIVATE ${CURSES_LIBRARIES})
endif()

if(TARGET tinfo)
  target_link_libraries(brickbreaker-curses PRIVATE tinfo)
endif()

target_compile_features(brickbreaker-curses PRIVATE cxx_std_20)
brickbreaker_apply_warnings(brickbreaker-curses)
