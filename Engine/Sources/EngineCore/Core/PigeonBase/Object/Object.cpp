#include "../Headers/Object.h"

namespace PigeonEngine
{

	PObject::PObject() : m_Active(FALSE)
	{
	}
	PObject::~PObject()
	{
	}

	void PObject::SetIsTickable(const BOOL& bTickable)
	{
		this->bCanTick = bTickable;
	}

	BOOL PObject::IsTickable() const
	{
		return this->bCanTick;
	}
#ifdef _DEVELOPMENT_EDITOR
	void PObject::SetName(const std::string& name)
	{
		m_Name = name;
	}
#endif
	BOOL PObject::IsActive()const
	{
		return m_Active;
	}
	void PObject::Active()
	{
		m_Active = TRUE;
	}
	void PObject::Inactive()
	{
		m_Active = FALSE;
	}

};