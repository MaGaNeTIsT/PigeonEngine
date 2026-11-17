#pragma once
#include "IImporter.h"
#include <imgui.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class EAnimNotifyImporter : public IImporter
	{
	public:
		virtual ~EAnimNotifyImporter() {};
		virtual void CreateImportEditor(TArray<EString> InPaths) override;
		virtual EString GetFileFilterList() { return "json"; }
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> Paths;
		BOOL8 bInitializeEditor = FALSE;
	};
};
#endif
