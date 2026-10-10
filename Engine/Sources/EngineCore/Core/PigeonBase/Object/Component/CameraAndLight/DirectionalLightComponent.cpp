#include "DirectionalLightComponent.h"
#include <RenderProxy/LightSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PDirectionalLightComponent, PLightComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PDirectionalLightComponent::PDirectionalLightComponent()
		: CascadeShadowData(nullptr), SceneProxy(nullptr)
	{
		LightData.LightType = ELightType::LIGHT_TYPE_DIRECTIONAL;
		POBJ_DEBUGNAME_SET(this, "DirectionalLightComponent");
	}
	PDirectionalLightComponent::~PDirectionalLightComponent()
	{
		if (CascadeShadowData)
		{
			delete CascadeShadowData;
			CascadeShadowData = nullptr;
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("SceneProxy is not null in directional light component distruction.")), (!SceneProxy));
	}
	FLOAT PDirectionalLightComponent::GetIlluminanceLux() const noexcept
	{
		return (LightData.IlluminanceLux);
	}
	FLOAT PDirectionalLightComponent::GetSourceAngleDegrees() const noexcept
	{
		return (LightData.SourceAngleDegrees);
	}
	ECascadeShadowData* PDirectionalLightComponent::GetCascadeShadowData() noexcept
	{
		return CascadeShadowData;
	}
	const ECascadeShadowData* PDirectionalLightComponent::GetCascadeShadowData() const noexcept
	{
		return CascadeShadowData;
	}
	void PDirectionalLightComponent::SetIlluminanceLux(const FLOAT InIlluminanceLux)
	{
		LightData.IlluminanceLux = EMath::Max(0.f, InIlluminanceLux);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PDirectionalLightComponent::SetSourceAngleDegrees(const FLOAT InSourceAngleDegrees)
	{
		LightData.SourceAngleDegrees = EMath::Max(0.f, InSourceAngleDegrees);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	RDirectionalLightSceneProxy* PDirectionalLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Try creating directional light scene proxy, but already exist scene proxy.")), (!SceneProxy));
		SceneProxy = new RDirectionalLightSceneProxy(this);
		return SceneProxy;
	}
	void PDirectionalLightComponent::AddToRenderScene()
	{
		this->GetWorld()->GetRenderScene()->AddDirectionalLight(this);
	}
	void PDirectionalLightComponent::RemoveFromRenderScene()
	{
		this->GetWorld()->GetRenderScene()->RemoveDirectionalLight(this);
	}
	void PDirectionalLightComponent::UpdateInRenderScene()
	{
		this->GetWorld()->GetRenderScene()->UpdateDirectionalLight(this);
	}
	// Render proxy functions END

};
