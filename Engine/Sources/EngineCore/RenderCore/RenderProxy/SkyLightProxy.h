#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <Shader/ShaderParameter.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>

namespace PigeonEngine
{

	class RSkyLightMaterialParameter : public EMaterialParameter
	{
	public:
		virtual void	SetupParameters()override;

		CLASS_MATERIAL_PARAMETER(RSkyLightMaterialParameter)
	};

	class RSkyLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RSkyLightSceneProxy(PSkyLightComponent* InComponent);
		void			SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset);
	public:
		void			UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset);
		void			UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams);
		void			UpdateMatrices(const ERenderSkyLightMatrices& InMatrices);
		void			UpdateRenderResource();
	protected:
		RSkyLightMaterialParameter	SkyLightMaterialParameter;
		const ETextureCubeAsset*	TextureCubeAsset;
		Color3						AdjustColor;
		FLOAT						AdjustIntensity;
	protected:
		const PSkyLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RSkyLightSceneProxy)

	};

};