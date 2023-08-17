

#include "Scene.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PScene, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PScene::PScene()
	{
	}
	PScene::~PScene()
	{
	}

};