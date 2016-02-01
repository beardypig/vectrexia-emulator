cmake_minimum_required(VERSION 2.8)
project(vectrexia)
enable_language(CXX)

include(CheckCXXCompilerFlag)
include(TestBigEndian)

# Test for C++14 support
if(NOT "${CMAKE_CXX_FLAGS}" MATCHES "std=c\\+\\+(1y|14)")
    check_cxx_compiler_flag(-std=c++14 HAS_CXX14)
    if (HAS_CXX14)
        message("-- Compiler supports C++14")
        set(STDFLAG "-std=c++14")
    endif()

    if (NOT STDFLAG)
        check_cxx_compiler_flag(-std=c++1y HAS_CXX1Y)
        if(HAS_CXX1Y)
            message("-- Compiler supports C++1y")
            set(STDFLAG "-std=c++1y")
        endif()
    endif()

    if(NOT STDFLAG)
        message(FATAL_ERROR
                "vectrexia requires a compiler with at least C++1y support")
    endif()

endif()

if(APPLE)
    set(CMAKE_MACOSX_RPATH ON)
    # minimum version of macos needed for c++14
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.7")
endif()

# include custom cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# set CXX flags for different compilers
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${STDFLAG}")

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

TEST_BIG_ENDIAN(IS_BIG_ENDIAN)

if (IS_BIG_ENDIAN)
    add_definitions(-D__MSB_FIRST)
endif()

enable_testing()
add_subdirectory(src)
add_subdirectory(debugger)
add_subdirectory(tests)
add_subdirectory(vectgif)
