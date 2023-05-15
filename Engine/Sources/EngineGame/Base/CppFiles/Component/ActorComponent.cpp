#include "../../Headers/Component/ActorComponent.h"

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
	BOOL PActorComponent::ContainTransform()const
	{
		return FALSE;
	}

};