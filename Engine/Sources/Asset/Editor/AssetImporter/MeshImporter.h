#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class EMeshImporter : public IImporter
	{
	public:
		virtual ~EMeshImporter() {};
		virtual void CreateImportEditor(TArray<EString> Paths) override;
		virtual EString GetFileFilterList() { return "FBX"; }
	protected:
		virtual void UpdateImportEditor() override;
	};
};
#endif