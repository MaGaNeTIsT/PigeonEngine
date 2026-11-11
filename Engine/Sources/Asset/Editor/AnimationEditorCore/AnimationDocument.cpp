#include "AnimationDocument.h"

#if _EDITOR_ONLY

namespace PigeonEngine
{

	BOOL32 FAnimationDocument::IsDirty()const
	{
		return ((DraftJson != BaselineJson) ? TRUE : FALSE);
	}

	BOOL32 FAnimationDocument::IsApplied()const
	{
		return ((DraftJson == AppliedJson) ? TRUE : FALSE);
	}

	FEditorSnapshot FAnimationDocument::CaptureSnapshot()const
	{
		FEditorSnapshot Snapshot;
		Snapshot.Json = DraftJson;
		Snapshot.NodePositions = NodePositions;
		Snapshot.Selection = Selection;
		return Snapshot;
	}

	void FAnimationDocument::RestoreSnapshot(const FEditorSnapshot& InSnapshot)
	{
		DraftJson = InSnapshot.Json;
		NodePositions = InSnapshot.NodePositions;
		Selection = InSnapshot.Selection;
		RefreshDerived();
	}

}

#endif
