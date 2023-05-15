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
	PActorComponent::PActorComponent(const PActorComponent& other) : PComponent(other)
	{
		ParentActorComponent = other.ParentActorComponent;
		ChildrenActorComponent = other.ChildrenActorComponent;
	}
	PActorComponent& PActorComponent::operator=(const PActorComponent& other)
	{
		m_Active = other.m_Active;
#ifdef _DEVELOPMENT_EDITOR
		m_Name = other.m_Name;
#endif
		ParentActorComponent = other.ParentActorComponent;
		ChildrenActorComponent = other.ChildrenActorComponent;
		return (*this);
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

};