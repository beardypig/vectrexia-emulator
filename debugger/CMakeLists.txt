#set(CURSES_USE_NCURSES TRUE)
find_package(Curses REQUIRED)

add_executable(vectrexia_debug main.cpp window.cpp window.h debugger.cpp debugger.h)

include_directories(../src/)
include_directories(${CURSES_INCLUDE_DIR})

target_link_libraries(vectrexia_debug
        vectrexia_libretro
        ${CURSES_LIBRARIES})
