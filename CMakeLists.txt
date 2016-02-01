cmake_minimum_required(VERSION 3.9)
include("${CMAKE_SOURCE_DIR}/cmake/HunterGate.cmake")
# Enable ExternalProject CMake module
HunterGate(
        URL "https://github.com/ruslo/hunter/archive/v0.23.11.tar.gz"
        SHA1 "b732407d782459f89950ab52bc0ff9612f3e4be2"
)

project(vectrexia VERSION 0.1.0)
enable_language(CXX)

include(TestBigEndian)
include(CTest)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    # minimum version of macos needed for c++14
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
endif()

# include custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

if (IS_BIG_ENDIAN)
    add_definitions(-D__MSB_FIRST)
endif()

enable_testing()
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(vectgif)
