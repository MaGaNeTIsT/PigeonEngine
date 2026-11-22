#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <Development/EditorLogManager.h>

namespace PigeonEngine
{
	/*
	* The editor Output Log window. Reads through EEditorLogManager's snapshot API and
	* owns only view state (filters, search, pause, selected row); the log service keeps
	* storage, file writing and pruning, and exists in non-editor builds as well.
	*/
	class FOutputLogPanel final
	{
	public:
		FOutputLogPanel();
		~FOutputLogPanel() = default;

		void Draw();

	private:
		struct FLogRow
		{
			// Index into Snapshot. Consecutive identical lines collapse into one row.
			INT32	FirstIndex = 0;
			INT32	Count = 1;
		};

		void RefreshSnapshot();
		void RebuildCategories();
		void RebuildRows();
		void DrawToolbar();
		void DrawRows();
		void DrawRow(const INT32 InRowIndex);
		void ClearLogView();
		void CopyRow(const INT32 InRowIndex) const;
		void CopyAllFiltered() const;

		PE_NODISCARD BOOL32 MatchesFilters(const ELog& InLog, const EString& InSearchLower) const;
		PE_NODISCARD BOOL32 MatchesCategory(const ELog& InLog) const;

	private:
		TArray<TSharedPtr<ELog>>	Snapshot;
		TArray<FLogRow>				Rows;
		TArray<EString>				Categories;
		CHAR						SearchBuffer[256] = { 0 };
		// 0 is "All Categories"; the rest index Categories.
		INT32						CategoryIndex = 0;
		BOOL32						bShowLog = TRUE;
		BOOL32						bShowWarning = TRUE;
		BOOL32						bShowError = TRUE;
		BOOL32						bPaused = FALSE;
		BOOL32						bFollowTail = TRUE;
		BOOL32						bCollapseRepeats = TRUE;
		BOOL32						bFilterDirty = TRUE;
		BOOL32						bScrollToBottom = TRUE;
		UINT32						LastRevision = 0u;
		INT32						SelectedRow = -1;
		FLOAT						MaxLineWidth = 0.0f;
	};
}

#endif
