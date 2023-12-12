#pragma once
#include <CoreMinimal.h>
#include <Base/DataStructure/Text/Path.h>
#include <Base/DataStructure/Text/String.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class IImporter
	{
	public:
		virtual ~IImporter() {};
		virtual void CreateImportEditor(TArray<EString> Path) = 0;
		void UpdateEditor();
	protected:
		virtual void UpdateImportEditor() = 0;
		BOOL8 NeedUpdate = FALSE;
	};
};
#endif