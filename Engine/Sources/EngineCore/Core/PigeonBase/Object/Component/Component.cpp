#include "./Component.h"

namespace PigeonEngine
{

	PComponent::PComponent()
	{
	}
	PComponent::~PComponent()
	{
	}
	void PComponent::Init()
	{
	}
	void PComponent::Uninit()
	{
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