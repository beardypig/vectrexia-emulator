# Enable GTest and GMock
find_package(Threads REQUIRED)
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)
include(GoogleTest)

include_directories(. ../src)

set(GTEST_SOURCE m6809opcode_test.cpp m6809_test.cpp cartridge_test.cpp)

add_executable(vectrexia_tests test_runner.cpp ${GTEST_SOURCE})

target_link_libraries(vectrexia_tests vectrexia_libretro)
target_link_libraries(vectrexia_tests GTest::gtest)
target_link_libraries(vectrexia_tests GMock::main)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(vectrexia_tests ${LIBCXX_LIBRARY})
endif()

gtest_add_tests(vectrexia_tests SOURCES ${GTEST_SOURCE})

install(TARGETS vectrexia_tests DESTINATION bin)
