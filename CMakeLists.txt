cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

set(CMAKE_CONFIGURATION_TYPES "Debug;DebugEditor;Develop;DevelopEditor;Shipping" CACHE STRING "Pigeon Engine Project Configurations" FORCE)

#Pigeon Engine Project
set(ENGINE_PROJECT_NAME PigeonEngine)

project(${ENGINE_PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

if(MSVC)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${ENGINE_PROJECT_NAME})
endif()

add_executable(${ENGINE_PROJECT_NAME})

#Pigeon Engine Directory
set(BASE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Engine)
set(BASE_CONFIG_SOURCE_DIR ${BASE_SOURCE_DIR}/Configs)
set(ENGINE_SOURCE_DIR ${BASE_SOURCE_DIR}/Sources)
set(THIRD_PARTY_SOURCE_DIR ${BASE_SOURCE_DIR}/ThirdParty)
set(ASSETS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Assets)

set(BASE_BINARY_DIR ${PROJECT_BINARY_DIR}/Engine)
set(BASE_CONFIG_BINARY_DIR ${BASE_BINARY_DIR}/Configs)
set(TEMP_BINARY_DIR ${PROJECT_BINARY_DIR}/Temp)
set(TEMP_RAWSHADER_BINARY_DIR ${TEMP_BINARY_DIR}/OutputRawShaders)
set(BUILD_BINARY_DIR ${PROJECT_BINARY_DIR}/Build64)

#Pigeon Engine Entry
#Editor Entry
set(ENGINE_EDITOR_ENTRY ${ENGINE_SOURCE_DIR}/EngineCore/Main/Main.cpp)
#TODO Shipping Entry

#Pigeon Engine Build Properties
option(ENABLED_WARNING_AS_ERROR "Enable warning as error" ON)
option(USE_AVX2 "Enable AVX2" ON)
set_target_properties(${ENGINE_PROJECT_NAME} PROPERTIES
	CXX_STANDARD 17
	CXX_EXTENSIONS ON
	CXX_STANDARD_REQUIRED ON
	C_STANDARD 11
	C_EXTENSIONS ON
	C_STANDARD_REQUIRED ON
)
target_compile_options(${ENGINE_PROJECT_NAME} PUBLIC
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
	#TODO We will not ignore MSBuild warning lists : /wd4201 /wd4100 /wd4238
	target_compile_options(${ENGINE_PROJECT_NAME} PUBLIC
		/nologo
		/diagnostics:classic
		/D_MBCS
		/sdl
		/MP
		/W4
		/Gm-
		/EHsc
		/fp:fast
		/fp:except-
		/Zc:wchar_t
		/Zc:forScope
		/Zc:inline
		/std:c++17
		/std:c11
		/Gd
		/FC
		/wd4201
		/wd4100
		/wd4238
	)
	target_compile_options(${ENGINE_PROJECT_NAME} PUBLIC
		$<$<BOOL:${ENABLED_WARNING_AS_ERROR}>:/WX>
		$<$<NOT:$<BOOL:${ENABLED_WARNING_AS_ERROR}>>:/WX->
		$<$<BOOL:${USE_AVX2}>:/arch:AVX2>
	)
	target_compile_options(${ENGINE_PROJECT_NAME} PUBLIC
		$<$<CONFIG:Debug>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:DebugEditor>:/Od /Ob0 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:Develop>:/O2 /Oi /Ob1 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:DevelopEditor>:/O2 /Oi /Ob1 /RTC1 /GS /Zc:rvalueCast>
		$<$<CONFIG:Shipping>:/O2 /Oi /Ot /GL /Ob2 /Gy>
	)
	target_link_options(${ENGINE_PROJECT_NAME} PUBLIC
		$<$<CONFIG:Debug>:/INCREMENTAL>
		$<$<CONFIG:DebugEditor>:/INCREMENTAL>
		$<$<CONFIG:Develop>:/INCREMENTAL>
		$<$<CONFIG:DevelopEditor>:/INCREMENTAL>
		$<$<CONFIG:Shipping>:/INCREMENTAL:NO>
		$<$<CONFIG:Debug>:/DEBUG>
		$<$<CONFIG:DebugEditor>:/DEBUG>
		$<$<CONFIG:Develop>:/DEBUG>
		$<$<CONFIG:DevelopEditor>:/DEBUG>
	)
	target_link_options(${ENGINE_PROJECT_NAME} PRIVATE
		/SUBSYSTEM:WINDOWS
	)
endif()
target_link_options(${ENGINE_PROJECT_NAME} PUBLIC
	$<$<CONFIG:Develop>:/LTCG>
	$<$<CONFIG:DevelopEditor>:/LTCG>
	$<$<CONFIG:Shipping>:/LTCG>
)

#Pigeon Engine Build Configs
configure_file(
	${BASE_CONFIG_SOURCE_DIR}/BaseConfig.h.in
	${BASE_CONFIG_BINARY_DIR}/BaseConfig.h
)

#Pigeon Engine Platform Settings
option(PLATFORM_WINDOWS "Build on WindowsPlatform" ON)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PUBLIC
		$<$<BOOL:${PLATFORM_WINDOWS}>:_WINDOWS>
	)
	set_target_properties(${ENGINE_PROJECT_NAME} PROPERTIES
		VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION "10.0.18362.0"
	)
endif()

#Pigeon Engine Definitions
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:_DEBUG=1>
	$<$<CONFIG:DebugEditor>:_DEBUG=1>
	$<$<CONFIG:Develop>:_DEBUG=1>
	$<$<CONFIG:DevelopEditor>:_DEBUG=1>
	$<$<CONFIG:Shipping>:NDEBUG>
)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:_DEBUG_MODE=1>
	$<$<CONFIG:Debug>:_DEVELOP_MODE=0>
	$<$<CONFIG:Debug>:_SHIPPING_MODE=0>
	$<$<CONFIG:Debug>:_EDITOR_ONLY=0>
)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:DebugEditor>:_DEBUG_MODE=1>
	$<$<CONFIG:DebugEditor>:_DEVELOP_MODE=0>
	$<$<CONFIG:DebugEditor>:_SHIPPING_MODE=0>
	$<$<CONFIG:DebugEditor>:_EDITOR_ONLY=1>
)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Develop>:_DEBUG_MODE=0>
	$<$<CONFIG:Develop>:_DEVELOP_MODE=1>
	$<$<CONFIG:Develop>:_SHIPPING_MODE=0>
	$<$<CONFIG:Develop>:_EDITOR_ONLY=0>
)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:DevelopEditor>:_DEBUG_MODE=0>
	$<$<CONFIG:DevelopEditor>:_DEVELOP_MODE=1>
	$<$<CONFIG:DevelopEditor>:_SHIPPING_MODE=0>
	$<$<CONFIG:DevelopEditor>:_EDITOR_ONLY=1>
)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Shipping>:_DEBUG_MODE=0>
	$<$<CONFIG:Shipping>:_DEVELOP_MODE=0>
	$<$<CONFIG:Shipping>:_SHIPPING_MODE=1>
	$<$<CONFIG:Shipping>:_EDITOR_ONLY=0>
)

#Pigeon Engine Third Party Include & Link
#Pigeon Engine Third Party Imgui
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${THIRD_PARTY_SOURCE_DIR}/imgui/
)

#Pigeon Engine Third Party Json
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${THIRD_PARTY_SOURCE_DIR}/rapidjson/
)

#Pigeon Engine Third Party Assimp
set(ASSIMP_LIB_DIR ${THIRD_PARTY_SOURCE_DIR}/assimp/lib)
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${THIRD_PARTY_SOURCE_DIR}/assimp/
)
target_link_libraries(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:${ASSIMP_LIB_DIR}/Debug/zlibstaticd.lib>
	$<$<CONFIG:Debug>:${ASSIMP_LIB_DIR}/Debug/assimp-vc143-mtd.lib>
	$<$<CONFIG:DebugEditor>:${ASSIMP_LIB_DIR}/Debug/zlibstaticd.lib>
	$<$<CONFIG:DebugEditor>:${ASSIMP_LIB_DIR}/Debug/assimp-vc143-mtd.lib>
	$<$<CONFIG:Develop>:${ASSIMP_LIB_DIR}/Release/zlibstaticd.lib>
	$<$<CONFIG:Develop>:${ASSIMP_LIB_DIR}/Release/assimp-vc143-mtd.lib>
	$<$<CONFIG:DevelopEditor>:${ASSIMP_LIB_DIR}/Release/zlibstaticd.lib>
	$<$<CONFIG:DevelopEditor>:${ASSIMP_LIB_DIR}/Release/assimp-vc143-mtd.lib>
	$<$<CONFIG:Shipping>:${ASSIMP_LIB_DIR}/Release/zlibstaticd.lib>
	$<$<CONFIG:Shipping>:${ASSIMP_LIB_DIR}/Release/assimp-vc143-mtd.lib>
)

#Pigeon Engine Third Party Jolt Physics
set(JOLT_PROJECT_NAME Jolt)
add_subdirectory(${THIRD_PARTY_SOURCE_DIR}/JoltPhysics/)
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
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	JPH_PROFILE_ENABLED
	JPH_DISABLE_CUSTOM_ALLOCATOR
)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
		$<$<BOOL:${FLOATING_POINT_EXCEPTIONS_ENABLED}>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>
	)
endif()
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
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
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${THIRD_PARTY_SOURCE_DIR}/JoltPhysics/
)
target_link_libraries(${ENGINE_PROJECT_NAME} PRIVATE ${JOLT_PROJECT_NAME})

#Output Settings
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

#Pigeon Engine Sources Include
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${BASE_CONFIG_BINARY_DIR}/
	${BASE_SOURCE_DIR}/Sources/
	${BASE_SOURCE_DIR}/Sources/EngineCore/
	${BASE_SOURCE_DIR}/Sources/EngineCore/Core/
	${BASE_SOURCE_DIR}/Sources/EngineCore/HID/
	${BASE_SOURCE_DIR}/Sources/EngineCore/IO/
	${BASE_SOURCE_DIR}/Sources/EngineCore/Main/
	${BASE_SOURCE_DIR}/Sources/EngineCore/PhysicsCore/
	${BASE_SOURCE_DIR}/Sources/EngineCore/RenderCore/
	${BASE_SOURCE_DIR}/Sources/EngineCore/Shaders/
	${BASE_SOURCE_DIR}/Sources/Asset/
	${BASE_SOURCE_DIR}/EngineThirdParty/
)

#Pigeon Engine Target Sources
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_H ${BASE_SOURCE_DIR}/Sources/*.h)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_HPP ${BASE_SOURCE_DIR}/Sources/*.hpp)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_C ${BASE_SOURCE_DIR}/Sources/*.c)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_CPP ${BASE_SOURCE_DIR}/Sources/*.cpp)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_COMMON_HLSL ${BASE_SOURCE_DIR}/Sources/EngineCore/Shaders/Common/*.hlsl)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_PRIVATE_HLSL ${BASE_SOURCE_DIR}/Sources/EngineCore/Shaders/Private/*.hlsl)
file(GLOB_RECURSE ENGINE_PROJECT_HEADERS_PUBLIC_HLSL ${BASE_SOURCE_DIR}/Sources/EngineCore/Shaders/Public/*.hlsl)

file(GLOB_RECURSE ENGINE_THIRDPARTY_HEADERS_H ${BASE_SOURCE_DIR}/EngineThirdParty/*.h)
file(GLOB_RECURSE ENGINE_THIRDPARTY_HEADERS_HPP ${BASE_SOURCE_DIR}/EngineThirdParty/*.hpp)
file(GLOB_RECURSE ENGINE_THIRDPARTY_HEADERS_C ${BASE_SOURCE_DIR}/EngineThirdParty/*.c)
file(GLOB_RECURSE ENGINE_THIRDPARTY_HEADERS_CPP ${BASE_SOURCE_DIR}/EngineThirdParty/*.cpp)
file(GLOB_RECURSE ENGINE_THIRDPARTY_HEADERS_HLSL ${BASE_SOURCE_DIR}/EngineThirdParty/*.hlsl)

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_H})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_HPP})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_C})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_CPP})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_COMMON_HLSL})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_PRIVATE_HLSL})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_PROJECT_HEADERS_PUBLIC_HLSL})

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_THIRDPARTY_HEADERS_H})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_THIRDPARTY_HEADERS_HPP})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_THIRDPARTY_HEADERS_C})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_THIRDPARTY_HEADERS_CPP})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/Engine/ FILES ${ENGINE_THIRDPARTY_HEADERS_HLSL})

if(MSVC)
	option(ENABLED_HLSL_DEBUG "Enable hlsl debug info" OFF)
	message(STATUS "Start set shader files properties")
	foreach(HLSL_FILE ${ENGINE_PROJECT_HEADERS_COMMON_HLSL})
		set_source_files_properties(${HLSL_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
	endforeach()
	foreach(HLSL_FILE ${ENGINE_PROJECT_HEADERS_PUBLIC_HLSL})
		set_source_files_properties(${HLSL_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
	endforeach()
	foreach(HLSL_FILE ${ENGINE_PROJECT_HEADERS_PRIVATE_HLSL})
		get_filename_component(FILE_NAME ${HLSL_FILE} NAME)
		if(FILE_NAME MATCHES "_PEVS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a vertex shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Vertex")
		endif()
		if(FILE_NAME MATCHES "_PEPS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a pixel shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Pixel")
		endif()
		if(FILE_NAME MATCHES "_PECS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a compute shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Compute")
		endif()
		set_source_files_properties(${HLSL_FILE} PROPERTIES
			VS_SHADER_ENTRYPOINT "main"
			VS_SHADER_MODEL "5.0"
		)
		if(ENABLED_HLSL_DEBUG)
			set_source_files_properties(${HLSL_FILE} PROPERTIES
				VS_SHADER_DISABLE_OPTIMIZATIONS ON
				VS_SHADER_ENABLE_DEBUG ON
			)
		endif()
		get_filename_component(OUTPUT_FILE_NAME ${HLSL_FILE} NAME_WLE)
		set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_OBJECT_FILE_NAME "${TEMP_RAWSHADER_BINARY_DIR}/${OUTPUT_FILE_NAME}.cso")
	endforeach()
	foreach(HLSL_FILE ${ENGINE_THIRDPARTY_HEADERS_HLSL})
		get_filename_component(FILE_NAME ${HLSL_FILE} NAME)
		if(FILE_NAME MATCHES "_PEVS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a vertex shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Vertex")
		endif()
		if(FILE_NAME MATCHES "_PEPS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a pixel shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Pixel")
		endif()
		if(FILE_NAME MATCHES "_PECS\\.hlsl$")
			message(STATUS "${FILE_NAME} is a compute shader.")
			set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_TYPE "Compute")
		endif()
		set_source_files_properties(${HLSL_FILE} PROPERTIES
			VS_SHADER_ENTRYPOINT "main"
			VS_SHADER_MODEL "5.0"
		)
		if(ENABLED_HLSL_DEBUG)
			set_source_files_properties(${HLSL_FILE} PROPERTIES
				VS_SHADER_DISABLE_OPTIMIZATIONS ON
				VS_SHADER_ENABLE_DEBUG ON
			)
		endif()
		get_filename_component(OUTPUT_FILE_NAME ${HLSL_FILE} NAME_WLE)
		set_source_files_properties(${HLSL_FILE} PROPERTIES VS_SHADER_OBJECT_FILE_NAME "${TEMP_RAWSHADER_BINARY_DIR}/${OUTPUT_FILE_NAME}.cso")
	endforeach()
endif()

target_sources(${ENGINE_PROJECT_NAME} PRIVATE
	${ENGINE_PROJECT_HEADERS_H}
	${ENGINE_PROJECT_HEADERS_HPP}
	${ENGINE_PROJECT_HEADERS_C}
	${ENGINE_PROJECT_HEADERS_CPP}
	${ENGINE_PROJECT_HEADERS_COMMON_HLSL}
	${ENGINE_PROJECT_HEADERS_PRIVATE_HLSL}
	${ENGINE_PROJECT_HEADERS_PUBLIC_HLSL}
	${ENGINE_THIRDPARTY_HEADERS_H}
	${ENGINE_THIRDPARTY_HEADERS_HPP}
	${ENGINE_THIRDPARTY_HEADERS_C}
	${ENGINE_THIRDPARTY_HEADERS_CPP}
	${ENGINE_THIRDPARTY_HEADERS_HLSL}
)