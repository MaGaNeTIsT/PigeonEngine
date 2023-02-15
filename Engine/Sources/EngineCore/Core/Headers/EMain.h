#pragma once

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include <wrl/client.h>
#include <stdio.h>
#include <ctime>
#include <assert.h>
#include <io.h>
#include <tchar.h>
#include <dwmapi.h>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <functional>

#include <d3d11.h>
#include <DirectXMath.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")

#ifdef _DEVELOPMENT_EDITOR
#include "../../../../ThirdParty/imgui/imgui.h"
#endif

#include "../../Base/Headers/EMath.h"

#define ENGINE_FLOAT32_MAX									(3.402823466e+38f)
#define	ENGINE_DEFAULT_STENCIL_READ_MASK					(0xff)
#define	ENGINE_DEFAULT_STENCIL_WRITE_MASK					(0xff)

#define ENGINE_CUSTOM_ARRAYSIZE(_ARR)						(static_cast<UINT>(sizeof(_ARR) / sizeof(*(_ARR))))
#define ENGINE_NOT_EXIST_STRING								("Engine_Not_Exist")

#define ENGINE_SCREEN_WIDTH									(1600)
#define ENGINE_SCREEN_HEIGHT								(900)

#define ENGINE_FIXED_UPDATE_FRAME							(60)
#define ENGINE_UPDATE_FRAME									(90)
#define ENGINE_WINDOWED										(1)

#define ENGINE_CBUFFER_VS_PS_CS_PER_FRAME_START_SLOT		(0u)
#define ENGINE_CBUFFER_VS_PS_CS_LIGHT_DATA_START_SLOT		(1u)
#define ENGINE_CBUFFER_VS_PS_PER_DRAW_START_SLOT			(2u)
#define ENGINE_CBUFFER_VS_PS_MATERIAL_DATA_START_SLOT		(3u)
#define ENGINE_CBUFFER_VS_SKELETON_DATA_START_SLOT			(4u)

#define ENGINE_SAMPLER_FIRST_START_SLOT						(0u)
#define ENGINE_SAMPLER_POINT_CLAMP_START_SLOT				(ENGINE_SAMPLER_FIRST_START_SLOT + 0u)
#define ENGINE_SAMPLER_POINT_WRAP_START_SLOT				(ENGINE_SAMPLER_FIRST_START_SLOT + 1u)
#define ENGINE_SAMPLER_LINEAR_CLAMP_START_SLOT				(ENGINE_SAMPLER_FIRST_START_SLOT + 2u)
#define ENGINE_SAMPLER_LINEAR_WRAP_START_SLOT				(ENGINE_SAMPLER_FIRST_START_SLOT + 3u)

#define ENGINE_AO_GLOBAL_INPUT_START_SLOT					(0u)

#define ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT			(1u)
#define ENGINE_LIGHT_SHADOW_MAP_0_START_SLOT				(ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + 0u)
#define ENGINE_LIGHT_SHADOW_MAP_1_START_SLOT				(ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + 1u)
#define ENGINE_LIGHT_SHADOW_MAP_2_START_SLOT				(ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + 2u)
#define ENGINE_LIGHT_SHADOW_MAP_3_START_SLOT				(ENGINE_LIGHT_SHADOW_MAP_FIRST_START_SLOT + 3u)

#define ENGINE_GBUFFER_FIRST_START_SLOT						(5u)
#define ENGINE_GBUFFER_A_START_SLOT							(ENGINE_GBUFFER_FIRST_START_SLOT + 0u)
#define ENGINE_GBUFFER_B_START_SLOT							(ENGINE_GBUFFER_FIRST_START_SLOT + 1u)
#define ENGINE_GBUFFER_C_START_SLOT							(ENGINE_GBUFFER_FIRST_START_SLOT + 2u)
#define ENGINE_GBUFFER_D_START_SLOT							(ENGINE_GBUFFER_FIRST_START_SLOT + 3u)

#define ENGINE_SCENE_NORMAL									(5u)
#define ENGINE_SCENE_COLOR									(6u)
#define ENGINE_SCENE_DEPTH									(7u)

#define ENGINE_TEXTURE2D_CUSTOM_FIRST_START_SLOT			(5u)
#define ENGINE_TEXTURE2D_CUSTOM_A_START_SLOT				(ENGINE_TEXTURE2D_CUSTOM_FIRST_START_SLOT + 0u)
#define ENGINE_TEXTURE2D_CUSTOM_B_START_SLOT				(ENGINE_TEXTURE2D_CUSTOM_FIRST_START_SLOT + 1u)
#define ENGINE_TEXTURE2D_CUSTOM_C_START_SLOT				(ENGINE_TEXTURE2D_CUSTOM_FIRST_START_SLOT + 2u)
#define ENGINE_TEXTURE2D_CUSTOM_D_START_SLOT				(ENGINE_TEXTURE2D_CUSTOM_FIRST_START_SLOT + 3u)

#define ENGINE_DEFAULT_TEXTURE2D_WHITE						("./Engine/Assets/EngineTextures/Default/EngineDefaultWhite.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BLACK						("./Engine/Assets/EngineTextures/Default/EngineDefaultBlack.tga")
#define ENGINE_DEFAULT_TEXTURE2D_GRAY						("./Engine/Assets/EngineTextures/Default/EngineDefaultGray.tga")
#define ENGINE_DEFAULT_TEXTURE2D_RED						("./Engine/Assets/EngineTextures/Default/EngineDefaultRed.tga")
#define ENGINE_DEFAULT_TEXTURE2D_GREEN						("./Engine/Assets/EngineTextures/Default/EngineDefaultGreen.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BLUE						("./Engine/Assets/EngineTextures/Default/EngineDefaultBlue.tga")
#define ENGINE_DEFAULT_TEXTURE2D_BUMP						("./Engine/Assets/EngineTextures/Default/EngineDefaultBump.tga")
#define ENGINE_DEFAULT_TEXTURE2D_PROPERTY					("./Engine/Assets/EngineTextures/Default/EngineDefaultProperty.tga")

#define ENGINE_SHADER_NONE									("EngineBase/Shader/None")
#define ENGINE_DEFAULT_SHADER_VS							("./Engine/Assets/EngineShaders/EngineDefaultVS.cso")

#define ENGINE_SHADER_EMPTY_PS								("./Engine/Assets/EngineShaders/EngineEmptyPS.cso")
#define ENGINE_SHADER_GBUFFER_WRITE_PS						("./Engine/Assets/EngineShaders/EngineGBufferWritePS.cso")
#define ENGINE_SHADER_DIRECT_LIGHT_PS						("./Engine/Assets/EngineShaders/EngineDirectLightPS.cso")

#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_VS				("./Engine/Assets/EngineShaders/EngineDefaultLitForwardVS.cso")
#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_SKELETON_MESH_VS	("./Engine/Assets/EngineShaders/EngineDefaultLitForwardSkeletonMeshVS.cso")
#define ENGINE_DEFAULT_LIT_SHADER_FORWARD_PS				("./Engine/Assets/EngineShaders/EngineDefaultLitForwardPS.cso")
#define ENGINE_DEFAULT_ANISOTROPIC_SHADER_FORWARD_VS		("./Engine/Assets/EngineShaders/EngineDefaultAnisotropicForwardVS.cso")
#define ENGINE_DEFAULT_ANISOTROPIC_SHADER_FORWARD_PS		("./Engine/Assets/EngineShaders/EngineDefaultAnisotropicForwardPS.cso")
#define ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_VS			("./Engine/Assets/EngineShaders/EngineDefaultClearCoatForwardVS.cso")
#define ENGINE_DEFAULT_CLEARCOAT_SHADER_FORWARD_PS			("./Engine/Assets/EngineShaders/EngineDefaultClearCoatForwardPS.cso")
#define ENGINE_DEFAULT_SHADER_CLOTH_VS						("./Engine/Assets/EngineShaders/EngineDefaultClothVS.cso")
#define ENGINE_DEFAULT_SHADER_CLOTH_PS						("./Engine/Assets/EngineShaders/EngineDefaultClothPS.cso")
#define ENGINE_SHADER_CLOTH_ANISOTROPIC_VS					("./Engine/Assets/EngineShaders/EngineClothAnisotropicVS.cso")
#define ENGINE_SHADER_CLOTH_ANISOTROPIC_PS					("./Engine/Assets/EngineShaders/EngineClothAnisotropicPS.cso")
#define ENGINE_DEFAULT_SHADER_TERRAIN_VS					("./Engine/Assets/EngineShaders/EngineDefaultTerrainVS.cso")
#define ENGINE_DEFAULT_SHADER_TERRAIN_PS					("./Engine/Assets/EngineShaders/EngineDefaultTerrainPS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_VS					("./Engine/Assets/EngineShaders/EngineScreenPolygon2DVS.cso")
#define ENGINE_SHADER_SCREEN_POLYGON_2D_PS					("./Engine/Assets/EngineShaders/EngineScreenPolygon2DPS.cso")
#define ENGINE_SHADER_SKY_BOX_VS							("./Engine/Assets/EngineShaders/EngineSkyBoxVS.cso")
#define ENGINE_SHADER_SKY_BOX_PS							("./Engine/Assets/EngineShaders/EngineSkyBoxPS.cso")

#define ENGINE_MESH_MATERIAL_SPHERE_NAME					("./Engine/Assets/EngineModels/SceneModels/MaterialSphere/MaterialSphere.obj")
#define ENGINE_MESH_MONKEY_NAME								("./Engine/Assets/EngineModels/SceneModels/Monkey/Monkey.obj")
#define ENGINE_MESH_NORMAL_CUBE_NAME						("./Engine/Assets/EngineModels/BaseShapes/Cube.obj")
#define ENGINE_MESH_ROUNDED_CUBE_NAME						("./Engine/Assets/EngineModels/BaseShapes/RoundedCube.obj")
#define ENGINE_MESH_SMOOTH_SPHERE_NAME						("./Engine/Assets/EngineModels/BaseShapes/SphereSmooth.obj")
#define ENGINE_MESH_UV_SPHERE_NAME							("./Engine/Assets/EngineModels/BaseShapes/SphereUV.obj")
#define ENGINE_MESH_TORUS_NAME								("./Engine/Assets/EngineModels/BaseShapes/Torus.obj")
#define ENGINE_MESH_PRISM_NAME								("./Engine/Assets/EngineModels/BaseShapes/Prism.obj")
#define ENGINE_MESH_CUBE_NAME								("EngineBase/Mesh/CubeMesh")
#define ENGINE_MESH_PLANE_NAME								("EngineBase/Mesh/PlaneMesh")
#define ENGINE_MESH_POLYGON_NAME							("EngineBase/Mesh/PolygonMesh")
#define ENGINE_MESH_POLYGON_2D_NAME							("EngineBase/Mesh/Polygon2DMesh")

#ifdef _DEVELOPMENT_EDITOR
#define ENGINE_GPU_PROFILER_NONE							("EngineBase/GPUProfiler/None")
#define ENGINE_GPU_PROFILER_RECORD_FRAME_COUNT				(5)
#endif

#define ENGINE_GPU_QUERY_NONE								("EngineBase/GPUQuery/None")

#define ENGINE_DEFAULT_CULLING_DISTANCE						(200000.f)
#define ENGINE_DEFAULT_CULLING_OFFSET						(-5.f)

#define ENGINE_CAMERA_FOV									(60.f)
#define ENGINE_CAMERA_NEAR									(5.f)
#define ENGINE_CAMERA_FAR									(200000.f)
#define ENGINE_CAMERA_MOVE_SPEED							(200.f)
#define ENGINE_CAMERA_LOOK_SPEED							(270.f)

#define ENGINE_SHADOW_MAP_SIZE								(4096)