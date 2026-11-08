#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <Base/DataStructure/Text/String.h>

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
		void LoadJsonBufferFromAsset();

	private:
		BOOL8					bShowEditor = FALSE;
		const EAnimNotifyAsset*	Asset = nullptr;
		EString					OpenedAssetFullPath;
		EString					LastError;
		TArray<CHAR>			JsonBuffer;
	};

}

#endif
