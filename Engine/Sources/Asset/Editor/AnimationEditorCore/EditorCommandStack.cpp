#include "EditorCommandStack.h"

#if _EDITOR_ONLY

namespace PigeonEngine
{

	BOOL32 FEditorCommandStack::SnapshotsEqual(const FEditorSnapshot& InA, const FEditorSnapshot& InB)
	{
		if (InA.Json != InB.Json)
		{
			return FALSE;
		}
		if (InA.NodePositions.Num() != InB.NodePositions.Num())
		{
			return FALSE;
		}
		for (TMap<EString, ImVec2>::TConstIterator It = InA.NodePositions.begin()
			; It != InA.NodePositions.end(); ++It)
		{
			const ImVec2* OtherPosition = InB.NodePositions.FindValueAsPtr(It->first);
			if (!OtherPosition)
			{
				return FALSE;
			}
			if ((OtherPosition->x != It->second.x) || (OtherPosition->y != It->second.y))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	void FEditorCommandStack::Clear()
	{
		UndoStack.Empty();
		RedoStack.Empty();
		PendingSnapshot = FEditorSnapshot();
		bTransactionOpen = FALSE;
	}

	void FEditorCommandStack::BeginTransaction(const FEditorSnapshot& InCurrent)
	{
		if (bTransactionOpen != FALSE)
		{
			return;
		}
		PendingSnapshot = InCurrent;
		bTransactionOpen = TRUE;
	}

	void FEditorCommandStack::CommitTransaction(const FEditorSnapshot& InCurrent)
	{
		if (bTransactionOpen == FALSE)
		{
			return;
		}
		bTransactionOpen = FALSE;
		if (SnapshotsEqual(PendingSnapshot, InCurrent) != FALSE)
		{
			return;
		}
		UndoStack.Add(PendingSnapshot);
		if (UndoStack.Num() > MaxHistory)
		{
			UndoStack.RemoveAt(0);
		}
		RedoStack.Empty();
	}

	void FEditorCommandStack::CancelTransaction()
	{
		bTransactionOpen = FALSE;
		PendingSnapshot = FEditorSnapshot();
	}

	BOOL32 FEditorCommandStack::IsTransactionOpen()const
	{
		return bTransactionOpen;
	}

	BOOL32 FEditorCommandStack::CanUndo()const
	{
		return (((bTransactionOpen == FALSE) && (UndoStack.Num() > 0)) ? TRUE : FALSE);
	}

	BOOL32 FEditorCommandStack::CanRedo()const
	{
		return (((bTransactionOpen == FALSE) && (RedoStack.Num() > 0)) ? TRUE : FALSE);
	}

	BOOL32 FEditorCommandStack::Undo(const FEditorSnapshot& InCurrent, FEditorSnapshot& OutSnapshot)
	{
		if (CanUndo() == FALSE)
		{
			return FALSE;
		}
		RedoStack.Add(InCurrent);
		OutSnapshot = UndoStack[UndoStack.Num() - 1];
		UndoStack.RemoveAt(UndoStack.Num() - 1);
		return TRUE;
	}

	BOOL32 FEditorCommandStack::Redo(const FEditorSnapshot& InCurrent, FEditorSnapshot& OutSnapshot)
	{
		if (CanRedo() == FALSE)
		{
			return FALSE;
		}
		UndoStack.Add(InCurrent);
		if (UndoStack.Num() > MaxHistory)
		{
			UndoStack.RemoveAt(0);
		}
		OutSnapshot = RedoStack[RedoStack.Num() - 1];
		RedoStack.RemoveAt(RedoStack.Num() - 1);
		return TRUE;
	}

}

#endif
