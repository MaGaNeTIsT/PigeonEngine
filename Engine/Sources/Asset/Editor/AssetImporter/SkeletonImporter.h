#pragma once
#include "IImporter.h"
#include <imgui.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class ESkeletonImporter : public IImporter
	{
	public:
		virtual ~ESkeletonImporter()
		{

		}
		virtual void CreateImportEditor(TArray<EString> InPaths) override;
		virtual EString GetFileFilterList()
		{
			return PE_TEXT("FBX;OBJ;PNG");
		}
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> Paths;
		BOOL8 bInitializeEditor = FALSE;
	};
}   // namespace PigeonEngine
#endif
