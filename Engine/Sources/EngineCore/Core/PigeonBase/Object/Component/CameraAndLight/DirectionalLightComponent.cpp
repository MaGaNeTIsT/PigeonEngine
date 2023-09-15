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
		: LightData(ELightData(ELightType::LIGHT_TYPE_DIRECTIONAL, 1.f, 1.f, 1.f, 1.f, FALSE, 2, 2)), SceneProxy(nullptr), UpdateState(PLightUpdateState::LIGHT_UPDATE_STATE_NONE)
	{
	}
	PDirectionalLightComponent::~PDirectionalLightComponent()
	{
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
	void PDirectionalLightComponent::SetLightColor(const Color3& InColor)
	{
		LightData.LightColor = InColor;
	}
	void PDirectionalLightComponent::SetLightIntensity(FLOAT InIntensity)
	{
		LightData.LightIntensity = EMath::Max(0.001f, InIntensity);
	}
	void PDirectionalLightComponent::SetLightCastShadow(BOOL32 InIsCastShadow)
	{
		LightData.CastShadow = InIsCastShadow;
	}
	void PDirectionalLightComponent::SetShadowMapSize(const Vector2Int& InShadowMapSize)
	{
		LightData.ShadowMapSize = InShadowMapSize;
	}

	// Render proxy functions START
	UINT8 PDirectionalLightComponent::GetUpdateRenderState()const
	{
		return UpdateState;
	}
	void PDirectionalLightComponent::MarkAsDirty(PLightUpdateState InState)
	{
		UpdateState |= InState;
		MarkRenderStateAsDirty();
	}
	RDirectionalLightSceneProxy* PDirectionalLightComponent::GetSceneProxy()
	{
		return SceneProxy;
	}
	const RDirectionalLightSceneProxy* PDirectionalLightComponent::GetSceneProxy()const
	{
		return SceneProxy;
	}
	RDirectionalLightSceneProxy* PDirectionalLightComponent::CreateSceneProxy()
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Try creating mesh scene proxy, but already exist scene proxy."), (!SceneProxy));
		SceneProxy = new RDirectionalLightSceneProxy(this);
		return SceneProxy;
	}
	void PDirectionalLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PDirectionalLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			PWorldManager::GetWorld()->GetRenderScene()->AddDirectionalLight(this);
		}
	}
	void PDirectionalLightComponent::DestroyRenderState()
	{
		PWorldManager::GetWorld()->GetRenderScene()->RemoveDirectionalLight(this);
		PSceneComponent::DestroyRenderState();
	}
	void PDirectionalLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			PWorldManager::GetWorld()->GetRenderScene()->UpdateDirectionalLight(this);
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PDirectionalLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PLightUpdateState::LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

};