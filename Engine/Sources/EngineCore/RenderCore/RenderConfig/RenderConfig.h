#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class RCommonSettings final
	{
	public:
		PE_CONSTEXPR_STATIC		FLOAT			RENDER_DEPTH_NEAR					= 0.f;
		PE_CONSTEXPR_STATIC		FLOAT			RENDER_DEPTH_FAR					= 1.f;

		PE_CONSTEXPR_STATIC		FLOAT			RENDER_CAMERA_NEAR					= 5.f;
		PE_CONSTEXPR_STATIC		FLOAT			RENDER_CAMERA_FAR					= 200000.f;

		PE_CONSTEXPR_STATIC		USHORT			RENDER_MESH_BONE_NUM_MAX			= 256u;
		PE_CONSTEXPR_STATIC		USHORT			RENDER_SKELETON_BONE_NUM_MAX		= 512u;

		PE_CONSTEXPR_STATIC		UINT32			RENDER_TARGET_WIDTH					= 1600u;
		PE_CONSTEXPR_STATIC		UINT32			RENDER_TARGET_HEIGHT				= 900u;

		PE_CONSTEXPR_STATIC		UINT8			RENDER_STENCIL_READ_MASK			= 0xffu;
		PE_CONSTEXPR_STATIC		UINT8			RENDER_STENCIL_WRITE_MASK			= 0xffu;

		PE_CONSTEXPR_STATIC		UINT32			RENDER_SHADOW_MAP_DEPTH_BIT			= 16u;

		// Light list capacities.
		//
		// The two structured buffers are sized once at these maxima and never resized: a
		// structured buffer's shader resource view is built when the resource is created, so
		// growing it means recreating both. The published count is what bounds the shading
		// loop, so a frame with fewer lights uploads a shorter prefix rather than a smaller
		// buffer, and the cost of an idle slot is its bytes in a buffer nobody reads past the
		// count.
		//
		// Local is the sum of point and spot rather than a separate number, because the two
		// share one list: a single index range is what a tiled or clustered cull wants to
		// produce later, and it is one bind and one count instead of two of each.
		PE_CONSTEXPR_STATIC		UINT32			RENDER_DIRECTIONAL_LIGHT_NUM_MAX	= 4u;
		PE_CONSTEXPR_STATIC		UINT32			RENDER_POINT_LIGHT_NUM_MAX			= 16u;
		PE_CONSTEXPR_STATIC		UINT32			RENDER_SPOT_LIGHT_NUM_MAX			= 8u;
		PE_CONSTEXPR_STATIC		UINT32			RENDER_LOCAL_LIGHT_NUM_MAX			= RENDER_POINT_LIGHT_NUM_MAX + RENDER_SPOT_LIGHT_NUM_MAX;

		// The environment's diffuse representation: order 3 spherical harmonics, nine
		// coefficients, one sixteen byte slot each - the same slot per member the light lists use,
		// so the same reflection check covers this buffer too.
		//
		// This count must equal SH_COEFFICIENT_COUNT in Public/Lighting/ShaderSphericalHarmonics.hlsl,
		// and that pairing is the one thing the reflection check cannot verify: the buffer's element
		// is a bare float4, so the shader's element size says nothing about how many of them it
		// writes. The shader bounds its writes with its own constant, so a mismatch here is an
		// out of bounds write rather than a wrong colour - the two constants are a pair and have to
		// be changed together.
		PE_CONSTEXPR_STATIC		UINT32			RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM	= 9u;
	public:
		static RCommonSettings* GetSingleton()
		{
			static RCommonSettings _StaticSingletonObject;
			return (&_StaticSingletonObject);
		}
		RCommonSettings(const RCommonSettings&) = delete;
		RCommonSettings& operator=(const RCommonSettings&) = delete;
	private:
		RCommonSettings() {}
		~RCommonSettings() {}
	};

#if _EDITOR_ONLY
#define RENDER_GPU_PROFILER_NONE							("Render_GPUProfiler_None")
#define RENDER_GPU_PROFILER_RECORD_FRAME_COUNT				(5)
#endif

#define RENDER_GPU_QUERY_NONE								("Render_GPUQuery_None")

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
