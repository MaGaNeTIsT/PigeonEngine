#include "../../Headers/Component/ActorComponent.h"
#include "../../Headers/Actor.h"

namespace PigeonEngine
{

	PActorComponent::PActorComponent() : ParentActorComponent(nullptr)
	{
	}
	PActorComponent::~PActorComponent()
	{
	}

	PActor* PActorComponent::GetOwnerActor() const
	{
		return this->Owner;
	}

	void PActorComponent::SetOwnerActor(PActor* NewOwner)
	{
		if(!NewOwner)
		{
			// Need an alert here!
			return;
		}
		this->Owner = NewOwner;
		NewOwner->AddComponent(this);
	}

	BOOL PActorComponent::ContainTransform()const
	{
		return FALSE;
	}
	const class ETransform* PActorComponent::GetTransform()const
	{
		return nullptr;
	}
	const PActorComponent* PActorComponent::GetParent()const
	{
		return ParentActorComponent;
	}

};