cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CONFIGURATION_TYPES "Debug;DebugEditor;Develop;DevelopEditor;Shipping" CACHE STRING "Recast Navigation Project Configurations" FORCE)

project(RecastNavigation LANGUAGES C CXX)

#Pigeon Engine Third Party Recast Navigation
#Source copy of https://github.com/recastnavigation/recastnavigation (commit 9f4ce64).
#Only the runtime modules are vendored. RecastDemo (SDL2 + OpenGL), Tests (Catch2) and
#DebugUtils are left out: the engine draws navigation debug geometry through its own
#wireframe manager, so none of them is needed to build or query a navmesh. The one
#RecastDemo piece that runtime work does need is its FastLZ compression, vendored from
#the same upstream repository (RecastDemo/Contrib/fastlz) as FastLZ/ here.

#Every module builds as its own static library, mirroring the upstream layout: refreshing the
#copy is then a per-directory operation, and a module can be linked or dropped on its own.
#Sources are picked up from the module directory instead of being listed, so a newer upstream
#copy only needs a CMake reconfigure. All four targets live in the
#ThirdParty/RecastNavigation solution folder so the solution root stays readable.
#
#InTargetName: CMake target to create, also the module directory name.
#InLinkTarget: target this module depends on, empty for none.
function(PigeonRecastNavigationModule InTargetName InLinkTarget)
	file(GLOB MODULE_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/${InTargetName}/Source/*.cpp)
	file(GLOB MODULE_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/${InTargetName}/Include/*.h)
	add_library(${InTargetName} STATIC ${MODULE_SOURCES} ${MODULE_HEADERS})
	#Static runtime, configuration names and warning policy follow the engine target, so
	#anything linked into PigeonEngine.exe shares one CRT instance.
	set_target_properties(${InTargetName} PROPERTIES
		SUFFIX ".lib"
		CXX_STANDARD 98
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
		FOLDER "ThirdParty/RecastNavigation"
	)
	#The upstream sources include their own headers by bare file name ("Recast.h"), so the
	#module's Include directory has to be on the path. The repository root is added as well
	#so engine code can include the same header as <Recast/Include/Recast.h>, which keeps a
	#third party include recognizable at a glance - the engine code style asks for that.
	target_include_directories(${InTargetName} PUBLIC
		${CMAKE_CURRENT_SOURCE_DIR}/${InTargetName}/Include
		${CMAKE_CURRENT_SOURCE_DIR}
	)
	target_compile_options(${InTargetName} PRIVATE
		$<$<CONFIG:Debug>:/MTd>
		$<$<CONFIG:DebugEditor>:/MTd>
		$<$<CONFIG:Develop>:/MTd>
		$<$<CONFIG:DevelopEditor>:/MTd>
		$<$<CONFIG:Shipping>:/MT>
		$<$<CONFIG:Debug>:/Zi>
		$<$<CONFIG:DebugEditor>:/Zi>
		$<$<CONFIG:Develop>:/Zi>
		$<$<CONFIG:DevelopEditor>:/Zi>
		$<$<CONFIG:Shipping>:/Zi>
	)
	if(MSVC)
		#Warnings are reported, not fatal: this is upstream code and the engine builds
		#with /W4 /WX only for its own sources.
		target_compile_options(${InTargetName} PRIVATE
			/nologo
			/diagnostics:classic
			/sdl
			/MP
			/W3
			/WX-
			/Gm-
			/EHsc
			/Zc:wchar_t
			/Zc:forScope
			/Zc:inline
			/Gd
			/FC
		)
	endif()
	#RC_DISABLE_ASSERTS is read by both RecastAssert.h and DetourAssert.h. The macro has
	#to be defined identically for the libraries and for every translation unit that
	#includes those headers, otherwise the inline assertion code differs per object file.
	#Shipping is the only configuration where the engine itself defines NDEBUG, so it is
	#the only one that disables the library assertions.
	target_compile_definitions(${InTargetName} PUBLIC
		$<$<CONFIG:Shipping>:RC_DISABLE_ASSERTS>
	)
	target_compile_definitions(${InTargetName} PRIVATE
		$<$<CONFIG:Shipping>:NDEBUG>
	)
	if(NOT "${InLinkTarget}" STREQUAL "")
		target_link_libraries(${InTargetName} PUBLIC ${InLinkTarget})
	endif()
	foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
		string(TOUPPER ${CONFIG} CONFIG_UPPER)
		set_target_properties(${InTargetName} PROPERTIES
			ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER}        ${PROJECT_BINARY_DIR}/Lib/${CONFIG_UPPER}/
			PDB_OUTPUT_DIRECTORY_${CONFIG_UPPER}            ${PROJECT_BINARY_DIR}/Lib/${CONFIG_UPPER}/
			COMPILE_PDB_OUTPUT_DIRECTORY_${CONFIG_UPPER}    ${PROJECT_BINARY_DIR}/Lib/${CONFIG_UPPER}/
		)
	endforeach()
endfunction()

#Recast: triangle geometry to navmesh
PigeonRecastNavigationModule(Recast "")

#Detour: navmesh storage and path queries
PigeonRecastNavigationModule(Detour "")

#DetourCrowd: local steering and agent avoidance
PigeonRecastNavigationModule(DetourCrowd Detour)

#DetourTileCache: dynamically rebuilt navmesh tiles
PigeonRecastNavigationModule(DetourTileCache Detour)

#FastLZ: the compression DetourTileCache layers go through. Pure C and a single file
#pair, so it does not go through the module factory (which globs .cpp); the public
#include directory is the repository root, so engine code writes <FastLZ/fastlz.h>.
add_library(FastLZ STATIC
	${CMAKE_CURRENT_SOURCE_DIR}/FastLZ/fastlz.c
	${CMAKE_CURRENT_SOURCE_DIR}/FastLZ/fastlz.h)
set_target_properties(FastLZ PROPERTIES
	SUFFIX ".lib"
	FOLDER "ThirdParty/RecastNavigation"
)
target_include_directories(FastLZ PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
#Static runtime to match the engine target and the Recast modules: one CRT instance
#across everything linked into PigeonEngine.exe.
target_compile_options(FastLZ PRIVATE
	$<$<CONFIG:Debug>:/MTd>
	$<$<CONFIG:DebugEditor>:/MTd>
	$<$<CONFIG:Develop>:/MTd>
	$<$<CONFIG:DevelopEditor>:/MTd>
	$<$<CONFIG:Shipping>:/MT>
)
target_link_libraries(DetourTileCache PUBLIC FastLZ)
