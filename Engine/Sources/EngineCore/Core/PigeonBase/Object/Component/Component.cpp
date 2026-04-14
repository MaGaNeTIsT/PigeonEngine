#include "./Component.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PComponent, PObject>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PComponent::PComponent()
		: PObject()
	{
		POBJ_DEBUGNAME_SET(this, "Component");
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
		SetInitialized(TRUE);
	}

	void PComponent::Uninit()
	{
		PObject::Uninit();
		
	}

	void PComponent::Tick(FLOAT deltaTime)
	{
		(void)deltaTime;
	}
	void PComponent::FixedTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
	}
#if _EDITOR_ONLY
	void PComponent::EditorTick(FLOAT deltaTime)
	{
		if(!IsTickable())
		{
			return;
		}
	}
#endif

};