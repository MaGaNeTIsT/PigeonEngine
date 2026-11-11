#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include "../EditorUi.h"
#include "SlotGroupDocument.h"

namespace PigeonEngine
{

	class FSlotGroupEditorPanel
	{
	public:
		void Draw(FSlotGroupDocument& InDocument);

	private:
		void DrawToolbar(FSlotGroupDocument& InDocument);
		void DrawEditor(FSlotGroupDocument& InDocument, FLOAT InHeight);
		void DrawBottomPanel(FSlotGroupDocument& InDocument, FLOAT InTotalHeight);
		void DrawDiagnostics(FSlotGroupDocument& InDocument);
		void DrawSource(FSlotGroupDocument& InDocument);

		void EnsureSelection(FSlotGroupDocument& InDocument);
		void RefreshBuffers(FSlotGroupDocument& InDocument);
		void InvalidateBuffers();
		void RunMutation(FSlotGroupDocument& InDocument
			, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage);
		void RestoreSnapshot(FSlotGroupDocument& InDocument, const FEditorSnapshot& InSnapshot);

	private:
		TArray<FGrowableTextBuffer>			m_GroupNameBuffers;
		TArray<TArray<FGrowableTextBuffer>>	m_SlotNameBuffers;
		EString				m_BufferSource;
		BOOL8				m_bBuffersDirty = TRUE;
		INT32				m_SelectedGroupIndex = 0;
		INT32				m_SelectedSlotIndex = -1;
		FGrowableTextBuffer	m_SourceBuffer;
		const FSlotGroupDocument*	m_SourceDocument = nullptr;
		BOOL8				m_bShowOutput = TRUE;
		FLOAT				m_BottomHeight = 110.0f;
		EString				m_StatusMessage;
		BOOL32				m_bStatusSucceeded = TRUE;
	};

}

#endif
