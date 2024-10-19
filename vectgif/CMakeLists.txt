find_package(cxxopts CONFIG REQUIRED)

add_executable(vectgif main.cpp)

include_directories(../src)

if (MSVC)
    set(LIBRETRO_SRC vectrexia_libretro_static)
else()
    set(LIBRETRO_SRC vectrexia_libretro)
endif()

target_link_libraries(vectgif PRIVATE ${LIBRETRO_SRC})
target_link_libraries(vectgif PRIVATE cxxopts::cxxopts)
