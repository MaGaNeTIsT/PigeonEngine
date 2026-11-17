#include "AnimationCurvePlot.h"

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

namespace PigeonEngine
{

	void EAnimationCurvePlot::DrawVectorKeys(const CHAR* InLabel
		, const TArray<EBoneAnimationCurve::TPositionKey>& InKeys
		, FLOAT InTimeMax, FLOAT InWidth, FLOAT InHeight)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const ImVec2 Max(Origin.x + InWidth, Origin.y + InHeight);
		DrawList->AddRectFilled(Origin, Max, IM_COL32(30, 32, 38, 255), 3.0f);
		DrawList->AddRect(Origin, Max, IM_COL32(90, 92, 100, 255), 3.0f);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Origin.y + 3.0f), IM_COL32(220, 220, 225, 255), InLabel);
		if (InKeys.Num() <= 0)
		{
			DrawList->AddText(ImVec2(Origin.x + 6.0f, Origin.y + 20.0f), IM_COL32(150, 150, 155, 255), "No keys");
			ImGui::Dummy(ImVec2(InWidth, InHeight));
			return;
		}

		FLOAT Min = FLT_MAX;
		FLOAT MaxValue = -FLT_MAX;
		for (INT32 KeyIndex = 0; KeyIndex < InKeys.Num(); KeyIndex++)
		{
			const Vector3& Value = InKeys[KeyIndex].Value;
			Min = EMath::Min(Min, EMath::Min(Value.x, EMath::Min(Value.y, Value.z)));
			MaxValue = EMath::Max(MaxValue, EMath::Max(Value.x, EMath::Max(Value.y, Value.z)));
		}
		if (MaxValue - Min < 1e-4f)
		{
			Min -= 0.5f;
			MaxValue += 0.5f;
		}
		const FLOAT SafeTimeMax = EMath::Max(InTimeMax, 0.001f);
		const FLOAT Left = Origin.x + 80.0f;
		const FLOAT Right = Max.x - 8.0f;
		const FLOAT Top = Origin.y + 18.0f;
		const FLOAT Bottom = Max.y - 6.0f;
		const ImU32 Colors[3] = { IM_COL32(235, 90, 90, 255), IM_COL32(100, 210, 110, 255), IM_COL32(90, 150, 240, 255) };

		for (INT32 Component = 0; Component < 3; Component++)
		{
			TArray<ImVec2> Points;
			Points.SetNum(InKeys.Num());
			for (INT32 KeyIndex = 0; KeyIndex < InKeys.Num(); KeyIndex++)
			{
				const Vector3& Value = InKeys[KeyIndex].Value;
				const FLOAT ComponentValue = ((Component == 0) ? Value.x : ((Component == 1) ? Value.y : Value.z));
				const FLOAT TimeRatio = EMath::Clamp(InKeys[KeyIndex].Time / SafeTimeMax, 0.0f, 1.0f);
				const FLOAT ValueRatio = EMath::Clamp((ComponentValue - Min) / (MaxValue - Min), 0.0f, 1.0f);
				Points[KeyIndex] = ImVec2(Left + TimeRatio * (Right - Left), Bottom - ValueRatio * (Bottom - Top));
			}
			DrawList->AddPolyline(Points.GetData(), Points.Num(), Colors[Component], 0, 1.5f);
		}

		CHAR RangeText[64] = "";
		::sprintf_s(RangeText, sizeof(RangeText), "%.2f", MaxValue);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Top), IM_COL32(160, 160, 165, 255), RangeText);
		::sprintf_s(RangeText, sizeof(RangeText), "%.2f", Min);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Bottom - 12.0f), IM_COL32(160, 160, 165, 255), RangeText);
		ImGui::Dummy(ImVec2(InWidth, InHeight));
	}

	void EAnimationCurvePlot::DrawRotationKeys(const TArray<EBoneAnimationCurve::TRotationKey>& InKeys
		, FLOAT InTimeMax, FLOAT InWidth, FLOAT InHeight)
	{
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const ImVec2 Max(Origin.x + InWidth, Origin.y + InHeight);
		DrawList->AddRectFilled(Origin, Max, IM_COL32(30, 32, 38, 255), 3.0f);
		DrawList->AddRect(Origin, Max, IM_COL32(90, 92, 100, 255), 3.0f);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Origin.y + 3.0f), IM_COL32(220, 220, 225, 255), "Rotation");
		if (InKeys.Num() <= 0)
		{
			DrawList->AddText(ImVec2(Origin.x + 6.0f, Origin.y + 20.0f), IM_COL32(150, 150, 155, 255), "No keys");
			ImGui::Dummy(ImVec2(InWidth, InHeight));
			return;
		}

		FLOAT Min = FLT_MAX;
		FLOAT MaxValue = -FLT_MAX;
		for (INT32 KeyIndex = 0; KeyIndex < InKeys.Num(); KeyIndex++)
		{
			const Quaternion& Value = InKeys[KeyIndex].Value;
			Min = EMath::Min(Min, EMath::Min(Value.x, EMath::Min(Value.y, EMath::Min(Value.z, Value.w))));
			MaxValue = EMath::Max(MaxValue, EMath::Max(Value.x, EMath::Max(Value.y, EMath::Max(Value.z, Value.w))));
		}
		if (MaxValue - Min < 1e-4f)
		{
			Min -= 0.5f;
			MaxValue += 0.5f;
		}
		const FLOAT SafeTimeMax = EMath::Max(InTimeMax, 0.001f);
		const FLOAT Left = Origin.x + 80.0f;
		const FLOAT Right = Max.x - 8.0f;
		const FLOAT Top = Origin.y + 18.0f;
		const FLOAT Bottom = Max.y - 6.0f;
		const ImU32 Colors[4] = { IM_COL32(235, 90, 90, 255), IM_COL32(100, 210, 110, 255)
			, IM_COL32(90, 150, 240, 255), IM_COL32(235, 210, 90, 255) };

		for (INT32 Component = 0; Component < 4; Component++)
		{
			TArray<ImVec2> Points;
			Points.SetNum(InKeys.Num());
			for (INT32 KeyIndex = 0; KeyIndex < InKeys.Num(); KeyIndex++)
			{
				const Quaternion& Value = InKeys[KeyIndex].Value;
				const FLOAT ComponentValue = ((Component == 0) ? Value.x : ((Component == 1) ? Value.y
					: ((Component == 2) ? Value.z : Value.w)));
				const FLOAT TimeRatio = EMath::Clamp(InKeys[KeyIndex].Time / SafeTimeMax, 0.0f, 1.0f);
				const FLOAT ValueRatio = EMath::Clamp((ComponentValue - Min) / (MaxValue - Min), 0.0f, 1.0f);
				Points[KeyIndex] = ImVec2(Left + TimeRatio * (Right - Left), Bottom - ValueRatio * (Bottom - Top));
			}
			DrawList->AddPolyline(Points.GetData(), Points.Num(), Colors[Component], 0, 1.5f);
		}

		CHAR RangeText[64] = "";
		::sprintf_s(RangeText, sizeof(RangeText), "%.2f", MaxValue);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Top), IM_COL32(160, 160, 165, 255), RangeText);
		::sprintf_s(RangeText, sizeof(RangeText), "%.2f", Min);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Bottom - 12.0f), IM_COL32(160, 160, 165, 255), RangeText);
		ImGui::Dummy(ImVec2(InWidth, InHeight));
	}

}

#endif
