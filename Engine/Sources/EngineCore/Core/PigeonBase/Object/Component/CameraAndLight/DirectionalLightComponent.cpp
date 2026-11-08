#include "DirectionalLightComponent.h"
#include <RenderProxy/LightSceneProxy.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#endif

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
		// The cascade layout a directional light starts with: three slices of 100 / 300 / 1000
		// world units from the view origin, with a 10 and a 50 unit overlap into the slice that
		// follows. The values are thicknesses rather than distances - ECascadeShadowData stores
		// them that way and the render side accumulates them - and the last slice is extended to
		// the camera's far plane, so the ladder only has to describe where the near detail ends.
		//
		// Without this the light has no cascade data at all, IsLightUseCascadeShadow reports false
		// and every directional light falls back to one cascade covering the whole view: the CSM
		// path would exist but nothing could ever select more than one slice of it.
		{
			const FLOAT DefaultLayers[3]	= { 100.f, 300.f, 1000.f };
			const FLOAT DefaultBorders[2]	= { 10.f, 50.f };
			CascadeShadowData = new ECascadeShadowData(TRUE, DefaultLayers, DefaultBorders, 3u);
		}
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

#if ENGINE_EDITOR_ONLY
	void PDirectionalLightComponent::GenerateComponentDetail()
	{
		PLightComponent::GenerateComponentDetail();

		if (ImGui::TreeNodeEx("Directional Light"))
		{
			// Perpendicular illuminance in lux. Daylight values: 100000 in direct sun, 10000 under
			// overcast, 0.25 in full moonlight.
			FLOAT TempIlluminance = GetIlluminanceLux();
			if (ImGui::DragFloat("Illuminance", &TempIlluminance, 100.f, 0.f, 200000.f, "%.0f lux"))
			{
				SetIlluminanceLux(TempIlluminance);
			}

			// The angular diameter of the disc the light stands for - the sun's 0.53 degrees - which
			// is the input both to the specular lobe's width and to a soft shadow's penumbra.
			FLOAT TempSourceAngle = GetSourceAngleDegrees();
			if (ImGui::DragFloat("Source angle", &TempSourceAngle, 0.01f, 0.f, 5.f, "%.2f deg"))
			{
				SetSourceAngleDegrees(TempSourceAngle);
			}
			ImGui::TreePop();
		}
	}
#endif

};
