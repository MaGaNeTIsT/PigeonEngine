#include "../Headers/Component.h"

namespace PigeonEngine
{

	PComponent::PComponent()
	{
	}
	PComponent::~PComponent()
	{
	}
	PComponent::PComponent(const PComponent& other)
	{
		m_Active = other.m_Active;
		m_Name = other.m_Name;
	}
	PComponent& PComponent::operator=(const PComponent& other)
	{
		m_Active = other.m_Active;
		m_Name = other.m_Name;
		return (*this);
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
#ifdef _DEVELOPMENT_EDITOR
	void PComponent::EditorTick(FLOAT deltaTime)
	{
	}
#endif

};