

#include "Object.h"

namespace PigeonEngine
{

	PObject::PObject() : m_Active(FALSE)
	{
	}
	PObject::~PObject()
	{
		Destroy();
	}

	void PObject::SetIsTickable(const BOOL& bTickable)
	{
		this->bCanTick = bTickable;
	}

	BOOL PObject::IsTickable() const
	{
		return this->bCanTick;
	}

	void PObject::Destroy()
	{
		delete this;
	}
#ifdef _EDITOR_ONLY
	void PObject::SetName(const EString& name)
	{
		DebugName = name;
	}
#endif
	void PObject::Uninit()
	{
		Destroy();
	}

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