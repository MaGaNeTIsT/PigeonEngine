cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

#Jolt Project
set(PROJECT_NAME Jolt)

project(${PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE JOLT_SOURCES "${PROJECT_SOURCE_DIR}/Jolt/*.cpp")

add_library(${PROJECT_NAME} STATIC ${JOLT_SOURCES})

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
	$<$<CONFIG:Develop>:/MT>
	$<$<CONFIG:DevelopEditor>:/MT>
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
	target_compile_options(${PROJECT_NAME} PRIVATE
		$<$<CONFIG:Debug>:/Od>
		$<$<CONFIG:DebugEditor>:/Od>
		$<$<CONFIG:Develop>:/O2>
		$<$<CONFIG:DevelopEditor>:/O2>
		$<$<CONFIG:Shipping>:/O2>
		$<$<CONFIG:Develop>:/Oi>
		$<$<CONFIG:DevelopEditor>:/Oi>
		$<$<CONFIG:Shipping>:/Oi>
		$<$<CONFIG:Shipping>:/Ot>
		$<$<CONFIG:Shipping>:/GL>
		$<$<CONFIG:Debug>:/Ob0>
		$<$<CONFIG:DebugEditor>:/Ob0>
		$<$<CONFIG:Develop>:/Ob1>
		$<$<CONFIG:DevelopEditor>:/Ob1>
		$<$<CONFIG:Shipping>:/Ob2>
		$<$<CONFIG:Debug>:/RTCc>
		$<$<CONFIG:DebugEditor>:/RTCc>
		$<$<CONFIG:Debug>:/RTC1>
		$<$<CONFIG:DebugEditor>:/RTC1>
		$<$<CONFIG:Develop>:/RTC1>
		$<$<CONFIG:DevelopEditor>:/RTC1>
		$<$<CONFIG:Debug>:/GS>
		$<$<CONFIG:DebugEditor>:/GS>
		$<$<CONFIG:Develop>:/GS>
		$<$<CONFIG:DevelopEditor>:/GS>
		$<$<CONFIG:Shipping>:/GS->
		$<$<CONFIG:Shipping>:/Gy>
		$<$<CONFIG:Debug>:/fp:except>
		$<$<CONFIG:DebugEditor>:/fp:except>
		$<$<CONFIG:Develop>:/fp:except->
		$<$<CONFIG:DevelopEditor>:/fp:except->
		$<$<CONFIG:Shipping>:/fp:except->
		$<$<CONFIG:Debug>:/Zc:rvalueCast>
		$<$<CONFIG:DebugEditor>:/Zc:rvalueCast>
		$<$<CONFIG:Develop>:/Zc:rvalueCast>
		$<$<CONFIG:DevelopEditor>:/Zc:rvalueCast>
		$<$<CONFIG:Shipping>:/Zc:rvalueCast->
	)
endif()

#Jolt Platform Settings
set_target_properties(${PROJECT_NAME} PROPERTIES
	$<$<CONFIG:Debug>:LIBRARY_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Debug/">
	$<$<CONFIG:Debug>:PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Debug/">
	$<$<CONFIG:Debug>:COMPILE_PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Debug/">
	$<$<CONFIG:Debug>:ARCHIVE_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Debug/">
	$<$<CONFIG:Debug>:RUNTIME_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Bin/Debug/">
	$<$<CONFIG:DebugEditor>:LIBRARY_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DebugEditor/">
	$<$<CONFIG:DebugEditor>:PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DebugEditor/">
	$<$<CONFIG:DebugEditor>:COMPILE_PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DebugEditor/">
	$<$<CONFIG:DebugEditor>:ARCHIVE_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DebugEditor/">
	$<$<CONFIG:DebugEditor>:RUNTIME_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Bin/DebugEditor/">
	$<$<CONFIG:Develop>:LIBRARY_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Develop/">
	$<$<CONFIG:Develop>:PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Develop/">
	$<$<CONFIG:Develop>:COMPILE_PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Develop/">
	$<$<CONFIG:Develop>:ARCHIVE_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Develop/">
	$<$<CONFIG:Develop>:RUNTIME_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Bin/Develop/">
	$<$<CONFIG:DevelopEditor>:LIBRARY_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DevelopEditor/">
	$<$<CONFIG:DevelopEditor>:PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DevelopEditor/">
	$<$<CONFIG:DevelopEditor>:COMPILE_PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DevelopEditor/">
	$<$<CONFIG:DevelopEditor>:ARCHIVE_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/DevelopEditor/">
	$<$<CONFIG:DevelopEditor>:RUNTIME_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Bin/DevelopEditor/">
	$<$<CONFIG:Shipping>:LIBRARY_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Shipping/">
	$<$<CONFIG:Shipping>:PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Shipping/">
	$<$<CONFIG:Shipping>:COMPILE_PDB_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Shipping/">
	$<$<CONFIG:Shipping>:ARCHIVE_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Lib/Shipping/">
	$<$<CONFIG:Shipping>:RUNTIME_OUTPUT_DIRECTORY="${PROJECT_BINARY_DIR}/Bin/Shipping/">
)
option(PLATFORM_WINDOWS "Build on WindowsPlatform" ON)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${PROJECT_NAME} PUBLIC
		$<$<BOOL:${PLATFORM_WINDOWS}>:_WINDOWS>
	)
	set_target_properties(${PROJECT_NAME} PROPERTIES
		VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.18362.0
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
#option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" ON)
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

target_precompile_headers(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/Jolt.dir/cmake_pch.hxx)

#Jolt Sources Include
target_include_directories(${PROJECT_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/Jolt/)