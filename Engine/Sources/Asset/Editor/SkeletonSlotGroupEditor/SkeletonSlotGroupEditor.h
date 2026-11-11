#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Text/String.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include "../EditorDocument.h"

namespace PigeonEngine
{

	class ESkeletonAsset;

	/*
	* Edits the ordered slot groups stored on a skeleton. The order is the montage blend
	* order: group 0 is the base layer and each later group is blended on top.
	*/
	class ESkeletonSlotGroupEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenSkeletonAsset(const EString& InAssetFullPath);

	private:
		void OpenSkeletonAssetNow(const EString& InAssetFullPath);
		BOOL32 SaveSkeletonAsset(EString& OutError);
		BOOL32 ReloadSkeletonAssetNow(EString& OutError);
		void CaptureBaseline();
		void DiscardChanges();
		void HandleDocumentAction();
		BOOL32 ValidateSlotGroups(EString& OutError)const;

	private:
		BOOL8						m_ShowEditor = FALSE;
		const ESkeletonAsset*		m_Asset = nullptr;
		EString						m_OpenedAssetFullPath;
		EString						m_LastError;
		EString						m_StatusMessage;
		BOOL8						m_bStatusSucceeded = TRUE;
		FEditorDocumentState		m_Document;
		// Slot groups as they were when the asset was opened or last saved.
		TArray<ESkeletonSlotGroup>	m_BaselineSlotGroups;
	};

}

#endif
