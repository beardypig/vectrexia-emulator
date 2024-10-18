find_package(Threads REQUIRED)

# Enable vcpkg manifest mode
set(VCPKG_FEATURE_FLAGS "manifests")

# Find testing and mocking libraries
find_package(Catch2 CONFIG REQUIRED)
find_package(trompeloeil CONFIG REQUIRED)

include_directories(. ../src)

# Define the tests executable
add_executable(tests m6809opcode_test.cpp m6809_test.cpp cartridge_test.cpp gfxutil_test.cpp)

if (MSVC)
    set(LIBRETRO_SRC vectrexia_libretro_static)
else()
    set(LIBRETRO_SRC vectrexia_libretro)
endif()

# Link the tests target with the required libraries
target_link_libraries(tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)
target_link_libraries(tests PRIVATE trompeloeil::trompeloeil Threads::Threads)
target_link_libraries(tests PRIVATE ${LIBRETRO_SRC})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(tests PRIVATE ${LIBCXX_LIBRARY})
endif()

add_test(NAME OpcodeTest COMMAND tests "[m6809]")
add_test(NAME CartridgeTest COMMAND tests "[cartridge]")
add_test(NAME FlagsTest COMMAND tests "[flags]")
add_test(NAME M6809Test COMMAND tests "[m6809]")

# Enable coverage (optional)
if ("${COVERAGE}" STREQUAL "1")
    include("${CMAKE_SOURCE_DIR}/cmake/CodeCoverage.cmake")
    APPEND_COVERAGE_COMPILER_FLAGS()
    set(COVERAGE_LCOV_EXCLUDES 'tests/*' 'gmock/*' 'gtest/*' '/usr/*')
    SETUP_TARGET_FOR_COVERAGE_LCOV(
            NAME coverage
            EXECUTABLE tests
            DEPENDENCIES tests
    )
endif()
