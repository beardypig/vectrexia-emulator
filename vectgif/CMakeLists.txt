find_package(GIFLIB REQUIRED)

add_executable(vectgif main.cpp)

include_directories(../src/)

target_link_libraries(vectgif
        vectrexia_libretro
        ${CURSES_LIBRARIES})
