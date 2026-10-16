#
# Emulator core source files (everything except the libretro glue)
#
set(VECTREXIA_CORE_SOURCE
	vectrexia.cpp
	cartridge.cpp
	m6809_disassemble.cpp
	m6809.cpp
	via6522.cpp
	ay38910.cpp
	vectorizer.cpp gfxutil.h
	debugfont.cpp)

# vectrexia_core
# The emulator itself; a static library linked by the libretro core, the
# tests and vectgif. Position independent so it can go into the shared library.
#
add_library(vectrexia_core STATIC ${VECTREXIA_CORE_SOURCE})
target_include_directories(vectrexia_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
set_target_properties(vectrexia_core PROPERTIES
	POSITION_INDEPENDENT_CODE ON
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON)

# vectrexia_libretro
# Main target; a shared library (.so / .dll) that only exports the retro_*
# functions (marked with RETRO_API in libretro.h); everything else is hidden.
#
add_library(vectrexia_libretro SHARED libretro/libretro.cpp)
target_link_libraries(vectrexia_libretro PRIVATE vectrexia_core)
set_target_properties(vectrexia_libretro PROPERTIES
	PREFIX ""
	CXX_VISIBILITY_PRESET hidden
	VISIBILITY_INLINES_HIDDEN ON)

# Definitions are on the core and PUBLIC, so everything that links it (the
# libretro core, the tests and vectgif) is compiled with the same settings.
if (IS_BIG_ENDIAN)
	target_compile_definitions(vectrexia_core PUBLIC __MSB_FIRST)
endif()

target_compile_definitions(vectrexia_core PUBLIC $<$<CONFIG:Debug>:VECTREXIA_DEBUG>)
