#include "SkyLightComponent.h"
#include <RenderProxy/SkyLightProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSkyLightComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSkyLightComponent::PSkyLightComponent()
		: CubeMapAsset(nullptr), LightAdjust(Color3::White()), Intensity(1.f), SceneProxy(nullptr), UpdateState(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_NONE)
	{
	}
	PSkyLightComponent::~PSkyLightComponent()
	{
	}
	const ETextureCubeAsset* PSkyLightComponent::GetCubeMapAsset()const
	{
		return CubeMapAsset;
	}
	const Color3& PSkyLightComponent::GetLightAdjust()const
	{
		return LightAdjust;
	}
	FLOAT PSkyLightComponent::GetIntensity()const
	{
		return Intensity;
	}
	void PSkyLightComponent::SetCubeMapAsset(const ETextureCubeAsset* InCubeMapAsset)
	{
		CubeMapAsset = InCubeMapAsset;
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_TEXTURE);
	}
	void PSkyLightComponent::SetLightAdjust(const Color3& InColor)
	{
		LightAdjust = InColor;
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_PARAMS);
	}
	void PSkyLightComponent::SetIntensity(const FLOAT InIntensity)
	{
		Intensity = EMath::Max(0.001f, InIntensity);
		MarkAsDirty(PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_PARAMS);
	}

	// Render proxy functions START
	UINT8 PSkyLightComponent::GetUpdateRenderState()const
	{
		return UpdateState;
	}
	RSkyLightSceneProxy* PSkyLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating sky light scene proxy, but already exist scene proxy."), (!SceneProxy));
		SceneProxy = new RSkyLightSceneProxy(this);
		return SceneProxy;
	}
	void PSkyLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddSkyLight(this);
		}
	}
	void PSkyLightComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveSkyLight(this);
		PSceneComponent::DestroyRenderState();
	}
	void PSkyLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateSkyLight(this);
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PSkyLightComponent::MarkAsDirty(PSkyLightUpdateState InState)
	{
		if (InState == PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_MATRIX)
		{
			MarkRenderTransformAsDirty();
		}
		else
		{
			UpdateState |= InState;
			MarkRenderStateAsDirty();
		}
	}
	void PSkyLightComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PSkyLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PSkyLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PSkyLightUpdateState::SKY_LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

};