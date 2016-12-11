# Google Test and Google Mock external project
# ref: http://www.kaizou.org/2014/11/gtest-cmake/

# We need thread support
find_package(Threads REQUIRED)

# Enable ExternalProject CMake module
include(ExternalProject)

# Download and install GoogleTest
ExternalProject_Add(
        gtest
        URL https://github.com/google/googletest/archive/release-1.8.0.zip
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
        # Disable install step
        INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/gtest/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/googlemock/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        )

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories(
        "${source_dir}/googletest/include"
        "${source_dir}/googlemock/include"
        .
        ../src)

set(GTEST_SOURCE_FILES
    vectrex_test.cpp
    cartridge_test.cpp
    m6809opcode_test.cpp
    m6809_test.cpp)

add_executable(vectrexia_tests test_runner.cpp ${GTEST_SOURCE_FILES})

target_link_libraries(vectrexia_tests
        vectrexia_libretro
        libgmock
        libgtest)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(vectrexia_tests ${LIBCXX_LIBRARY})
endif()


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
