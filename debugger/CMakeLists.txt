find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(imgui CONFIG REQUIRED)
find_package(glad CONFIG REQUIRED)
find_package(plog CONFIG REQUIRED)

add_executable(vectdebug "main.cpp" "ui/application.cpp")

include_directories(../src)

if (MSVC)
    set(LIBRETRO_SRC vectrexia_libretro_static)
else()
    set(LIBRETRO_SRC vectrexia_libretro)
endif()

target_link_libraries(vectdebug PRIVATE ${LIBRETRO_SRC})
target_link_libraries(vectdebug PRIVATE cxxopts::cxxopts)
target_link_libraries(vectdebug PRIVATE fmt::fmt)
target_link_libraries(vectdebug PRIVATE imgui::imgui)
target_link_libraries(vectdebug PRIVATE glad::glad)
target_link_libraries(vectdebug PRIVATE plog::plog)
target_link_libraries(vectdebug
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)
