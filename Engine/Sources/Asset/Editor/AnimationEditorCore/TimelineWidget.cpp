#include "TimelineWidget.h"

#if ENGINE_EDITOR_ONLY

#include "../EditorUi.h"
#include <cstdio>
#include <cfloat>
#include <cmath>

namespace PigeonEngine
{

	namespace
	{
		FLOAT ChooseTimelineStep(const FLOAT InRawStep)
		{
			const FLOAT Steps[] = { 0.01f, 0.02f, 0.05f, 0.1f, 0.2f, 0.5f, 1.0f, 2.0f
				, 5.0f, 10.0f, 20.0f, 30.0f, 60.0f, 120.0f, 300.0f, 600.0f };
			for (INT32 StepIndex = 0; StepIndex < static_cast<INT32>(sizeof(Steps) / sizeof(Steps[0])); StepIndex++)
			{
				if (Steps[StepIndex] >= InRawStep)
				{
					return Steps[StepIndex];
				}
			}
			return Steps[static_cast<INT32>(sizeof(Steps) / sizeof(Steps[0])) - 1];
		}

		struct FLaneItem
		{
			INT32	MarkerIndex;
			FLOAT	Start;
			FLOAT	End;
		};

		struct FTrackLanes
		{
			TArray<TArray<FLaneItem>> Lanes;
		};

		struct FRowLayout
		{
			INT32	TrackIndex;
			INT32	LaneIndex;
			FLOAT	Y;
		};

		void BuildTrackLanes(const FTimelineTrack& InTrack, FTrackLanes& OutLanes)
		{
			OutLanes.Lanes.Empty();
			TArray<INT32> SortedIndices;
			for (INT32 MarkerIndex = 0; MarkerIndex < InTrack.Markers.Num(); MarkerIndex++)
			{
				SortedIndices.Add(MarkerIndex);
			}
			for (INT32 Outer = 1; Outer < SortedIndices.Num(); Outer++)
			{
				const INT32 Value = SortedIndices[Outer];
				INT32 Inner = Outer - 1;
				while ((Inner >= 0) && (InTrack.Markers[SortedIndices[Inner]].Time > InTrack.Markers[Value].Time))
				{
					SortedIndices[Inner + 1] = SortedIndices[Inner];
					Inner--;
				}
				SortedIndices[Inner + 1] = Value;
			}

			TArray<FLOAT> LaneEnds;
			for (INT32 SortedIndex = 0; SortedIndex < SortedIndices.Num(); SortedIndex++)
			{
				const INT32 MarkerIndex = SortedIndices[SortedIndex];
				const FTimelineMarker& Marker = InTrack.Markers[MarkerIndex];
				const FLOAT Start = Marker.Time;
				const FLOAT End = ((Marker.bRange != FALSE) ? (Marker.Time + Marker.Duration) : Marker.Time);
				INT32 TargetLane = -1;
				for (INT32 LaneIndex = 0; LaneIndex < LaneEnds.Num(); LaneIndex++)
				{
					// Same-time instant markers overlap visually, so a lane is reusable only
					// when the new marker starts strictly after the previous one ends.
					if (Start > LaneEnds[LaneIndex])
					{
						TargetLane = LaneIndex;
						break;
					}
				}
				if (TargetLane < 0)
				{
					TargetLane = LaneEnds.Num();
					LaneEnds.Add(End);
					OutLanes.Lanes.Add(TArray<FLaneItem>());
				}
				else
				{
					LaneEnds[TargetLane] = EMath::Max(LaneEnds[TargetLane], End);
				}
				FLaneItem Item;
				Item.MarkerIndex = MarkerIndex;
				Item.Start = Start;
				Item.End = End;
				OutLanes.Lanes[TargetLane].Add(Item);
			}
			if (OutLanes.Lanes.Num() <= 0)
			{
				OutLanes.Lanes.Add(TArray<FLaneItem>());
			}
		}
	}

	BOOL32 FTimelineWidget::IsMarkerSelected(INT32 InTrackIndex, INT32 InMarkerIndex)const
	{
		for (INT32 RefIndex = 0; RefIndex < m_SelectedMarkers.Num(); RefIndex++)
		{
			if ((m_SelectedMarkers[RefIndex].TrackIndex == InTrackIndex)
				&& (m_SelectedMarkers[RefIndex].MarkerIndex == InMarkerIndex))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	void FTimelineWidget::SetSingleSelection(INT32 InTrackIndex, INT32 InMarkerIndex)
	{
		m_SelectedMarkers.Empty();
		if ((InTrackIndex >= 0) && (InMarkerIndex >= 0))
		{
			FTimelineMarkerRef Ref;
			Ref.TrackIndex = InTrackIndex;
			Ref.MarkerIndex = InMarkerIndex;
			m_SelectedMarkers.Add(Ref);
		}
		m_LastInputTrack = InTrackIndex;
		m_LastInputMarker = InMarkerIndex;
	}

	void FTimelineWidget::ToggleSelection(INT32 InTrackIndex, INT32 InMarkerIndex)
	{
		FTimelineMarkerRef Ref;
		Ref.TrackIndex = InTrackIndex;
		Ref.MarkerIndex = InMarkerIndex;
		const INT32 ExistingIndex = m_SelectedMarkers.Find(Ref);
		if (ExistingIndex >= 0)
		{
			m_SelectedMarkers.RemoveAt(ExistingIndex);
		}
		else if (Ref.IsValid() != FALSE)
		{
			m_SelectedMarkers.Add(Ref);
		}
		const FTimelineMarkerRef Primary = (m_SelectedMarkers.Num() > 0)
			? m_SelectedMarkers[m_SelectedMarkers.Num() - 1] : FTimelineMarkerRef();
		m_LastInputTrack = Primary.TrackIndex;
		m_LastInputMarker = Primary.MarkerIndex;
	}

	void FTimelineWidget::Draw(const CHAR* InId, const TArray<FTimelineTrack>& InTracks
		, FLOAT InDuration, FLOAT InCurrentTime, INT32 InSelectedTrack, INT32 InSelectedMarker
		, FTimelineInteraction& OutInteraction)
	{
		OutInteraction = FTimelineInteraction();
		OutInteraction.SelectedTrack = InSelectedTrack;
		OutInteraction.SelectedMarker = InSelectedMarker;
		if (InDuration <= 0.0f)
		{
			ImGui::TextDisabled("No timeline duration.");
			return;
		}

		// The panel owns the primary selection. If it changed outside the widget (inspector,
		// delete, document reload), reset the multi-selection to that single marker.
		if ((InSelectedTrack != m_LastInputTrack) || (InSelectedMarker != m_LastInputMarker))
		{
			SetSingleSelection(InSelectedTrack, InSelectedMarker);
		}
		if (m_SelectedMarkers.IsEmpty() && (InSelectedMarker >= 0))
		{
			SetSingleSelection(InSelectedTrack, InSelectedMarker);
		}

		FImGuiIdScope TimelineScope(InId);
		constexpr FLOAT LabelWidth = 118.0f;
		constexpr FLOAT RulerHeight = 20.0f;
		constexpr FLOAT LaneHeight = 24.0f;
		constexpr FLOAT TrackGap = 4.0f;
		constexpr FLOAT BottomPadding = 6.0f;
		const FLOAT AvailableWidth = EMath::Max(ImGui::GetContentRegionAvail().x, 260.0f);
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const FLOAT TimelineX = Origin.x + LabelWidth + 6.0f;
		const FLOAT TimelineWidth = EMath::Max(AvailableWidth - LabelWidth - 12.0f, 80.0f);

		TArray<FTrackLanes> TrackLanes;
		TArray<FRowLayout> Rows;
		FLOAT CurrentY = Origin.y + RulerHeight;
		INT32 TotalLanes = 0;
		for (INT32 TrackIndex = 0; TrackIndex < InTracks.Num(); TrackIndex++)
		{
			FTrackLanes Lanes;
			BuildTrackLanes(InTracks[TrackIndex], Lanes);
			TrackLanes.Add(Lanes);
			for (INT32 LaneIndex = 0; LaneIndex < Lanes.Lanes.Num(); LaneIndex++)
			{
				FRowLayout Row;
				Row.TrackIndex = TrackIndex;
				Row.LaneIndex = LaneIndex;
				Row.Y = CurrentY;
				Rows.Add(Row);
				CurrentY += LaneHeight;
				TotalLanes++;
			}
			CurrentY += TrackGap;
		}
		const FLOAT TotalHeight = RulerHeight
			+ static_cast<FLOAT>(TotalLanes) * LaneHeight
			+ static_cast<FLOAT>(InTracks.Num()) * TrackGap + BottomPadding;

		ImGui::InvisibleButton("##timeline_canvas", ImVec2(AvailableWidth, TotalHeight));
		const BOOL32 bHovered = ImGui::IsItemHovered();
		const BOOL32 bActive = ImGui::IsItemActive();
		if ((ImGui::GetIO().KeyCtrl != false) || (ImGui::GetIO().KeyShift != false))
		{
			// Zoom/pan consume the wheel; otherwise the surrounding panel would also scroll.
			ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
		}
		const ImVec2 MousePosition = ImGui::GetIO().MousePos;
		const ImGuiIO& IO = ImGui::GetIO();

		if ((m_ViewDuration <= 0.0f) || (m_LastDuration != InDuration))
		{
			m_ViewStart = 0.0f;
			m_ViewDuration = InDuration;
			m_LastDuration = InDuration;
			m_bPanning = FALSE;
		}
		const FLOAT MinViewDuration = EMath::Max(0.01f, InDuration * 0.01f);
		m_ViewDuration = EMath::Clamp(m_ViewDuration, MinViewDuration, InDuration);
		m_ViewStart = EMath::Clamp(m_ViewStart, 0.0f, EMath::Max(0.0f, InDuration - m_ViewDuration));

		auto ViewAlpha = [TimelineX, TimelineWidth](const FLOAT InX) -> FLOAT
		{
			return (InX - TimelineX) / TimelineWidth;
		};
		auto ViewXToTimeUnclamped = [this, &ViewAlpha](const FLOAT InX) -> FLOAT
		{
			return m_ViewStart + ViewAlpha(InX) * m_ViewDuration;
		};

		const BOOL32 bOverTimeline = ((bHovered != FALSE)
			&& (MousePosition.x >= TimelineX)
			&& (MousePosition.x <= TimelineX + TimelineWidth)) ? TRUE : FALSE;
		if ((bOverTimeline != FALSE) && (m_bDraggingMarker == FALSE)
			&& (m_bResizingMarker == FALSE) && (m_bScrubbing == FALSE))
		{
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Middle))
			{
				m_ViewStart = 0.0f;
				m_ViewDuration = InDuration;
				m_bPanning = FALSE;
			}
			else if (IO.MouseWheel != 0.0f)
			{
				if (IO.KeyCtrl != false)
				{
					const FLOAT AnchorAlpha = ViewAlpha(MousePosition.x);
					const FLOAT AnchorTime = EMath::Clamp(ViewXToTimeUnclamped(MousePosition.x)
						, 0.0f, InDuration);
					const FLOAT NewDuration = EMath::Clamp(m_ViewDuration
						* ((IO.MouseWheel > 0.0f) ? 0.85f : 1.18f), MinViewDuration, InDuration);
					m_ViewDuration = NewDuration;
					m_ViewStart = EMath::Clamp(AnchorTime - AnchorAlpha * NewDuration
						, 0.0f, EMath::Max(0.0f, InDuration - m_ViewDuration));
				}
				else if (IO.KeyShift != false)
				{
					m_ViewStart = EMath::Clamp(m_ViewStart - IO.MouseWheel * m_ViewDuration * 0.1f
						, 0.0f, EMath::Max(0.0f, InDuration - m_ViewDuration));
				}
			}
			if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
			{
				m_bPanning = TRUE;
			}
		}
		if (m_bPanning != FALSE)
		{
			if (ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				m_ViewStart = EMath::Clamp(m_ViewStart - IO.MouseDelta.x / TimelineWidth * m_ViewDuration
					, 0.0f, EMath::Max(0.0f, InDuration - m_ViewDuration));
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			else
			{
				m_bPanning = FALSE;
			}
		}

		if ((IO.WantTextInput == false)
			&& (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false)
			&& (ImGui::IsKeyPressed(ImGuiKey_Escape, false) != false)
			&& (m_SelectedMarkers.Num() > 0))
		{
			SetSingleSelection(-1, -1);
			m_bBoxSelecting = FALSE;
			OutInteraction.bMarkerSelected = TRUE;
			OutInteraction.bMarkerSelectionChanged = TRUE;
			OutInteraction.SelectedTrack = -1;
			OutInteraction.SelectedMarker = -1;
		}

		const FLOAT ViewStart = m_ViewStart;
		const FLOAT ViewDuration = m_ViewDuration;
		const FLOAT ViewEnd = ViewStart + ViewDuration;
		auto TimeToX = [TimelineX, TimelineWidth, ViewStart, ViewDuration](const FLOAT InTime) -> FLOAT
		{
			return TimelineX + ((InTime - ViewStart) / ViewDuration) * TimelineWidth;
		};
		auto XToTime = [TimelineX, TimelineWidth, ViewStart, ViewDuration, InDuration](const FLOAT InX) -> FLOAT
		{
			const FLOAT Alpha = (InX - TimelineX) / TimelineWidth;
			return EMath::Clamp(ViewStart + Alpha * ViewDuration, 0.0f, InDuration);
		};

		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		DrawList->AddRectFilled(Origin, ImVec2(Origin.x + AvailableWidth, Origin.y + TotalHeight)
			, IM_COL32(28, 30, 35, 255));
		DrawList->AddRectFilled(Origin, ImVec2(Origin.x + LabelWidth, Origin.y + TotalHeight)
			, IM_COL32(34, 36, 42, 255));
		DrawList->AddLine(ImVec2(Origin.x + LabelWidth, Origin.y)
			, ImVec2(Origin.x + LabelWidth, Origin.y + TotalHeight), IM_COL32(70, 72, 80, 255));

		const FLOAT RulerY = Origin.y + RulerHeight;
		DrawList->AddLine(ImVec2(Origin.x, RulerY), ImVec2(Origin.x + AvailableWidth, RulerY)
			, IM_COL32(80, 82, 90, 255));
		const FLOAT RawStep = EMath::Max(ViewDuration / 8.0f, 0.001f);
		const FLOAT TickStep = ChooseTimelineStep(RawStep);
		const FLOAT TickSpacing = (TickStep / ViewDuration) * TimelineWidth;
		const INT32 LabelEvery = EMath::Max(1, static_cast<INT32>(::ceilf(60.0f / EMath::Max(TickSpacing, 1.0f))));
		const FLOAT FirstTick = ::ceilf(ViewStart / TickStep) * TickStep;
		INT32 TickIndex = 0;
		for (FLOAT TickTime = FirstTick; TickTime <= (ViewEnd + TickStep * 0.001f); TickTime += TickStep)
		{
			const FLOAT X = TimeToX(TickTime);
			DrawList->AddLine(ImVec2(X, RulerY - 5.0f), ImVec2(X, RulerY + 2.0f)
				, IM_COL32(130, 132, 140, 255));
			DrawList->AddLine(ImVec2(X, RulerY), ImVec2(X, Origin.y + TotalHeight)
				, IM_COL32(48, 50, 58, 255));
			if ((TickIndex % LabelEvery) == 0)
			{
				CHAR TimeText[32] = "";
				::snprintf(TimeText, sizeof(TimeText)
					, (TickStep < 1.0f) ? "%.2fs" : "%.1fs", TickTime);
				DrawList->AddText(ImVec2(X + 2.0f, Origin.y + 2.0f)
					, IM_COL32(160, 162, 170, 255), TimeText);
			}
			TickIndex++;
		}

		CHAR ViewText[64] = "";
		::snprintf(ViewText, sizeof(ViewText), "View %.2f-%.2fs", ViewStart, ViewEnd);
		DrawList->AddText(ImVec2(Origin.x + 6.0f, Origin.y + 2.0f)
			, IM_COL32(120, 125, 138, 255), ViewText);

		DrawList->PushClipRect(ImVec2(TimelineX, Origin.y)
			, ImVec2(TimelineX + TimelineWidth, Origin.y + TotalHeight), true);
		INT32 HoverTrack = -1;
		INT32 HoverMarker = -1;
		BOOL32 bHoverResize = FALSE;
		BOOL32 bHoverResizeLeft = FALSE;
		for (INT32 RowIndex = 0; RowIndex < Rows.Num(); RowIndex++)
		{
			const FRowLayout& Row = Rows[RowIndex];
			const FTimelineTrack& Track = InTracks[Row.TrackIndex];
			const FTrackLanes& Lanes = TrackLanes[Row.TrackIndex];
			const FLOAT RowMinY = Row.Y;
			const FLOAT RowMaxY = RowMinY + LaneHeight;
			if ((Row.LaneIndex % 2) != 0)
			{
				DrawList->AddRectFilled(ImVec2(Origin.x + LabelWidth, RowMinY)
					, ImVec2(Origin.x + AvailableWidth, RowMaxY), IM_COL32(32, 34, 40, 255));
			}
			if (OutInteraction.SelectedTrack == Row.TrackIndex)
			{
				DrawList->AddRectFilled(ImVec2(Origin.x + LabelWidth, RowMinY)
					, ImVec2(Origin.x + AvailableWidth, RowMaxY), IM_COL32(48, 58, 78, 150));
			}
			if (Row.LaneIndex == 0)
			{
				DrawList->AddText(ImVec2(Origin.x + 6.0f, RowMinY + 5.0f)
					, IM_COL32(210, 212, 220, 255), *Track.Name);
			}
			else
			{
				DrawList->AddText(ImVec2(Origin.x + 16.0f, RowMinY + 5.0f)
					, IM_COL32(120, 125, 138, 255), "lane");
			}

			const TArray<FLaneItem>& Items = Lanes.Lanes[Row.LaneIndex];
			for (INT32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++)
			{
				const FLaneItem& Item = Items[ItemIndex];
				const FTimelineMarker& Marker = Track.Markers[Item.MarkerIndex];
				FLOAT MarkerTime = Marker.Time;
				FLOAT MarkerDuration = Marker.Duration;
				if ((m_bDraggingMarker != FALSE) && (m_DragTrack == Row.TrackIndex)
					&& (m_DragMarker == Item.MarkerIndex))
				{
					MarkerTime = m_DragTime;
				}
				if ((m_bResizingMarker != FALSE) && (m_DragTrack == Row.TrackIndex)
					&& (m_DragMarker == Item.MarkerIndex))
				{
					MarkerTime = m_ResizeTime;
					MarkerDuration = m_ResizeDuration;
				}
				else if ((m_bMultiDragging != FALSE)
					&& (IsMarkerSelected(Row.TrackIndex, Item.MarkerIndex) != FALSE))
				{
					MarkerTime += m_MultiDragDelta;
				}
				const FLOAT MarkerX = TimeToX(MarkerTime);
				const FLOAT MarkerEndX = TimeToX(MarkerTime + MarkerDuration);
				const BOOL32 bSelectedMarker = IsMarkerSelected(Row.TrackIndex, Item.MarkerIndex);
				const ImU32 MarkerColor = (bSelectedMarker != FALSE)
					? IM_COL32(255, 210, 90, 255) : Marker.Color;

				if (Marker.bRange != FALSE)
				{
					const ImVec2 RangeMin(MarkerX, RowMinY + 3.0f);
					const ImVec2 RangeMax(EMath::Max(MarkerEndX, MarkerX + 2.0f), RowMaxY - 3.0f);
					DrawList->AddRectFilled(RangeMin, RangeMax, (MarkerColor & 0x00FFFFFF) | 0x90000000, 2.0f);
					DrawList->AddRect(RangeMin, RangeMax, MarkerColor, 2.0f);
					DrawList->AddText(ImVec2(RangeMin.x + 4.0f, RangeMin.y), MarkerColor, *Marker.Label);
					if (Marker.bResizable != FALSE)
					{
						DrawList->AddRectFilled(ImVec2(RangeMin.x - 2.0f, RangeMin.y)
							, ImVec2(RangeMin.x + 2.0f, RangeMax.y), MarkerColor);
						DrawList->AddRectFilled(ImVec2(RangeMax.x - 2.0f, RangeMin.y)
							, ImVec2(RangeMax.x + 2.0f, RangeMax.y), MarkerColor);
					}
					if ((bHovered != FALSE)
						&& (MousePosition.x >= RangeMin.x - 4.0f) && (MousePosition.x <= RangeMax.x + 4.0f)
						&& (MousePosition.y >= RangeMin.y) && (MousePosition.y <= RangeMax.y))
					{
						HoverTrack = Row.TrackIndex;
						HoverMarker = Item.MarkerIndex;
						// Keep a body zone even for short NotifyStates. Fixed 5 px edge zones
						// overlap on a narrow marker, so a click in its middle was treated as a
						// left-edge resize and moved the start to the cursor.
						const FLOAT RangeWidth = EMath::Max(RangeMax.x - RangeMin.x, 1.0f);
						const FLOAT EdgeGrabWidth = EMath::Min(5.0f
							, EMath::Max(0.5f, RangeWidth * 0.2f));
						const BOOL32 bNearLeft = ((Marker.bResizable != FALSE)
							&& (EMath::Abs(MousePosition.x - RangeMin.x) <= EdgeGrabWidth)) ? TRUE : FALSE;
						const BOOL32 bNearRight = ((Marker.bResizable != FALSE)
							&& (EMath::Abs(MousePosition.x - RangeMax.x) <= EdgeGrabWidth)) ? TRUE : FALSE;
						bHoverResize = ((bNearLeft != FALSE) || (bNearRight != FALSE)) ? TRUE : FALSE;
						bHoverResizeLeft = bNearLeft;
					}
				}
				else
				{
					DrawList->AddLine(ImVec2(MarkerX, RowMinY + 4.0f)
						, ImVec2(MarkerX, RowMaxY - 4.0f), MarkerColor, 2.0f);
					DrawList->AddTriangleFilled(ImVec2(MarkerX, RowMinY + 2.0f)
						, ImVec2(MarkerX - 4.0f, RowMinY + 7.0f)
						, ImVec2(MarkerX + 4.0f, RowMinY + 7.0f), MarkerColor);
					if ((bHovered != FALSE) && (EMath::Abs(MousePosition.x - MarkerX) <= 5.0f)
						&& (MousePosition.y >= RowMinY) && (MousePosition.y <= RowMaxY))
					{
						HoverTrack = Row.TrackIndex;
						HoverMarker = Item.MarkerIndex;
					}
				}
			}
		}

		const FLOAT PlayheadX = TimeToX(InCurrentTime);
		DrawList->AddLine(ImVec2(PlayheadX, Origin.y + RulerHeight - 4.0f)
			, ImVec2(PlayheadX, Origin.y + TotalHeight), IM_COL32(245, 220, 90, 255), 2.0f);
		DrawList->AddTriangleFilled(ImVec2(PlayheadX, Origin.y + RulerHeight - 4.0f)
			, ImVec2(PlayheadX - 4.0f, Origin.y + RulerHeight + 1.0f)
			, ImVec2(PlayheadX + 4.0f, Origin.y + RulerHeight + 1.0f), IM_COL32(245, 220, 90, 255));
		if ((m_bBoxSelecting != FALSE) && (ImGui::IsMouseDown(ImGuiMouseButton_Left) != false))
		{
			const ImVec2 BoxMin(EMath::Clamp(EMath::Min(m_BoxStart.x, MousePosition.x)
				, TimelineX, TimelineX + TimelineWidth)
				, EMath::Clamp(EMath::Min(m_BoxStart.y, MousePosition.y)
					, Origin.y + RulerHeight, Origin.y + TotalHeight));
			const ImVec2 BoxMax(EMath::Clamp(EMath::Max(m_BoxStart.x, MousePosition.x)
				, TimelineX, TimelineX + TimelineWidth)
				, EMath::Clamp(EMath::Max(m_BoxStart.y, MousePosition.y)
					, Origin.y + RulerHeight, Origin.y + TotalHeight));
			DrawList->AddRectFilled(BoxMin, BoxMax, IM_COL32(90, 140, 220, 50));
			DrawList->AddRect(BoxMin, BoxMax, IM_COL32(140, 180, 245, 220));
		}
		DrawList->PopClipRect();

		if ((HoverTrack >= 0) && (HoverMarker >= 0))
		{
			const FTimelineMarker& Marker = InTracks[HoverTrack].Markers[HoverMarker];
			if (bHoverResize != FALSE)
			{
				ImGui::SetTooltip("%s: drag either edge to resize", *Marker.Label);
				ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
			}
			else if (Marker.bRange != FALSE)
			{
				ImGui::SetTooltip("%s  %.3fs + %.3fs", *Marker.Label, Marker.Time, Marker.Duration);
			}
			else
			{
				ImGui::SetTooltip("%s @ %.3fs", *Marker.Label, Marker.Time);
			}
		}

		if ((bHovered != FALSE) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& (IO.KeyShift != false) && (bOverTimeline != FALSE))
		{
			// Shift+left drag is box selection; plain left drag stays scrub.
			m_bBoxSelecting = TRUE;
			m_BoxStart = MousePosition;
			m_bScrubbing = FALSE;
			m_bDraggingMarker = FALSE;
			m_bResizingMarker = FALSE;
			m_bMultiDragging = FALSE;
		}
		else if ((bHovered != FALSE) && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& (m_bBoxSelecting == FALSE) && (IO.KeyShift == false))
		{
			if ((HoverTrack >= 0) && (HoverMarker >= 0))
			{
				const FTimelineMarker& Marker = InTracks[HoverTrack].Markers[HoverMarker];
				OutInteraction.bMarkerSelected = TRUE;
				OutInteraction.SelectedTrack = HoverTrack;
				OutInteraction.SelectedMarker = HoverMarker;
				m_bScrubbing = FALSE;
				if (IO.KeyCtrl != false)
				{
					ToggleSelection(HoverTrack, HoverMarker);
					if (m_SelectedMarkers.Num() > 0)
					{
						const FTimelineMarkerRef Primary = m_SelectedMarkers[m_SelectedMarkers.Num() - 1];
						OutInteraction.SelectedTrack = Primary.TrackIndex;
						OutInteraction.SelectedMarker = Primary.MarkerIndex;
					}
					else
					{
						OutInteraction.SelectedTrack = -1;
						OutInteraction.SelectedMarker = -1;
					}
					OutInteraction.bMarkerSelectionChanged = TRUE;
					m_bDraggingMarker = FALSE;
					m_bResizingMarker = FALSE;
					m_bMultiDragging = FALSE;
				}
				else
				{
					m_bDraggingMarker = FALSE;
					m_bResizingMarker = FALSE;
					m_bMultiDragging = FALSE;
					if (bHoverResize != FALSE)
					{
						SetSingleSelection(HoverTrack, HoverMarker);
						m_bResizingMarker = TRUE;
						m_bResizeLeftEdge = bHoverResizeLeft;
						m_DragTrack = HoverTrack;
						m_DragMarker = HoverMarker;
						m_ResizeTime = Marker.Time;
						m_ResizeDuration = Marker.Duration;
					}
					else if (Marker.bDraggable != FALSE)
					{
						if ((IsMarkerSelected(HoverTrack, HoverMarker) != FALSE)
							&& (m_SelectedMarkers.Num() > 1))
						{
							// Drag the whole selection by one shared delta.
							m_bMultiDragging = TRUE;
							m_DragTrack = HoverTrack;
							m_DragMarker = HoverMarker;
							m_MultiDragStartTime = XToTime(MousePosition.x);
							m_MultiDragDelta = 0.0f;
							m_MultiDragRefs.Empty();
							m_MultiDragBaseTimes.Empty();
							m_MultiDragDurations.Empty();
							for (INT32 RefIndex = 0; RefIndex < m_SelectedMarkers.Num(); RefIndex++)
							{
								const FTimelineMarkerRef& Ref = m_SelectedMarkers[RefIndex];
								if ((Ref.TrackIndex >= 0) && (Ref.TrackIndex < InTracks.Num())
									&& (Ref.MarkerIndex >= 0)
									&& (Ref.MarkerIndex < InTracks[Ref.TrackIndex].Markers.Num()))
								{
									const FTimelineMarker& SelectedMarker = InTracks[Ref.TrackIndex].Markers[Ref.MarkerIndex];
									m_MultiDragRefs.Add(Ref);
									m_MultiDragBaseTimes.Add(SelectedMarker.Time);
									m_MultiDragDurations.Add((SelectedMarker.bRange != FALSE)
										? SelectedMarker.Duration : 0.0f);
								}
							}
							m_LastInputTrack = HoverTrack;
							m_LastInputMarker = HoverMarker;
						}
						else
						{
							SetSingleSelection(HoverTrack, HoverMarker);
							m_bDraggingMarker = TRUE;
							m_DragTrack = HoverTrack;
							m_DragMarker = HoverMarker;
							m_DragTime = Marker.Time;
							// Remember where inside the marker the user grabbed it. Without this a
							// click on the body of a NotifyState immediately snaps its start to the
							// cursor, even before the mouse moves.
							m_DragOffsetTime = XToTime(MousePosition.x) - Marker.Time;
						}
					}
					else
					{
						SetSingleSelection(HoverTrack, HoverMarker);
					}
					OutInteraction.bMarkerSelectionChanged = TRUE;
				}
			}
				else
				{
					// Clicking outside a marker leaves marker selection. Keep scrub/track
					// selection working, but clear the multi-selection first.
					const BOOL32 bHadSelection = ((m_SelectedMarkers.Num() > 0) ? TRUE : FALSE);
					SetSingleSelection(-1, -1);
					if (bHadSelection != FALSE)
					{
						OutInteraction.bMarkerSelected = TRUE;
						OutInteraction.bMarkerSelectionChanged = TRUE;
						OutInteraction.SelectedTrack = -1;
						OutInteraction.SelectedMarker = -1;
					}
					INT32 TrackAtMouse = -1;
					for (INT32 RowIndex = 0; RowIndex < Rows.Num(); RowIndex++)
					{
						const FRowLayout& Row = Rows[RowIndex];
						if ((MousePosition.y >= Row.Y) && (MousePosition.y < (Row.Y + LaneHeight)))
						{
							TrackAtMouse = Row.TrackIndex;
							break;
						}
					}
					if ((MousePosition.x < TimelineX) && (TrackAtMouse >= 0))
					{
						OutInteraction.bTrackSelected = TRUE;
						OutInteraction.SelectedTrack = TrackAtMouse;
					}
					else if ((MousePosition.x >= TimelineX)
						&& (MousePosition.x <= TimelineX + TimelineWidth))
					{
						m_bScrubbing = TRUE;
						OutInteraction.bScrubbed = TRUE;
						OutInteraction.ScrubbedTime = XToTime(MousePosition.x);
					}
				}
		}

		if ((m_bBoxSelecting != FALSE) && (ImGui::IsMouseReleased(ImGuiMouseButton_Left) != false))
		{
			const ImVec2 BoxMin(EMath::Min(m_BoxStart.x, MousePosition.x)
				, EMath::Min(m_BoxStart.y, MousePosition.y));
			const ImVec2 BoxMax(EMath::Max(m_BoxStart.x, MousePosition.x)
				, EMath::Max(m_BoxStart.y, MousePosition.y));
			m_SelectedMarkers.Empty();
			for (INT32 RowIndex = 0; RowIndex < Rows.Num(); RowIndex++)
			{
				const FRowLayout& Row = Rows[RowIndex];
				const FTimelineTrack& Track = InTracks[Row.TrackIndex];
				const TArray<FLaneItem>& Items = TrackLanes[Row.TrackIndex].Lanes[Row.LaneIndex];
				const FLOAT RowMinY = Row.Y;
				const FLOAT RowMaxY = RowMinY + LaneHeight;
				if ((RowMaxY < BoxMin.y) || (RowMinY > BoxMax.y))
				{
					continue;
				}
				for (INT32 ItemIndex = 0; ItemIndex < Items.Num(); ItemIndex++)
				{
					const FLaneItem& Item = Items[ItemIndex];
					const FTimelineMarker& Marker = Track.Markers[Item.MarkerIndex];
					const FLOAT StartX = TimeToX(Marker.Time);
					const FLOAT EndX = TimeToX(Marker.Time + ((Marker.bRange != FALSE) ? Marker.Duration : 0.0f));
					if (((StartX >= BoxMin.x) && (StartX <= BoxMax.x))
						|| ((EndX >= BoxMin.x) && (EndX <= BoxMax.x))
						|| ((StartX <= BoxMin.x) && (EndX >= BoxMax.x)))
					{
						FTimelineMarkerRef Ref;
						Ref.TrackIndex = Row.TrackIndex;
						Ref.MarkerIndex = Item.MarkerIndex;
						m_SelectedMarkers.Add(Ref);
					}
				}
			}
			m_bBoxSelecting = FALSE;
			OutInteraction.bMarkerSelectionChanged = TRUE;
			OutInteraction.bMarkerSelected = TRUE;
			if (m_SelectedMarkers.Num() > 0)
			{
				const FTimelineMarkerRef Primary = m_SelectedMarkers[0];
				m_LastInputTrack = Primary.TrackIndex;
				m_LastInputMarker = Primary.MarkerIndex;
				OutInteraction.SelectedTrack = Primary.TrackIndex;
				OutInteraction.SelectedMarker = Primary.MarkerIndex;
			}
			else
			{
				m_LastInputTrack = -1;
				m_LastInputMarker = -1;
				OutInteraction.SelectedTrack = -1;
				OutInteraction.SelectedMarker = -1;
			}
		}

		if (m_bResizingMarker != FALSE)
		{
			OutInteraction.bMarkerResizeActive = TRUE;
			OutInteraction.SelectedTrack = m_DragTrack;
			OutInteraction.SelectedMarker = m_DragMarker;
			if (bActive != FALSE)
			{
				const FTimelineMarker& Marker = InTracks[m_DragTrack].Markers[m_DragMarker];
				if (m_bResizeLeftEdge != FALSE)
				{
					const FLOAT OriginalEnd = Marker.Time + Marker.Duration;
					const FLOAT NewStart = EMath::Min(XToTime(MousePosition.x)
						, EMath::Max(OriginalEnd - 0.01f, 0.0f));
					m_ResizeTime = EMath::Max(NewStart, 0.0f);
					m_ResizeDuration = EMath::Max(OriginalEnd - m_ResizeTime, 0.01f);
				}
				else
				{
					m_ResizeTime = Marker.Time;
					m_ResizeDuration = EMath::Max(0.01f, XToTime(MousePosition.x) - Marker.Time);
				}
				OutInteraction.MarkerResizeTime = m_ResizeTime;
				OutInteraction.MarkerResizeDuration = m_ResizeDuration;
				OutInteraction.bMarkerResizeLeftEdge = m_bResizeLeftEdge;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				const FTimelineMarker& Marker = InTracks[m_DragTrack].Markers[m_DragMarker];
				// Only commit a real resize. Clicking a handle without moving should just
				// select the marker and must not add an identical edit to Undo.
				if ((EMath::Abs(m_ResizeTime - Marker.Time) > 0.0001f)
					|| (EMath::Abs(m_ResizeDuration - Marker.Duration) > 0.0001f))
				{
					OutInteraction.MarkerResizeTime = m_ResizeTime;
					OutInteraction.MarkerResizeDuration = m_ResizeDuration;
					OutInteraction.bMarkerResizeLeftEdge = m_bResizeLeftEdge;
					OutInteraction.bMarkerResizeFinished = TRUE;
				}
				m_bResizingMarker = FALSE;
			}
		}
		else if (m_bMultiDragging != FALSE)
		{
			OutInteraction.bMarkerMultiDragActive = TRUE;
			OutInteraction.SelectedTrack = m_DragTrack;
			OutInteraction.SelectedMarker = m_DragMarker;
			if (bActive != FALSE)
			{
				const FLOAT RequestedDelta = XToTime(MousePosition.x) - m_MultiDragStartTime;
				FLOAT MinDelta = -FLT_MAX;
				FLOAT MaxDelta = FLT_MAX;
				for (INT32 RefIndex = 0; RefIndex < m_MultiDragRefs.Num(); RefIndex++)
				{
					const FLOAT BaseTime = m_MultiDragBaseTimes[RefIndex];
					const FLOAT Duration = m_MultiDragDurations[RefIndex];
					MinDelta = EMath::Max(MinDelta, -BaseTime);
					MaxDelta = EMath::Min(MaxDelta, InDuration - (BaseTime + Duration));
				}
				if (MaxDelta < MinDelta)
				{
					MaxDelta = MinDelta;
				}
				m_MultiDragDelta = EMath::Clamp(RequestedDelta, MinDelta, MaxDelta);
				OutInteraction.MarkerMultiDragDelta = m_MultiDragDelta;
				OutInteraction.MultiDraggedMarkers = m_MultiDragRefs;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				if ((EMath::Abs(m_MultiDragDelta) > 0.0001f) && (m_MultiDragRefs.Num() > 0))
				{
					OutInteraction.MarkerMultiDragDelta = m_MultiDragDelta;
					OutInteraction.MultiDraggedMarkers = m_MultiDragRefs;
					OutInteraction.bMarkerMultiDragFinished = TRUE;
				}
				m_bMultiDragging = FALSE;
				m_MultiDragDelta = 0.0f;
				m_MultiDragRefs.Empty();
				m_MultiDragBaseTimes.Empty();
				m_MultiDragDurations.Empty();
			}
		}
		else if (m_bDraggingMarker != FALSE)
		{
			OutInteraction.bMarkerDragActive = TRUE;
			OutInteraction.SelectedTrack = m_DragTrack;
			OutInteraction.SelectedMarker = m_DragMarker;
			if (bActive != FALSE)
			{
				// Move the marker by the cursor delta, preserving the grab offset inside it.
				// Range markers are also clamped so their end cannot leave the clip.
				const FTimelineMarker& Marker = InTracks[m_DragTrack].Markers[m_DragMarker];
				const FLOAT RangeDuration = (Marker.bRange != FALSE)
					? EMath::Max(Marker.Duration, 0.0f) : 0.0f;
				const FLOAT MaxStartTime = EMath::Max(0.0f, InDuration - RangeDuration);
				m_DragTime = EMath::Clamp(XToTime(MousePosition.x) - m_DragOffsetTime
					, 0.0f, MaxStartTime);
				OutInteraction.MarkerDragTime = m_DragTime;
			}
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				const FTimelineMarker& Marker = InTracks[m_DragTrack].Markers[m_DragMarker];
				// A plain click only selects; do not push a no-op retime into Undo.
				if (EMath::Abs(m_DragTime - Marker.Time) > 0.0001f)
				{
					OutInteraction.MarkerDragTime = m_DragTime;
					OutInteraction.bMarkerDragFinished = TRUE;
				}
				m_bDraggingMarker = FALSE;
			}
		}
		else if ((m_bScrubbing != FALSE) && (bActive != FALSE))
		{
			OutInteraction.bScrubbed = TRUE;
			OutInteraction.ScrubbedTime = XToTime(MousePosition.x);
			if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
			{
				m_bScrubbing = FALSE;
			}
		}
		else if (ImGui::IsMouseReleased(ImGuiMouseButton_Left))
		{
			m_bScrubbing = FALSE;
		}

		OutInteraction.SelectedMarkers = m_SelectedMarkers;
	}

}

#endif
