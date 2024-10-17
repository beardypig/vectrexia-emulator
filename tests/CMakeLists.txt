find_package(Threads REQUIRED)

# Enable vcpkg manifest mode
set(VCPKG_FEATURE_FLAGS "manifests")

# Find testing and mocking libraries
find_package(Catch2 CONFIG REQUIRED)
find_package(trompeloeil CONFIG REQUIRED)

include_directories(. ../src)

# Define source files for the tests
set(TEST_SOURCES m6809opcode_test.cpp m6809_test.cpp cartridge_test.cpp gfxutil_test.cpp)

# Define the tests executable
add_executable(tests test_runner.cpp ${TEST_SOURCES})

if (MSVC)
	set(LIBRETRO_SRC vectrexia_libretro_static)
else()
	set(LIBRETRO_SRC vectrexia_libretro)
endif()

# Link the tests target with the required libraries
target_link_libraries(tests PRIVATE ${LIBRETRO_SRC})
target_link_libraries(tests PRIVATE Catch2::Catch2 Catch2::Catch2WithMain)
target_link_libraries(tests PRIVATE trompeloeil::trompeloeil Threads::Threads)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(tests PRIVATE ${LIBCXX_LIBRARY})
endif()

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
