#include "TransportBar.h"

#if _EDITOR_ONLY

#include <imgui.h>
#include "../EditorUi.h"

namespace PigeonEngine
{

	namespace
	{
		const CHAR* TransportLoopModeLabel(const EAnimationLoopMode InMode)
		{
			switch (InMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			case ANIMATION_LOOP_REPEAT:
			default:
				return "Repeat";
			}
		}
	}

	void FTransportBar::Draw(const CHAR* InId, FTransportState& InOutState
		, FTransportInteraction& OutInteraction)
	{
		OutInteraction = FTransportInteraction();
		FImGuiIdScope TransportScope(InId);

		if (ImGui::Button(InOutState.bPlaying != FALSE ? "Pause" : "Play", ImVec2(64.0f, 0.0f)))
		{
			InOutState.bPlaying = ((InOutState.bPlaying != FALSE) ? FALSE : TRUE);
			OutInteraction.bPlaybackChanged = TRUE;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		if (ImGui::SliderFloat("##transport_rate", &InOutState.PlayRate, 0.0f, 4.0f
			, "Rate %.2f", ImGuiSliderFlags_AlwaysClamp))
		{
			OutInteraction.bPlaybackChanged = TRUE;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(105.0f);
		if (ImGui::BeginCombo("##transport_loop", TransportLoopModeLabel(InOutState.LoopMode)))
		{
			const EAnimationLoopMode LoopModes[4] = {
				ANIMATION_LOOP_ONCE, ANIMATION_LOOP_REPEAT, ANIMATION_LOOP_PINGPONG, ANIMATION_LOOP_CLAMP
			};
			for (UINT32 ModeIndex = 0u; ModeIndex < 4u; ModeIndex++)
			{
				const BOOL8 bSelected = (InOutState.LoopMode == LoopModes[ModeIndex]);
				if (ImGui::Selectable(TransportLoopModeLabel(LoopModes[ModeIndex]), bSelected))
				{
					InOutState.LoopMode = LoopModes[ModeIndex];
					OutInteraction.bPlaybackChanged = TRUE;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		constexpr FLOAT FrameStep = 1.0f / 30.0f;
		if (ImGui::Button("-1f"))
		{
			OutInteraction.bSeekRequested = TRUE;
			OutInteraction.SeekTime = EMath::Max(InOutState.Time - FrameStep, 0.0f);
		}
		ImGui::SameLine();
		if (ImGui::Button("+1f"))
		{
			OutInteraction.bSeekRequested = TRUE;
			OutInteraction.SeekTime = EMath::Min(InOutState.Time + FrameStep, InOutState.Duration);
		}
		ImGui::SameLine();
		FLOAT Time = InOutState.Time;
		ImGui::SetNextItemWidth(95.0f);
		ImGui::InputFloat("##transport_time", &Time, 0.0f, 0.0f, "%.3f");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			OutInteraction.bSeekRequested = TRUE;
			OutInteraction.SeekTime = EMath::Clamp(Time, 0.0f, InOutState.Duration);
		}
		ImGui::SameLine();
		ImGui::TextDisabled("/ %.3fs", InOutState.Duration);
	}

}

#endif
