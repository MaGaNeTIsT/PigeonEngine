#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class EFBXImporter : public IImporter
	{
	public:
		virtual ~EFBXImporter() {};
		virtual void CreateImportEditor(TArray<EString> Paths) override;
	protected:
		virtual void UpdateImportEditor() override;
	};
};
#endif