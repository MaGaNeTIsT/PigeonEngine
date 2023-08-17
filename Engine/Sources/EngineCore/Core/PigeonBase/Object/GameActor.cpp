

#include "GameActor.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PGameActor, PActor>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PGameActor::PGameActor()
	{
	}
	PGameActor::~PGameActor()
	{
	}

};