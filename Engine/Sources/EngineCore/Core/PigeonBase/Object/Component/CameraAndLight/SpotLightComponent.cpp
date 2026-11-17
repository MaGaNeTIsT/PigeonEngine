#include "SpotLightComponent.h"

#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSpotLightComponent, PLocalLightComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSpotLightComponent::PSpotLightComponent()
	{
		LightData.LightType = ELightType::LIGHT_TYPE_SPOT;
		POBJ_DEBUGNAME_SET(this, "SpotLightComponent");
	}
	PSpotLightComponent::~PSpotLightComponent()
	{
	}
	FLOAT PSpotLightComponent::GetSpotInnerAngleDegrees() const noexcept
	{
		return (LightData.SpotInnerAngleDegrees);
	}
	FLOAT PSpotLightComponent::GetSpotOuterAngleDegrees() const noexcept
	{
		return (LightData.SpotOuterAngleDegrees);
	}
	void PSpotLightComponent::SetSpotInnerAngleDegrees(const FLOAT InSpotInnerAngleDegrees)
	{
		SetSpotConeAnglesDegrees(InSpotInnerAngleDegrees, LightData.SpotOuterAngleDegrees);
	}
	void PSpotLightComponent::SetSpotOuterAngleDegrees(const FLOAT InSpotOuterAngleDegrees)
	{
		SetSpotConeAnglesDegrees(LightData.SpotInnerAngleDegrees, InSpotOuterAngleDegrees);
	}
	void PSpotLightComponent::SetSpotConeAnglesDegrees(const FLOAT InSpotInnerAngleDegrees, const FLOAT InSpotOuterAngleDegrees)
	{
		const FLOAT MaxHalfAngleDegrees = 89.f;
		const FLOAT Outer = EMath::Clamp(InSpotOuterAngleDegrees, 0.001f, MaxHalfAngleDegrees);
		const FLOAT Inner = EMath::Clamp(InSpotInnerAngleDegrees, 0.f, Outer);

		LightData.SpotInnerAngleDegrees = Inner;
		LightData.SpotOuterAngleDegrees = Outer;
		MarkAsDirty(PLightUpdateState::LIGHT_UPDATE_STATE_DATA);
	}
	void PSpotLightComponent::SetLuminousFluxLumen(const FLOAT InLuminousFluxLumen)
	{
		// The same flux over a smaller solid angle is a brighter beam. The cone's solid angle is
		// the area of a spherical cap, 2 pi (1 - cos(outer)), so I = Phi / that.
		const FLOAT OuterRadians = EMath::DegreesToRadians(EMath::Max(LightData.SpotOuterAngleDegrees, 0.001f));
		const FLOAT ConeSolidAngle = (2.f * PE_PI) * (1.f - EMath::Cos(OuterRadians));
		if (ConeSolidAngle <= 0.f)
		{
			return;
		}
		SetLuminousIntensityCandela(EMath::Max(0.f, InLuminousFluxLumen) / ConeSolidAngle);
	}

#if ENGINE_EDITOR_ONLY
	void PSpotLightComponent::GenerateComponentDetail()
	{
		PLocalLightComponent::GenerateComponentDetail();

		if (ImGui::TreeNodeEx("Spot"))
		{
			// Half angles from the axis. They are written as a pair, because the setter is the one
			// place that is able to keep the inner no wider than the outer.
			FLOAT TempInner = GetSpotInnerAngleDegrees();
			FLOAT TempOuter = GetSpotOuterAngleDegrees();
			const BOOL8 bInnerChanged = ImGui::DragFloat("Inner angle", &TempInner, 0.1f, 0.f, 89.f, "%.2f deg");
			const BOOL8 bOuterChanged = ImGui::DragFloat("Outer angle", &TempOuter, 0.1f, 0.f, 89.f, "%.2f deg");
			if (bInnerChanged || bOuterChanged)
			{
				SetSpotConeAnglesDegrees(TempInner, TempOuter);
			}
			ImGui::TreePop();
		}
	}
#endif

}   // namespace PigeonEngine
