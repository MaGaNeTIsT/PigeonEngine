#include "LightComponent.h"
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
		RegisterClassType<PLightComponent, PSceneComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PLightComponent::PLightComponent()
		: UpdateState(PLightUpdateState::LIGHT_UPDATE_STATE_NONE)
	{
		POBJ_DEBUGNAME_SET(this, "LightComponent");
	}
	PLightComponent::~PLightComponent()
	{
	}
	ELightType PLightComponent::GetLightType() const noexcept
	{
		return (LightData.LightType);
	}
	const Color3& PLightComponent::GetLightColor() const noexcept
	{
		return (LightData.LightColor);
	}
	BOOL32 PLightComponent::IsLightCastShadow() const noexcept
	{
		return (LightData.CastShadow);
	}
	const Vector2Int& PLightComponent::GetShadowMapSize() const noexcept
	{
		return (LightData.ShadowMapSize);
	}
	const ELightData& PLightComponent::GetLightData() const noexcept
	{
		return LightData;
	}
	void PLightComponent::SetLightColor(const Color3& InColor)
	{
		LightData.LightColor = InColor;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLightComponent::SetShadowMapSize(const BOOL32 InIsCastShadow, const Vector2Int& InShadowMapSize)
	{
		LightData.CastShadow = InIsCastShadow;
		LightData.ShadowMapSize = InShadowMapSize;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	UINT8 PLightComponent::GetUpdateRenderState() const noexcept
	{
		return UpdateState;
	}
	void PLightComponent::CreateRenderState()
	{
		PSceneComponent::CreateRenderState();
		if (ShouldRender())
		{
			AddToRenderScene();
		}
	}
	void PLightComponent::DestroyRenderState()
	{
		RemoveFromRenderScene();
		PSceneComponent::DestroyRenderState();
	}
	void PLightComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			UpdateInRenderScene();
		}
		PSceneComponent::SendUpdateRenderState();
	}
	void PLightComponent::MarkAsDirty(PLightUpdateState InState)
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
	void PLightComponent::MarkRenderTransformAsDirty()
	{
		UpdateState |= PLightUpdateState::LIGHT_UPDATE_STATE_MATRIX;
		PSceneComponent::MarkRenderTransformAsDirty();
	}
	void PLightComponent::MarkRenderStateAsDirty()
	{
		PSceneComponent::MarkRenderStateAsDirty();
	}
	void PLightComponent::CleanMarkRenderStateDirty()
	{
		UpdateState = PLightUpdateState::LIGHT_UPDATE_STATE_NONE;
		PSceneComponent::CleanMarkRenderStateDirty();
	}
	// Render proxy functions END

	PLocalLightComponent::PLocalLightComponent()
		: SceneProxy(nullptr)
	{
		LightData.LightType = ELightType::LIGHT_TYPE_POINT;
		POBJ_DEBUGNAME_SET(this, "LocalLightComponent");
	}
	PLocalLightComponent::~PLocalLightComponent()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("SceneProxy is not null in local light component distruction.")), (!SceneProxy));
	}
	FLOAT PLocalLightComponent::GetLuminousIntensityCandela() const noexcept
	{
		return (LightData.LuminousIntensityCandela);
	}
	FLOAT PLocalLightComponent::GetAttenuationRadiusMeters() const noexcept
	{
		return (LightData.AttenuationRadiusMeters);
	}
	FLOAT PLocalLightComponent::GetSourceRadiusMeters() const noexcept
	{
		return (LightData.SourceRadiusMeters);
	}
	void PLocalLightComponent::SetLuminousIntensityCandela(const FLOAT InLuminousIntensityCandela)
	{
		LightData.LuminousIntensityCandela = EMath::Max(0.f, InLuminousIntensityCandela);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLocalLightComponent::SetLuminousFluxLumen(const FLOAT InLuminousFluxLumen)
	{
		// A point light's flux over the whole sphere. The spot type overrides this, because a
		// spot's flux covers a cone and the same lumens therefore mean a brighter beam.
		SetLuminousIntensityCandela(EMath::Max(0.f, InLuminousFluxLumen) / (4.f * PE_PI));
	}
	void PLocalLightComponent::SetAttenuationRadiusMeters(const FLOAT InAttenuationRadiusMeters)
	{
		// A radius of zero would make the windowed falloff divide by zero, and it also means
		// "light nothing", which is never what someone typing 0 wants.
		LightData.AttenuationRadiusMeters = EMath::Max(0.001f, InAttenuationRadiusMeters);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PLocalLightComponent::SetSourceRadiusMeters(const FLOAT InSourceRadiusMeters)
	{
		LightData.SourceRadiusMeters = EMath::Max(0.f, InSourceRadiusMeters);
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}

	// Render proxy functions START
	RLocalLightSceneProxy* PLocalLightComponent::CreateSceneProxy()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Try creating local light scene proxy, but already exist scene proxy.")), (!SceneProxy));
		SceneProxy = new RLocalLightSceneProxy(this);
		return SceneProxy;
	}
	void PLocalLightComponent::AddToRenderScene()
	{
		this->GetWorld()->GetRenderScene()->AddLocalLight(this);
	}
	void PLocalLightComponent::RemoveFromRenderScene()
	{
		this->GetWorld()->GetRenderScene()->RemoveLocalLight(this);
	}
	void PLocalLightComponent::UpdateInRenderScene()
	{
		this->GetWorld()->GetRenderScene()->UpdateLocalLight(this);
	}
	// Render proxy functions END

#if ENGINE_EDITOR_ONLY
	void PLightComponent::GenerateComponentDetail()
	{
		PSceneComponent::GenerateComponentDetail();

		if (ImGui::TreeNodeEx("Light"))
		{
			// Normalized chromaticity rather than a magnitude: how bright the light is belongs to
			// its own unit-explicit field, which each type draws below.
			const Color3& Color = GetLightColor();
			FLOAT TempColor[3] = { Color.r, Color.g, Color.b };
			if (ImGui::ColorEdit3("Color", TempColor))
			{
				SetLightColor(Color3(TempColor[0], TempColor[1], TempColor[2]));
			}
			ImGui::TreePop();
		}
	}
	void PLocalLightComponent::GenerateComponentDetail()
	{
		PLightComponent::GenerateComponentDetail();

		if (ImGui::TreeNodeEx("Local Light"))
		{
			// No upper clamp: passing 0 for both bounds is ImGui's "unbounded" idiom (it treats
			// min == max == 0 as no range), and the setter still refuses a negative value. A local
			// light is physically far below the sun at daylight exposure, so a scene that wants it
			// visible has to be able to author a larger number here.
			FLOAT TempIntensity = GetLuminousIntensityCandela();
			if (ImGui::DragFloat("Luminous intensity", &TempIntensity, 10.f, 0.f, 0.f, "%.0f cd"))
			{
				SetLuminousIntensityCandela(TempIntensity);
			}

			// The cutoff that bounds the shading cost, not the light's size: the falloff is inverse
			// square out to here and zero past it.
			FLOAT TempRadius = GetAttenuationRadiusMeters();
			if (ImGui::DragFloat("Attenuation radius", &TempRadius, 1.f, 0.001f, 5000.f, "%.1f m"))
			{
				SetAttenuationRadiusMeters(TempRadius);
			}
			ImGui::TreePop();
		}
	}
#endif

}   // namespace PigeonEngine
