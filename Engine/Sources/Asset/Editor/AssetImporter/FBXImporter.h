#pragma once
#include "IImporter.h"
#include <imgui.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class FBXImporter : public IImporter
	{
	public:
		virtual ~FBXImporter() {};
		virtual void CreateImportEditor(EString Path) override;
	};
};
#endif