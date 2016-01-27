include_directories(.)
set(LINKER_SCRIPT "${CMAKE_SOURCE_DIR}/link.T")

add_library(vectrexia_libretro SHARED
        libretro/libretro.cpp
        vectrexia.cpp
        cartridge.cpp
        m6809_disassemble.cpp
        m6809.cpp
        via6522.cpp
        vectorframebuffer.cpp
        )

set_target_properties(vectrexia_libretro PROPERTIES PREFIX "")
set(CMAKE_EXE_LINKER_FLAGS "-T ${LINKER_SCRIPT}")

set_source_files_properties(libretro/libretro.cpp
        PROPERTIES OBJECT_DEPENDS ${LINKER_SCRIPT})

if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")        ## Optimize
endif()
