include_directories(.)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link.T")

#
# Locate the clang-tidy executable and set variable CLANG_TIDY_EXE
#
find_program(
  CLANG_TIDY_EXE
  NAMES "clang-tidy"
  DOC "Path to clang-tidy executable"
  )
if(NOT CLANG_TIDY_EXE)
  message(STATUS "clang-tidy not found.")
else()
  message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
  set(DO_CLANG_TIDY "${CLANG_TIDY_EXE}" "-checks=*,-clang-analyzer-alpha.*")
endif()

#
# Vectrexia target source files
#
set(VECTREXIA_SOURCE
        libretro/libretro.cpp
        vectrexia.cpp
        cartridge.cpp
        m6809_disassemble.cpp
        m6809.cpp
        via6522.cpp updatetimer.h ay38910.cpp ay38910.h)

# vectrexia_libretro
# Main target; a shared library (.so / .dll)
#
add_library(vectrexia_libretro SHARED ${VECTREXIA_SOURCE})

# vectrexia_libretro_static
# Extra MSVC target; static library needed for compiling the tests
# on windows using Visual Studio.
#
if(MSVC)
	add_library(vectrexia_libretro_static STATIC ${VECTREXIA_SOURCE})
endif()

set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT}")
set_target_properties(vectrexia_libretro PROPERTIES PREFIX "")
set_source_files_properties(libretro/libretro.cpp
        PROPERTIES OBJECT_DEPENDS ${LINKER_SCRIPT})

# vectrexia_libretro_tidy
# A third target for compiling vectrexia_libretro with clang and
# running clang-tidy to check for static analysis errors and warnings
#
if(CLANG_TIDY_EXE)
	add_library(vectrexia_libretro_tidy STATIC ${VECTREXIA_SOURCE})
	if (MSVC)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++14")
	endif()

	set (CLANG_TIDY_CHECKS "-checks=*,-clang-analyzer-alpha.*")
	set (DO_CLANG_TIDY, "${DO_CLANG_TIDY} ${CLANG_TIDY_CHECKS}")

	set_target_properties(
		vectrexia_libretro_tidy PROPERTIES
		CXX_CLANG_TIDY "${DO_CLANG_TIDY}"
	)
endif()

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")        ## Optimize
endif()
