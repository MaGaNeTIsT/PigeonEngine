#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>
#include "../EditorDocument.h"

namespace PigeonEngine
{

	class EAnimNotifyAsset;

	/*
	* Standalone notify asset editor. The JSON is the source of truth; Apply validates it
	* into a temporary definition and only then replaces the live one, and Save writes the
	* validated text back into the PAsset.
	*/
	class EAnimNotifyEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenNotifyAsset(const EString& InAssetFullPath);

	private:
		void OpenNotifyAssetNow(const EString& InAssetFullPath);
		BOOL32 SaveNotifyAsset(EString& OutError);
		BOOL32 ReloadNotifyAssetNow(EString& OutError);
		void CaptureBaseline();
		BOOL32 DiscardChanges();
		void HandleDocumentAction();
		void LoadJsonBufferFromAsset();

	private:
		BOOL8					m_ShowEditor = FALSE;
		const EAnimNotifyAsset*	m_Asset = nullptr;
		EString					m_OpenedAssetFullPath;
		EString					m_LastError;
		EString					m_StatusMessage;
		BOOL8					m_bStatusSucceeded = TRUE;
		FEditorDocumentState	m_Document;
		// Last applied/saved JSON. Discard rebuilds the live definition from this text.
		EString					m_BaselineJson;
		TArray<CHAR>			m_JsonBuffer;
	};

}

#endif
