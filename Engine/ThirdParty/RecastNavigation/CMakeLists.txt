cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CONFIGURATION_TYPES "Debug;DebugEditor;Develop;DevelopEditor;Shipping" CACHE STRING "Recast Navigation Project Configurations" FORCE)

project(RecastNavigation LANGUAGES CXX)

#Pigeon Engine Third Party Recast Navigation
#Source copy of https://github.com/recastnavigation/recastnavigation (commit 9f4ce64).
#Only the runtime modules are vendored. RecastDemo (SDL2 + OpenGL), Tests (Catch2) and
#DebugUtils are left out: the engine draws navigation debug geometry through its own
#wireframe manager, so none of them is needed to build or query a navmesh.

#Every module builds as its own static library so the engine can link a subset. The
#target names are the upstream ones, which is also what the Recast documentation uses.
#
#InTargetName: CMake target to create.
#InLinkTarget: target this module depends on, empty for none.
#Remaining arguments: source files. Headers are added for IDE grouping only.
function(PigeonRecastNavigationModule InTargetName InLinkTarget)
	add_library(${InTargetName} STATIC ${ARGN})
	#Static runtime, configuration names and warning policy follow the engine target, so
	#anything linked into PigeonEngine.exe shares one CRT instance.
	set_target_properties(${InTargetName} PROPERTIES
		SUFFIX ".lib"
		CXX_STANDARD 98
		CXX_STANDARD_REQUIRED ON
		CXX_EXTENSIONS OFF
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
PigeonRecastNavigationModule(Recast ""
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/Recast.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastAlloc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastArea.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastAssert.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastContour.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastFilter.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastLayers.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastMeshDetail.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastRasterization.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Source/RecastRegion.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Include/Recast.h
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Include/RecastAlloc.h
	${CMAKE_CURRENT_SOURCE_DIR}/Recast/Include/RecastAssert.h
)

#Detour: navmesh storage and path queries
PigeonRecastNavigationModule(Detour ""
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourAlloc.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourAssert.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourCommon.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourNavMesh.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourNavMeshBuilder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourNavMeshQuery.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Source/DetourNode.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourAlloc.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourAssert.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourCommon.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourMath.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourNavMesh.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourNavMeshBuilder.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourNavMeshQuery.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourNode.h
	${CMAKE_CURRENT_SOURCE_DIR}/Detour/Include/DetourStatus.h
)

#DetourCrowd: local steering and agent avoidance
PigeonRecastNavigationModule(DetourCrowd Detour
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourCrowd.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourLocalBoundary.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourObstacleAvoidance.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourPathCorridor.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourPathQueue.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Source/DetourProximityGrid.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourCrowd.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourLocalBoundary.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourObstacleAvoidance.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourPathCorridor.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourPathQueue.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourCrowd/Include/DetourProximityGrid.h
)

#DetourTileCache: dynamically rebuilt navmesh tiles
PigeonRecastNavigationModule(DetourTileCache Detour
	${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache/Source/DetourTileCache.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache/Source/DetourTileCacheBuilder.cpp
	${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache/Include/DetourTileCache.h
	${CMAKE_CURRENT_SOURCE_DIR}/DetourTileCache/Include/DetourTileCacheBuilder.h
)
