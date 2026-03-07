#include "Level.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PLevel, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

};