#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include "../EditorUi.h"
#include "NotifyDocument.h"

namespace PigeonEngine
{

	class FNotifyEditorPanel
	{
	public:
		void Draw(FNotifyDocument& InDocument);

	private:
		void DrawToolbar(FNotifyDocument& InDocument);
		void DrawForm(FNotifyDocument& InDocument, FLOAT InHeight);
		void DrawBottomPanel(FNotifyDocument& InDocument, FLOAT InTotalHeight);
		void DrawDiagnostics(FNotifyDocument& InDocument);
		void DrawSource(FNotifyDocument& InDocument);

		void RefreshBuffers(FNotifyDocument& InDocument);
		void RunMutation(FNotifyDocument& InDocument
			, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage);
		BOOL32 CommitDefinition(FNotifyDocument& InDocument, const EString& InClassName
			, FLOAT InDefaultDuration, EString& OutError);
		void RestoreSnapshot(FNotifyDocument& InDocument, const FEditorSnapshot& InSnapshot);

	private:
		FGrowableTextBuffer	m_NameBuffer;
		FGrowableTextBuffer	m_DescriptionBuffer;
		FGrowableTextBuffer	m_PayloadBuffer;
		EString				m_BufferSource;
		FGrowableTextBuffer	m_SourceBuffer;
		const FNotifyDocument*	m_SourceDocument = nullptr;
		BOOL8				m_bShowOutput = TRUE;
		FLOAT				m_BottomHeight = 110.0f;
		EString				m_StatusMessage;
		BOOL32				m_bStatusSucceeded = TRUE;
	};

}

#endif
