include_directories(.)

add_library(vectrexia_libretro SHARED
        libretro/libretro.cpp
        vectrexia.cpp
        cartridge.cpp)

set_target_properties(vectrexia_libretro PROPERTIES PREFIX "")
