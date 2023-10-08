#include "DirectionalLightComponent.h"
#include <RenderProxy/LightSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PDirectionalLightComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PDirectionalLightComponent::PDirectionalLightComponent()
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, 1.f, 1.f, 1.f, 1.f, FALSE, 2, 2)), CascadeShadowData(nullptr), SceneProxy(nullptr), UpdateState(PLightUpdateState::LIGHT_UPDATE_STATE_NONE)
	{
	}
	PDirectionalLightComponent::~PDirectionalLightComponent()
	{
		if (CascadeShadowData)
		{
			delete CascadeShadowData;
			CascadeShadowData = nullptr;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("SceneProxy is not null in directional light component distruction."), (!!SceneProxy));
	}
	ELightType PDirectionalLightComponent::GetLightType()const
	{
		return (LightData.LightType);
	}
	const Color3& PDirectionalLightComponent::GetLightColor()const
	{
		return (LightData.LightColor);
	}
	FLOAT PDirectionalLightComponent::GetLightIntensity()const
	{
		return (LightData.LightIntensity);
	}
	BOOL32 PDirectionalLightComponent::IsLightCastShadow()const
	{
		return (LightData.CastShadow);
	}
	const Vector2Int& PDirectionalLightComponent::GetShadowMapSize()const
	{
		return (LightData.ShadowMapSize);
	}
	ECascadeShadowData* PDirectionalLightComponent::GetCascadeShadowData()
	{
		return CascadeShadowData;
	}
	const ECascadeShadowData* PDirectionalLightComponent::GetCascadeShadowData()const
	{
		return CascadeShadowData;
	}
	void PDirectionalLightComponent::SetLightColor(const Color3& InColor)
	{
		LightData.LightColor = InColor;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PDirectionalLightComponent::SetLightIntensity(const FLOAT InIntensity)
	{
		LightData.LightIntensity = EMath::Max(0.001f, InIntensity);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PDirectionalLightComponent::SetShadowMapSize(const BOOL32 InIsCastShadow, const Vector2Int& InShadowMapSize)
	{
		LightData.CastShadow = InIsCastShadow;
		LightData.ShadowMapSize = InShadowMapSize;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	UINT8 PDirectionalLightComponent::GetUpdateRenderState()const
	{
		return UpdateState;
	}
	RDirectionalLightSceneProxy* PDirectionalLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Try creating directional light scene proxy, but already exist scene proxy."), (!SceneProxy));
		SceneProxy = new RDirectionalLightSceneProxy(this);
		return SceneProxy;
	}
	void PDirectionalLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddDirectionalLight(this);
		}
	}
	void PDirectionalLightComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveDirectionalLight(this);
		PSceneComponent::DestroyRenderState();
	}
	void PDirectionalLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateDirectionalLight(this);
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PDirectionalLightComponent::MarkAsDirty(PLightUpdateState InState)
	{
		if (InState == PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX)
		{
			MarkRenderTransformAsDirty();
		}
		else
		{
			UpdateState |= InState;
			MarkRenderStateAsDirty();
		}
	}
	void PDirectionalLightComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PDirectionalLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PDirectionalLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PLightUpdateState::LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

};