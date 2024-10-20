include_directories(.)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link.T")

#
# Vectrexia target source files
#
set(VECTREXIA_SOURCE
	libretro/libretro.cpp
	vectrexia.cpp
	cartridge.cpp
	m6809_disassemble.cpp
	m6809.cpp
    via6522.cpp
    ay38910.cpp
	vectorizer.cpp gfxutil.h
	debugfont.cpp)

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

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DVECTREXIA_DEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVECTREXIA_DEBUG")

if (VECTORIZER_DEBUG)
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -DVECTORIZER_DEBUG")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DVECTORIZER_DEBUG")
endif()