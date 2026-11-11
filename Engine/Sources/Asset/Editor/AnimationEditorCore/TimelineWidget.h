#pragma once

#if _EDITOR_ONLY

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

	struct FTimelineInteraction
	{
	public:
		BOOL32	bScrubbed = FALSE;
		FLOAT	ScrubbedTime = 0.0f;
		BOOL32	bTrackSelected = FALSE;
		INT32	SelectedTrack = -1;
		BOOL32	bMarkerSelected = FALSE;
		INT32	SelectedMarker = -1;
		BOOL32	bMarkerDragActive = FALSE;
		BOOL32	bMarkerDragFinished = FALSE;
		FLOAT	MarkerDragTime = 0.0f;
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
		BOOL32	m_bDraggingMarker = FALSE;
		BOOL32	m_bResizingMarker = FALSE;
		BOOL32	m_bResizeLeftEdge = FALSE;
		BOOL32	m_bScrubbing = FALSE;
		INT32	m_DragTrack = -1;
		INT32	m_DragMarker = -1;
		FLOAT	m_DragTime = 0.0f;
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
