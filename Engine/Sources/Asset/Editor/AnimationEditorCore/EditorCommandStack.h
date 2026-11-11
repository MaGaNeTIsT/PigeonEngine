#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Container/Map.h>
#include <imgui.h>
#include "EditorSelection.h"

namespace PigeonEngine
{

	/*
	* One complete editor state. Animation assets are small, so snapshot-based undo is both
	* simpler and safer than trying to invert every structural JSON edit.
	*/
	struct FEditorSnapshot
	{
	public:
		EString					Json;
		TMap<EString, ImVec2>	NodePositions;
		FEditorSelection		Selection;
	};

	/*
	* Document-level transaction stack. BeginTransaction is called before a drag starts or a
	* button mutates the document; CommitTransaction only pushes a snapshot when something
	* actually changed. Failed validations call CancelTransaction.
	*/
	class FEditorCommandStack
	{
	public:
		void Clear();
		void BeginTransaction(const FEditorSnapshot& InCurrent);
		void CommitTransaction(const FEditorSnapshot& InCurrent);
		void CancelTransaction();
		PE_NODISCARD BOOL32 IsTransactionOpen()const;
		PE_NODISCARD BOOL32 CanUndo()const;
		PE_NODISCARD BOOL32 CanRedo()const;
		BOOL32 Undo(const FEditorSnapshot& InCurrent, FEditorSnapshot& OutSnapshot);
		BOOL32 Redo(const FEditorSnapshot& InCurrent, FEditorSnapshot& OutSnapshot);
	private:
		static BOOL32 SnapshotsEqual(const FEditorSnapshot& InA, const FEditorSnapshot& InB);
	private:
		static constexpr INT32 MaxHistory = 100;
		TArray<FEditorSnapshot> UndoStack;
		TArray<FEditorSnapshot> RedoStack;
		FEditorSnapshot PendingSnapshot;
		BOOL32 bTransactionOpen = FALSE;
	};

}

#endif
