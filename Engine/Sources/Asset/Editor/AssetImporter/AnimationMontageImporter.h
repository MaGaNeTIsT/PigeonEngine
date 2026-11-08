#pragma once
#include "IImporter.h"
#include <imgui.h>
#if ENGINE_EDITOR_ONLY
namespace PigeonEngine
{
	class EAnimMontageImporter : public IImporter
	{
	public:
		virtual ~EAnimMontageImporter() {};
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
