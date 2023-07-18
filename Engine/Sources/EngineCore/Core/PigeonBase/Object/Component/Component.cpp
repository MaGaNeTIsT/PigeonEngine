#include "./Component.h"

namespace PigeonEngine
{

	PComponent::PComponent()
	{
		
	}
	PComponent::~PComponent()
	{
	}

	void PComponent::Destroy()
	{
		PObject::Destroy();
	}


	void PComponent::Init()
	{
	}

	void PComponent::Uninit()
	{
		PObject::Uninit();
		
	}

	void PComponent::Tick(FLOAT deltaTime)
	{
	}
	void PComponent::FixedTick(FLOAT deltaTime)
	{
	}
#ifdef _DEVELOPMENT_EDITOR
	void PComponent::EditorTick(FLOAT deltaTime)
	{
	}
#endif

};