# Enable GTest and GMock
find_package(Threads REQUIRED)
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)
include(GoogleTest)

include_directories(. ../src)

set(GTEST_SOURCE m6809opcode_test.cpp m6809_test.cpp cartridge_test.cpp)

add_executable(tests test_runner.cpp ${GTEST_SOURCE})

if (MSVC)
	set(LIBRETRO_SRC vectrexia_libretro_static)
else()
	set(LIBRETRO_SRC vectrexia_libretro)
endif()

target_link_libraries(tests ${LIBRETRO_SRC})
target_link_libraries(tests GTest::gtest)
target_link_libraries(tests GMock::main)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(tests ${LIBCXX_LIBRARY})
endif()

gtest_add_tests(tests SOURCES ${GTEST_SOURCE})

install(TARGETS tests DESTINATION bin)

# enable coverage
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