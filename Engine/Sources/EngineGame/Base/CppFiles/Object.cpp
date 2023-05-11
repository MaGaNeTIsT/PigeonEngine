#include "../Headers/Object.h"

namespace PigeonEngine
{

	PObject::PObject()
	{
	}
	PObject::~PObject()
	{
	}
#ifdef _DEVELOPMENT_EDITOR
	void PObject::SetName(const std::string& name)
	{
		m_Name = name;
	}
#endif

};