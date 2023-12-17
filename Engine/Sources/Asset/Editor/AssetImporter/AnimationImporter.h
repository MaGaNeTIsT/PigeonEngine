#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class EAnimationImporter : public IImporter
	{
	public:
		virtual ~EAnimationImporter() {};
		virtual void CreateImportEditor(TArray<EString> Paths) override;
		virtual EString GetFileFilterList() { return "FBX;OBJ;PNG"; }
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> m_Paths;
		BOOL8 InitializeEditor = FALSE;
	};
};
#endif