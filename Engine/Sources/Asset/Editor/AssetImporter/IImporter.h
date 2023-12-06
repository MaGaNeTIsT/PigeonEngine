#pragma once
#include <Base/DataStructure/Text/Path.h>
#include <Base/DataStructure/Text/String.h>
#if _EDITOR_ONLY
namespace PigeonEngine
{
	class IImporter
	{
	public:
		virtual ~IImporter() {};
		virtual void CreateImportEditor(EString Path) = 0;
	};
};
#endif