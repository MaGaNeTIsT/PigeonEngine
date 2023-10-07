#include "SkyLightProxy.h"

namespace PigeonEngine
{

	RSkyLightSceneProxy::RSkyLightSceneProxy(PSkyLightComponent* InComponent)
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(InComponent)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy()
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f), Component(nullptr)
	{
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy(const RSkyLightSceneProxy& Other)
		: RBaseSceneProxy(Other), TextureCubeAsset(Other.TextureCubeAsset), AdjustColor(Other.AdjustColor), AdjustIntensity(Other.AdjustIntensity), Component(Other.Component)
	{
	}
	RSkyLightSceneProxy::~RSkyLightSceneProxy()
	{
	}
	void RSkyLightSceneProxy::SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset)
	{
		UpdateTextureCubeAsset(InTextureCubeAsset);
		UpdateSkyLightParams(InSkyLightParams);
		UpdateMatrices(InMatrices);
	}
	void RSkyLightSceneProxy::UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset)
	{
		TextureCubeAsset = InTextureCubeAsset;
	}
	void RSkyLightSceneProxy::UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams)
	{
		AdjustColor		= InSkyLightParams.LightAdjustColor;
		AdjustIntensity	= InSkyLightParams.LightAdjustIntensity;
	}
	void RSkyLightSceneProxy::UpdateMatrices(const ERenderSkyLightMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}

};