#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

	class EAnimMontageAsset;

	/*
	* First-pass montage editor: slot groups, sections, chaining and notifies are authored as
	* JSON, validated on Apply, and saved back into the same PAsset. The read-only summary
	* makes the current chain and event counts visible without parsing the JSON by eye. A
	* timeline view can replace this window later without changing the asset or runtime.
	*/
	class EAnimMontageEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenMontageAsset(const EString& InAssetFullPath);

	private:
		void LoadJsonBufferFromAsset();

	private:
		BOOL8						m_ShowEditor = FALSE;
		const EAnimMontageAsset*	m_Asset = nullptr;
		EString						m_OpenedAssetFullPath;
		EString						m_LastError;
		TArray<CHAR>				m_JsonBuffer;
	};

}

#endif
