#pragma once
#include "IImporter.h"
#include <imgui.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class EMeshImporter : public IImporter
	{
	public:
		virtual ~EMeshImporter()
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
		TArray<BOOL32> MeshCombineChecks;
		BOOL8 bInitializeEditor = FALSE;
	};
}   // namespace PigeonEngine
#endif
