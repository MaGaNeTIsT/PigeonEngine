#pragma once

#include <CoreMinimal.h>
#include "BaseSceneProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <PigeonBase/Object/Component/Primitive/SkyLightComponent.h>

namespace PigeonEngine
{

	class RSkyLightSceneProxy : public RBaseSceneProxy
	{
	public:
		RSkyLightSceneProxy(PSkyLightComponent* InComponent);
		void			SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset);
	public:
		void			UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset);
		void			UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams);
		void			UpdateMatrices(const ERenderSkyLightMatrices& InMatrices);
	public:

	protected:
		const ETextureCubeAsset*	TextureCubeAsset;
		Color3						AdjustColor;
		FLOAT						AdjustIntensity;
	protected:
		const PSkyLightComponent*	Component;

		RENDER_PROXY_CLASS_BODY(RSkyLightSceneProxy)

	};

};