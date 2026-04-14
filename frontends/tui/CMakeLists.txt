add_executable(brickbreaker-curses
  main.cpp
)

target_link_libraries(brickbreaker-curses
  PRIVATE
    libbrickbreaker
    ncursesw
    tinfo
)
