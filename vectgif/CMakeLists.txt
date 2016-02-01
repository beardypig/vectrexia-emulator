add_executable(vectgif main.cpp gif.h)

include_directories(../src/)

target_link_libraries(vectgif
        vectrexia_libretro)
