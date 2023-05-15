#include "../../Headers/Component/Component.h"

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
#ifdef _DEVELOPMENT_EDITOR
		m_Name = other.m_Name;
#endif
	}
	PComponent& PComponent::operator=(const PComponent& other)
	{
		m_Active = other.m_Active;
#ifdef _DEVELOPMENT_EDITOR
		m_Name = other.m_Name;
#endif
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
	void PComponent::FixedTick(FLOAT deltaTime)
	{
	}
#ifdef _DEVELOPMENT_EDITOR
	void PComponent::EditorTick(FLOAT deltaTime)
	{
	}
#endif

};