#include "RenderScene.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RScene, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

};