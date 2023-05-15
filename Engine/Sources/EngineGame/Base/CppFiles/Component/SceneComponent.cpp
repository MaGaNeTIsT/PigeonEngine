#include "../../Headers/Component/SceneComponent.h"

namespace PigeonEngine
{

	PSceneComponent::PSceneComponent()
	{
	}
	PSceneComponent::~PSceneComponent()
	{
	}
	PSceneComponent::PSceneComponent(const PSceneComponent& other)
	{
	}
	PSceneComponent& PSceneComponent::operator=(const PSceneComponent& other)
	{
		//TODO
		return (*this);
	}
	BOOL PSceneComponent::ContainTransform()const
	{
		return TRUE;
	}

};