cmake_minimum_required(VERSION 3.16 FATAL_ERROR)

#Pigeon Engine Project
set(ENGINE_PROJECT_NAME PigeonEngine)

project(${ENGINE_PROJECT_NAME} VERSION 1.0 LANGUAGES CXX)

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

#Pigeon Engine Entry
#Editor Entry
set(ENGINE_EDITOR_ENTRY ${ENGINE_SOURCE_DIR}/EngineCore/Main/Main.cpp)
#TODO Shipping Entry

#Pigeon Engine Build Properties
set_target_properties(${ENGINE_PROJECT_NAME} PROPERTIES
	C_STANDARD 11
	C_STANDARD_REQUIRED ON
	CXX_STANDARD 17
	CXX_STANDARD_REQUIRED ON
)

#Pigeon Engine Build Configs
configure_file(
	${BASE_CONFIG_SOURCE_DIR}/BaseConfig.h.in
	${BASE_CONFIG_BINARY_DIR}/BaseConfig.h
)

#Pigeon Engine Platform Settings
option(PLATFORM_WINDOWS "Build on WindowsPlatform" ON)
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<BOOL:${PLATFORM_WINDOWS}>:_WINDOWS>
)

#Pigeon Engine Definitions
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:_DEBUG=1>
	$<$<CONFIG:DebugEditor>:_DEBUG=1>
	$<$<CONFIG:Develop>:_DEBUG=1>
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
	${THIRD_PARTY_SOURCE_DIR}/assimp/assimp/
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
set(JOLT_LIB_DIR ${THIRD_PARTY_SOURCE_DIR}/JoltPhysics/lib)
configure_file(
	${BASE_CONFIG_SOURCE_DIR}/BasePhysicsConfig.h.in
	${BASE_CONFIG_BINARY_DIR}/BasePhysicsConfig.h
)
#Note that this currently only works using MSVC.
#Clang turns Float2 into a SIMD vector sometimes
#causing floating point exceptions (the option is ignored).
option(FLOATING_POINT_EXCEPTIONS_ENABLED "Enable floating point exceptions" ON)
option(USE_SSE4_1 "Enable SSE4.1" ON)
option(USE_SSE4_2 "Enable SSE4.2" ON)
option(USE_AVX "Enable AVX" ON)
option(USE_AVX2 "Enable AVX2" ON)
option(USE_AVX512 "Enable AVX512" OFF)
option(USE_LZCNT "Enable LZCNT" ON)
option(USE_TZCNT "Enable TZCNT" ON)
option(USE_F16C "Enable F16C" ON)
option(USE_FMADD "Enable FMADD" ON)
#option(ENABLE_ALL_WARNINGS "Enable all warnings and warnings as errors" ON)
if(PLATFORM_WINDOWS)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
		$<$<BOOL:${FLOATING_POINT_EXCEPTIONS_ENABLED}>:JPH_FLOATING_POINT_EXCEPTIONS_ENABLED>
	)
endif()
target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE
	JPH_PROFILE_ENABLED
	JPH_DISABLE_CUSTOM_ALLOCATOR
)
if(USE_SSE4_1)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_SSE4_1)
endif()
if(USE_SSE4_2)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_SSE4_2)
endif()
if(USE_AVX)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_AVX)
endif()
if(USE_AVX2)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_AVX2)
endif()
if(USE_AVX512)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_AVX512)
endif()
if(USE_LZCNT)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_LZCNT)
endif()
if(USE_TZCNT)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_TZCNT)
endif()
if(USE_F16C)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_F16C)
endif()
if(USE_FMADD)
	target_compile_definitions(${ENGINE_PROJECT_NAME} PRIVATE JPH_USE_FMADD)
endif()
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${THIRD_PARTY_SOURCE_DIR}/JoltPhysics/Jolt/
)
target_link_libraries(${ENGINE_PROJECT_NAME} PRIVATE
	$<$<CONFIG:Debug>:${JOLT_LIB_DIR}/Debug/Jolt.lib>
	$<$<CONFIG:DebugEditor>:${JOLT_LIB_DIR}/Debug/Jolt.lib>
	$<$<CONFIG:Develop>:${JOLT_LIB_DIR}/Release/Jolt.lib>
	$<$<CONFIG:DevelopEditor>:${JOLT_LIB_DIR}/Release/Jolt.lib>
	$<$<CONFIG:Shipping>:${JOLT_LIB_DIR}/Release/Jolt.lib>
)

#Pigeon Engine Sources Include
target_include_directories(${ENGINE_PROJECT_NAME} PRIVATE
	${BASE_CONFIG_BINARY_DIR}/
	${BASE_SOURCE_DIR}/Sources/
	${BASE_SOURCE_DIR}/EngineThirdParty/
)

#Pigeon Engine Target Sources
#Editor Entry
target_sources(${ENGINE_PROJECT_NAME} PRIVATE $<$<CONFIG:DebugEditor>:${ENGINE_EDITOR_ENTRY}>)
target_sources(${ENGINE_PROJECT_NAME} PRIVATE $<$<CONFIG:DevelopEditor>:${ENGINE_EDITOR_ENTRY}>)
#TODO Shipping Entry