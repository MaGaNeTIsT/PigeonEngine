#include "./Component.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PComponent, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
#ifdef _EDITOR_ONLY.
		EditorTick(deltaTime);
		return;
#endif
		FixedTick(deltaTime);
	}
	void PComponent::FixedTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
	}
#ifdef _EDITOR_ONLY
	void PComponent::EditorTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
	}
#endif

};