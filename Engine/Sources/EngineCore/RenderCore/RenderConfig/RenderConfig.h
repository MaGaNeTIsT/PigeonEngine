#pragma once

namespace PigeonEngine
{

#define RENDER_CULLING_DISTANCE								(200000.f)
#define RENDER_CULLING_OFFSET								(-5.f)

#define RENDER_DEPTH_MIN									(0.f)
#define RENDER_DEPTH_MAX									(1.f)

#define RENDER_CAMERA_FOV									(60.f)
#define RENDER_CAMERA_NEAR									(5.f)
#define RENDER_CAMERA_FAR									(200000.f)

#define RENDER_SHADOW_MAP_SIZE								(4096)

#ifdef _EDITOR_ONLY
#define RENDER_GPU_PROFILER_NONE							("Render_GPUProfiler_None")
#define RENDER_GPU_PROFILER_RECORD_FRAME_COUNT				(5)
#endif

#define RENDER_GPU_QUERY_NONE								("Render_GPUQuery_None")

#define	ENGINE_DEFAULT_STENCIL_READ_MASK					(0xff)
#define	ENGINE_DEFAULT_STENCIL_WRITE_MASK					(0xff)

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

};