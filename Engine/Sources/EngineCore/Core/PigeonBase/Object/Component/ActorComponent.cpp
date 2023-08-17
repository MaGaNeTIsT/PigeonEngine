#include "./ActorComponent.h"
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PActorComponent, PComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PActorComponent::PActorComponent() 
	{
	}
	PActorComponent::~PActorComponent()
	{
	}

	PActor* PActorComponent::GetOwnerActor() const
	{
		return this->Owner;
	}

	
	void PActorComponent::RemoveFromOwnerActor()
	{
		if(Owner)
		{
			Owner->DestoyComponent(this);
		}
		Owner = nullptr;
	}

	void PActorComponent::SetOwnerActor(PActor* NewOwner)
	{
		Check(ENGINE_COMPONENT_ERROR, "You are setting this component's owner actor to a nullptr. ", NewOwner == nullptr);
		this->Owner = NewOwner;
		NewOwner->AddComponent(this);
	}

	void PActorComponent::Destroy()
	{
		RemoveFromOwnerActor();
		PComponent::Destroy();
	}

	void PActorComponent::Init()
	{
		PComponent::Init();
	}

	void PActorComponent::Uninit()
	{
		PComponent::Uninit();
	}

	BOOL PActorComponent::ContainTransform()const
	{
		return FALSE;
	}


};