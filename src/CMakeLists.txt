include_directories(.)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link.T")

set(VECTREXIA_SOURCE
	libretro/libretro.cpp
	vectrexia.cpp
	cartridge.cpp
	m6809_disassemble.cpp
	m6809.cpp
	)

add_library(vectrexia_libretro SHARED ${VECTREXIA_SOURCE})

if(MSVC)
	add_library(vectrexia_libretro_static STATIC ${VECTREXIA_SOURCE})
endif()

set_target_properties(vectrexia_libretro PROPERTIES PREFIX "")
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT}")

set_source_files_properties(libretro/libretro.cpp
        PROPERTIES OBJECT_DEPENDS ${LINKER_SCRIPT})

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")        ## Optimize
endif()
