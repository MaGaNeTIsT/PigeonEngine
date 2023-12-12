#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class EPNGImporter : public IImporter
	{
	public:
		virtual ~EPNGImporter() {};
		virtual void CreateImportEditor(TArray<EString> Paths) override;
	protected:
		virtual void UpdateImportEditor() override;
	private:
		TArray<EString> m_Paths;
		const EString m_Extension = "PNG";
		BOOL8 CheckCopyFile = FALSE;
		BOOL8 InitializeEditor = FALSE;
	};
};
#endif