#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Text/Path.h>
#include <Base/DataStructure/Text/String.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class IImporter
	{
	public:
		virtual ~IImporter() {};
		virtual void CreateImportEditor(TArray<EString> Path) = 0;
		virtual EString GetFileFilterList() { return ""; }
		void UpdateEditor();
	protected:
		virtual void UpdateImportEditor() = 0;
		// Called by an importer after it has written one or more assets. The request is
		// coalesced by EImportManager and consumed by the content browser on its next frame.
		void NotifyImportFinished();
		BOOL8 NeedUpdate = FALSE;
	};
};
#endif
