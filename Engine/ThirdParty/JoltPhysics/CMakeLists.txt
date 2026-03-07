cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CONFIGURATION_TYPES "Debug;DebugEditor;Develop;DevelopEditor;Shipping" CACHE STRING "Jolt Project Configurations" FORCE)

#Jolt Project
set(PROJECT_NAME Jolt)

project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE JOLT_PROJECT_HEADERS_H ./Jolt/*.h)
file(GLOB_RECURSE JOLT_PROJECT_HEADERS_HPP ./Jolt/*.hpp)
file(GLOB_RECURSE JOLT_PROJECT_HEADERS_C ./Jolt/*.c)
file(GLOB_RECURSE JOLT_PROJECT_HEADERS_CPP ./Jolt/*.cpp)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${JOLT_PROJECT_HEADERS_H})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${JOLT_PROJECT_HEADERS_HPP})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${JOLT_PROJECT_HEADERS_C})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/ FILES ${JOLT_PROJECT_HEADERS_CPP})

add_library(${PROJECT_NAME} STATIC
	${JOLT_PROJECT_HEADERS_C}
	${JOLT_PROJECT_HEADERS_CPP}
)

target_sources(${PROJECT_NAME} PRIVATE
	${JOLT_PROJECT_HEADERS_H}
	${JOLT_PROJECT_HEADERS_HPP}
	${JOLT_PROJECT_HEADERS_C}
	${JOLT_PROJECT_HEADERS_CPP}
)

#Jolt Build Properties
option(ENABLED_WARNING_AS_ERROR "Enable warning as error" ON)
option(USE_AVX2 "Enable AVX2" ON)
set_target_properties(${PROJECT_NAME} PROPERTIES
	SUFFIX ".lib"
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
)
target_compile_options(${PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:/MTd>
	$<$<CONFIG:DebugEditor>:/MTd>
	$<$<CONFIG:Develop>:/MTd>
	$<$<CONFIG:DevelopEditor>:/MTd>
	$<$<CONFIG:Shipping>:/MT>
	$<$<CONFIG:Debug>:/ZI>
	$<$<CONFIG:DebugEditor>:/ZI>
	$<$<CONFIG:Develop>:/Zi>
	$<$<CONFIG:DevelopEditor>:/Zi>
	$<$<CONFIG:Shipping>:/Zi>
)
if(MSVC)
	target_compile_options(${PROJECT_NAME} PRIVATE
		/nologo
		/diagnostics:classic
		/D_MBCS
		/sdl
		/MP
		/Wall
		/Gm-
		/EHsc
		/fp:fast
		/fp:except-
		/Zc:wchar_t
		/Zc:forScope
		/Zc:inline
		/std:c++17
		/std:c11
		/GR-
		/Gd
		/FC
	)
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<BOOL:${ENABLED_WARNING_AS_ERROR}>:/WX>
		$<$<NOT:$<BOOL:${ENABLED_WARNING_AS_ERROR}>>:/WX->
		$<$<BOOL:${USE_AVX2}>:/arch:AVX2>
	)
	target_compile_options(${ENGINE_PROJECT_NAME} PUBLIC
		$<$<CONFIG:Debug>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:DebugEditor>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:Develop>:/O2 /Oi /Ob1 /GS /Zc:rvalueCast>
		$<$<CONFIG:DevelopEditor>:/O2 /Oi /Ob1 /GS /Zc:rvalueCast>
		$<$<CONFIG:Shipping>:/O2 /Oi /Ot /GL /Ob2 /Gy>
	)
endif()

#Jolt Platform Settings
set(BUILD_BINARY_DIR ${PROJECT_BINARY_DIR}/JoltBuild64)
foreach(CONFIG IN LISTS CMAKE_CONFIGURATION_TYPES)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
	set_target_properties(${ENGINE_PROJECT_NAME} PROPERTIES
		LIBRARY_OUTPUT_DIRECTORY ${BUILD_BINARY_DIR}/Lib/${CONFIG_UPPER}/
		PDB_OUTPUT_DIRECTORY ${BUILD_BINARY_DIR}/Lib/${CONFIG_UPPER}/
		COMPILE_PDB_OUTPUT_DIRECTORY ${BUILD_BINARY_DIR}/Lib/${CONFIG_UPPER}/
		ARCHIVE_OUTPUT_DIRECTORY ${BUILD_BINARY_DIR}/Lib/${CONFIG_UPPER}/
		RUNTIME_OUTPUT_DIRECTORY ${BUILD_BINARY_DIR}/Bin/${CONFIG_UPPER}/
	)
endforeach()
option(PLATFORM_WINDOWS "Build on WindowsPlatform" ON)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${PROJECT_NAME} PUBLIC
		$<$<BOOL:${PLATFORM_WINDOWS}>:_WINDOWS>
		__STDC_WANT_SECURE_LIB__
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.18362.0"
	)
endif()

#Jolt Definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:_DEBUG=1>
	$<$<CONFIG:DebugEditor>:_DEBUG=1>
	$<$<CONFIG:Develop>:_DEBUG=1>
	$<$<CONFIG:DevelopEditor>:_DEBUG=1>
	$<$<CONFIG:Shipping>:NDEBUG>
)
#Note that this currently only works using MSVC.
#Clang turns Float2 into a SIMD vector sometimes
#causing floating point exceptions (the option is ignored).
option(FLOATING_POINT_EXCEPTIONS_ENABLED "Enable floating point exceptions" ON)
option(USE_SSE4_1 "Enable SSE4.1" OFF)
option(USE_SSE4_2 "Enable SSE4.2" OFF)
option(USE_AVX "Enable AVX" OFF)
option(USE_AVX512 "Enable AVX512" OFF)
option(USE_LZCNT "Enable LZCNT" ON)
option(USE_TZCNT "Enable TZCNT" ON)
option(USE_F16C "Enable F16C" ON)
option(USE_FMADD "Enable FMADD" ON)
target_compile_definitions(${PROJECT_NAME} PRIVATE
	JPH_PROFILE_ENABLED
	JPH_DISABLE_CUSTOM_ALLOCATOR
)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		$<$<BOOL:${FLOATING_POINT_EXCEPTIONS_ENABLED}>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>
	)
endif()
target_compile_definitions(${PROJECT_NAME} PRIVATE
	$<$<BOOL:${USE_SSE4_1}>:JPH_USE_SSE4_1>
	$<$<BOOL:${USE_SSE4_2}>:JPH_USE_SSE4_2>
	$<$<BOOL:${USE_AVX}>:JPH_USE_AVX>
	$<$<BOOL:${USE_AVX2}>:JPH_USE_AVX2>
	$<$<BOOL:${USE_AVX512}>:JPH_USE_AVX512>
	$<$<BOOL:${USE_LZCNT}>:JPH_USE_LZCNT>
	$<$<BOOL:${USE_TZCNT}>:JPH_USE_TZCNT>
	$<$<BOOL:${USE_F16C}>:JPH_USE_F16C>
	$<$<BOOL:${USE_FMADD}>:JPH_USE_FMADD>
)

target_precompile_headers(${PROJECT_NAME} PRIVATE ./Jolt.dir/cmake_pch.hxx)

#Jolt Sources Include
target_include_directories(${PROJECT_NAME} PRIVATE ./)