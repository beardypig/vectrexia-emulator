add_executable(vectgif
        main.cpp
        gif.h
        getopt.h)

include_directories(../src)

if (MSVC)
    set(LIBRETRO_SRC vectrexia_libretro_static)
else()
    set(LIBRETRO_SRC vectrexia_libretro)
endif()

target_link_libraries(vectgif ${LIBRETRO_SRC})
