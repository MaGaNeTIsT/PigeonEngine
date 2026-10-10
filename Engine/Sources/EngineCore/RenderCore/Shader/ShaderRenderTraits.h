#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	/*
	* ShaderAsset - abstract pipeline data.
	*
	* This is the only pipeline description the shader module stores, and it is
	* intentionally coarse: LightingModel / SurfaceMode are what a renderer needs to
	* choose a pipeline, concrete rasterizer / blend / depth state is not the shader
	* module's business. The module stores the data, the render layer interprets it
	* and performs the final override and assembly.
	*
	* Traits live on EShaderInstance only. A shader asset does not carry them: the
	* editor / runtime object is the place that is parameterized, and a future
	* serialization layer will persist the instance data next to the asset reference.
	*/

	enum EShaderLightingModelType : UINT8
	{
		SHADER_LIGHTING_MODEL_NONE		= 0,	// not specified
		SHADER_LIGHTING_MODEL_LIT		= 1,
		SHADER_LIGHTING_MODEL_UNLIT		= 2,
		SHADER_LIGHTING_MODEL_COUNT,
	};

	enum EShaderSurfaceModeType : UINT8
	{
		SHADER_SURFACE_MODE_NONE		= 0,	// not specified
		SHADER_SURFACE_MODE_OPAQUE		= 1,
		SHADER_SURFACE_MODE_MASK		= 2,
		SHADER_SURFACE_MODE_TRANSLUCENT	= 3,
		SHADER_SURFACE_MODE_COUNT,
	};

	struct EShaderRenderTraits
	{
	public:
		EShaderLightingModelType	LightingModel	= EShaderLightingModelType::SHADER_LIGHTING_MODEL_NONE;
		EShaderSurfaceModeType		SurfaceMode		= EShaderSurfaceModeType::SHADER_SURFACE_MODE_NONE;
	public:
		void	Reset() noexcept
		{
			LightingModel	= EShaderLightingModelType::SHADER_LIGHTING_MODEL_NONE;
			SurfaceMode		= EShaderSurfaceModeType::SHADER_SURFACE_MODE_NONE;
		}
		PE_NODISCARD BOOL32	IsAllUnset() const noexcept
		{
			return (((LightingModel == EShaderLightingModelType::SHADER_LIGHTING_MODEL_NONE)
				&& (SurfaceMode == EShaderSurfaceModeType::SHADER_SURFACE_MODE_NONE)) ? TRUE : FALSE);
		}
		PE_NODISCARD BOOL32	IsSameAs(const EShaderRenderTraits& InOther) const noexcept
		{
			return (((LightingModel == InOther.LightingModel)
				&& (SurfaceMode == InOther.SurfaceMode)) ? TRUE : FALSE);
		}
	};

} // namespace PigeonEngine
