#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class EAnimMontageImporter : public IImporter
	{
	public:
		virtual ~EAnimMontageImporter() {};
		virtual void CreateImportEditor(TArray<EString> Paths) override;
		virtual EString GetFileFilterList() { return "json"; }
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> m_Paths;
		BOOL8 InitializeEditor = FALSE;
	};
};
#endif
