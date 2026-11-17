#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <imgui.h>

namespace PigeonEngine
{

	struct FTimelineMarker
	{
	public:
		EString		Label;
		FLOAT		Time = 0.0f;
		FLOAT		Duration = 0.0f;
		BOOL8		bRange = FALSE;
		BOOL8		bDraggable = FALSE;
		// Range markers can drag their right edge to change duration.
		BOOL8		bResizable = FALSE;
		ImU32		Color = IM_COL32(235, 150, 60, 255);
		INT32		UserIndex = -1;
	};

	struct FTimelineTrack
	{
	public:
		EString					Name;
		TArray<FTimelineMarker>	Markers;
	};

	struct FTimelineMarkerRef
	{
	public:
		INT32 TrackIndex = -1;
		INT32 MarkerIndex = -1;
	public:
		PE_NODISCARD BOOL32 IsValid()const
		{
			return (((TrackIndex >= 0) && (MarkerIndex >= 0)) ? TRUE : FALSE);
		}
		bool operator==(const FTimelineMarkerRef& InOther)const
		{
			return ((TrackIndex == InOther.TrackIndex) && (MarkerIndex == InOther.MarkerIndex));
		}
	};

	struct FTimelineInteraction
	{
	public:
		BOOL32	bScrubbed = FALSE;
		FLOAT	ScrubbedTime = 0.0f;
		BOOL32	bTrackSelected = FALSE;
		INT32	SelectedTrack = -1;
		BOOL32	bMarkerSelected = FALSE;
		INT32	SelectedMarker = -1;
		BOOL32	bMarkerSelectionChanged = FALSE;
		TArray<FTimelineMarkerRef> SelectedMarkers;
		BOOL32	bMarkerDragActive = FALSE;
		BOOL32	bMarkerDragFinished = FALSE;
		FLOAT	MarkerDragTime = 0.0f;
		// Multi-marker drag: all selected markers move by the same clamped delta.
		BOOL32	bMarkerMultiDragActive = FALSE;
		BOOL32	bMarkerMultiDragFinished = FALSE;
		FLOAT	MarkerMultiDragDelta = 0.0f;
		TArray<FTimelineMarkerRef> MultiDraggedMarkers;
		BOOL32	bMarkerResizeActive = FALSE;
		BOOL32	bMarkerResizeFinished = FALSE;
		BOOL32	bMarkerResizeLeftEdge = FALSE;
		FLOAT	MarkerResizeTime = 0.0f;
		FLOAT	MarkerResizeDuration = 0.0f;
	};

	/*
	* Shared multi-track timeline: ruler, scrub, marker selection and horizontal marker
	* dragging. Clip notifies and Montage section/notify rows both use this widget.
	*/
	class FTimelineWidget
	{
	public:
		void Draw(const CHAR* InId, const TArray<FTimelineTrack>& InTracks, FLOAT InDuration
			, FLOAT InCurrentTime, INT32 InSelectedTrack, INT32 InSelectedMarker
			, FTimelineInteraction& OutInteraction);
	private:
		PE_NODISCARD BOOL32 IsMarkerSelected(INT32 InTrackIndex, INT32 InMarkerIndex)const;
		void SetSingleSelection(INT32 InTrackIndex, INT32 InMarkerIndex);
		void ToggleSelection(INT32 InTrackIndex, INT32 InMarkerIndex);
	private:
		BOOL32	m_bDraggingMarker = FALSE;
		BOOL32	m_bResizingMarker = FALSE;
		BOOL32	m_bResizeLeftEdge = FALSE;
		BOOL32	m_bScrubbing = FALSE;
		BOOL32	m_bBoxSelecting = FALSE;
		BOOL32	m_bMultiDragging = FALSE;
		INT32	m_DragTrack = -1;
		INT32	m_DragMarker = -1;
		INT32	m_LastInputTrack = -1;
		INT32	m_LastInputMarker = -1;
		FLOAT	m_DragTime = 0.0f;
		FLOAT	m_MultiDragStartTime = 0.0f;
		FLOAT	m_MultiDragDelta = 0.0f;
		ImVec2	m_BoxStart = ImVec2(0.0f, 0.0f);
		TArray<FTimelineMarkerRef>	m_SelectedMarkers;
		TArray<FTimelineMarkerRef>	m_MultiDragRefs;
		TArray<FLOAT>				m_MultiDragBaseTimes;
		TArray<FLOAT>				m_MultiDragDurations;
		// Visible time window. Ctrl+wheel zooms around the cursor, Shift+wheel and
		// middle-drag pan, middle-double-click fits the whole clip.
		FLOAT	m_ViewStart = 0.0f;
		FLOAT	m_ViewDuration = 0.0f;
		FLOAT	m_LastDuration = -1.0f;
		BOOL32	m_bPanning = FALSE;
		// Cursor time minus marker start at mouse-down. Dragging applies the cursor delta
		// through this offset so clicking the middle of a range marker does not snap its
		// start to the click position.
		FLOAT	m_DragOffsetTime = 0.0f;
		FLOAT	m_ResizeTime = 0.0f;
		FLOAT	m_ResizeDuration = 0.0f;
	};

}

#endif
