#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include "AnimationDocumentManager.h"
#include "ClipEditorPanel.h"
#include "MontageEditorPanel.h"
#include "NotifyEditorPanel.h"
#include "SlotGroupEditorPanel.h"
#include "StateMachineEditorPanel.h"

namespace PigeonEngine
{

	/*
	* Dedicated animation authoring workspace. It owns the open documents and hosts the
	* document tabs; the per-kind panels own their own layout and inspector.
	*/
	class EAnimationWorkspace
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void ShutDown();
		void OpenStateMachineAsset(const EString& InAssetFullPath);
		void OpenAnimationAsset(const EString& InAssetFullPath);
		void OpenMontageAsset(const EString& InAssetFullPath);
		void OpenNotifyAsset(const EString& InAssetFullPath);
		void OpenSlotGroupAsset(const EString& InAssetFullPath);
		PE_NODISCARD BOOL32 IsOpen()const;
		PE_NODISCARD BOOL32 IsAssetOpen(const EString& InAssetFullPath)const;

	private:
		void DrawWorkspace();
		void DrawDocumentTabs();
		void DrawCloseDocumentPopup();
		void DrawEmptyState();

	private:
		FAnimationDocumentManager	m_Documents;
		FClipEditorPanel			m_ClipPanel;
		FMontageEditorPanel			m_MontagePanel;
		FNotifyEditorPanel			m_NotifyPanel;
		FSlotGroupEditorPanel		m_SlotGroupPanel;
		FStateMachineEditorPanel	m_StateMachinePanel;
		BOOL32						m_bOpen = FALSE;
		// Set only by the explicit File -> Reset Layout action. Normal startup must respect
		// the window position/size saved in ImGuiEngine.ini.
		BOOL32						m_bApplyWindowLayout = FALSE;
		EString						m_LastError;
	};

}

#endif
