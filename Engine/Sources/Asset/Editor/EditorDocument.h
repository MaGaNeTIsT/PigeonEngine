#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>

namespace PigeonEngine
{

	enum class EEditorUnsavedChoiceType : UINT8
	{
		NONE = 0,
		SAVE,
		DISCARD,
		CANCEL
	};

	/*
	* Editing state shared by the animation-family editors.
	*
	* The editors still modify the live asset, so "Discard" restores the baseline each editor
	* captured when it opened or last saved; this struct only tracks whether there is
	* something to restore and which action is waiting for the user's answer.
	*/
	struct FEditorDocumentState
	{
	public:
		BOOL8	bDirty = FALSE;
		// This frame's pending action. Only one of them is set at a time.
		BOOL8	bClosePending = FALSE;
		BOOL8	bOpenPending = FALSE;
		BOOL8	bReloadPending = FALSE;
		EString	PendingOpenPath;

	public:
		PE_NODISCARD BOOL8 HasPendingAction()const
		{
			return (((bClosePending != FALSE) || (bOpenPending != FALSE) || (bReloadPending != FALSE)) ? TRUE : FALSE);
		}
		void ClearPendingAction()
		{
			bClosePending = FALSE;
			bOpenPending = FALSE;
			bReloadPending = FALSE;
			PendingOpenPath = EString();
		}
	};

	/*
	* Draws the Save / Discard / Cancel modal for the pending action. The caller performs the
	* selected operation and clears the pending action; this helper closes the popup itself.
	* InAssetName may be null.
	*/
	EEditorUnsavedChoiceType DrawEditorUnsavedChangesPopup(const CHAR* InPopupId, const CHAR* InAssetName
		, FEditorDocumentState& InOutState);

}

#endif
