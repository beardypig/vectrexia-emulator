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

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set gtest properties
ExternalProject_Get_Property(gtest source_dir binary_dir)
set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        #    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
        )
# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/include")

# Download and install GoogleMock
ExternalProject_Add(
        gmock
        URL https://github.com/google/googlemock/archive/release-1.7.0.zip        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gmock
        # Disable install step
        INSTALL_COMMAND ""
)

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gmock)

# Set gmock properties
ExternalProject_Get_Property(gmock source_dir binary_dir)
set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
        #    "INTERFACE_INCLUDE_DIRECTORIES" "${source_dir}/include"
        )
# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/include")

include_directories(. ../src)

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
