#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include "../EditorUi.h"
#include "StateMachineDocument.h"
#include "StateMachinePreviewPanel.h"

namespace PigeonEngine
{

	/*
	* State machine document view used by the animation workspace. The panel never owns the
	* asset; it edits the document draft through FStateMachineDocument and draws the same
	* selection the inspector and graph share.
	*/
	class FStateMachineEditorPanel
	{
	public:
		void Draw(FStateMachineDocument& InDocument);
	private:
		void DrawToolbar(FStateMachineDocument& InDocument);
		void DrawLayout(FStateMachineDocument& InDocument);
		void DrawLeftPanel(FStateMachineDocument& InDocument);
		void DrawGraph(FStateMachineDocument& InDocument);
		void DrawInspector(FStateMachineDocument& InDocument);
		void DrawStateInspector(FStateMachineDocument& InDocument, const EString& InStateName);
		void DrawParameterInspector(FStateMachineDocument& InDocument, const EString& InParameterName);
		void DrawTransitionInspector(FStateMachineDocument& InDocument, INT32 InTransitionIndex);
		void DrawConditions(FStateMachineDocument& InDocument, INT32 InTransitionIndex);
		void DrawBottomPanel(FStateMachineDocument& InDocument);
		void DrawDiagnostics(FStateMachineDocument& InDocument);
		void DrawSource(FStateMachineDocument& InDocument);
		void DrawParameterTable(FStateMachineDocument& InDocument);
		void DrawStateList(FStateMachineDocument& InDocument);

		void RunMutation(FStateMachineDocument& InDocument
			, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage);
		void ResetEditorBuffers();
		PE_NODISCARD INT32 GetParameterUsageCount(const FStateMachineDocument& InDocument
			, const EString& InParameterName)const;

	private:
		FStateMachinePreviewPanel	m_PreviewPanel;
		FGrowableTextBuffer			m_StateNameBuffer;
		EString						m_StateNameBufferSource;
		TArray<FGrowableTextBuffer>	m_ParameterNameBuffers;
		INT32						m_ParameterNameBufferCount = -1;
		FGrowableTextBuffer			m_SourceBuffer;
		const FStateMachineDocument* m_SourceDocument = nullptr;
		EString						m_StatusMessage;
		BOOL32						m_bStatusSucceeded = TRUE;
	};

}

#endif
