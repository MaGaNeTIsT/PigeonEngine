#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

namespace PigeonEngine
{

	class EAnimStateMachineAsset;

	/*
	* First-pass state machine editor: the graph is authored as JSON, validated on Apply,
	* and saved back into the same PAsset. A structured graph view can replace this window
	* later without changing the asset or runtime.
	*/
	class EAnimStateMachineEditor
	{
	public:
		void EditorInit();
		void EditorUpdate();
		void OpenStateMachineAsset(const EString& InAssetFullPath);

	private:
		void LoadJsonBufferFromAsset();

	private:
		BOOL8						m_ShowEditor = FALSE;
		const EAnimStateMachineAsset*	m_Asset = nullptr;
		EString						m_OpenedAssetFullPath;
		EString						m_LastError;
		TArray<CHAR>				m_JsonBuffer;
	};

}

#endif
