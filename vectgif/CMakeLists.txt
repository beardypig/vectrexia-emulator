find_package(cxxopts CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)

add_executable(vectgif main.cpp)

include_directories(../src)

target_link_libraries(vectgif PRIVATE vectrexia_core)
target_link_libraries(vectgif PRIVATE cxxopts::cxxopts)
target_link_libraries(vectgif PRIVATE fmt::fmt)
