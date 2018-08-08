# Enable GTest and GMock
hunter_add_package(GTest)
find_package(GTest CONFIG REQUIRED)
find_package(GMock CONFIG REQUIRED)

include_directories(. ../src)

set(GTEST_SOURCE_FILES
    vectrex_test.cpp
    cartridge_test.cpp
    m6809opcode_test.cpp
    m6809_test.cpp)

add_executable(vectrexia_tests test_runner.cpp ${GTEST_SOURCE_FILES})

target_link_libraries(vectrexia_tests vectrexia_libretro)
target_link_libraries(vectrexia_tests GTest::gtest)
target_link_libraries(vectrexia_tests GMock::main)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(vectrexia_tests ${LIBCXX_LIBRARY})
endif()

enable_testing()

foreach(GTEST_SOURCE_FILE ${GTEST_SOURCE_FILES})
    file(STRINGS ${GTEST_SOURCE_FILE} GTEST_NAMES REGEX ^TEST)
    foreach(GTEST_NAME ${GTEST_NAMES})
        string(REGEX REPLACE ["\) \(,"] ";" GTEST_NAME ${GTEST_NAME})
        list(GET GTEST_NAME 1 GTEST_GROUP_NAME)
        list(GET GTEST_NAME 3 GTEST_NAME)
        add_test(NAME ${GTEST_GROUP_NAME}.${GTEST_NAME}
                COMMAND vectrexia_tests --gtest_filter=${GTEST_GROUP_NAME}.${GTEST_NAME})
    endforeach()
endforeach()

install(TARGETS vectrexia_tests DESTINATION bin)
