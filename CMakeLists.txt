cmake_minimum_required(VERSION 3.9)
project(vectrexia VERSION 0.1.0)
enable_language(CXX)

include(TestBigEndian)
include(CTest)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

if (IS_BIG_ENDIAN)
    message(STATUS "System is big endian")
    add_definitions(-D__MSB_FIRST)
else()
    message(STATUS "System is little endian")
endif()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(vectgif)
