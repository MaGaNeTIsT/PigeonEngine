#include "../../Entry/MyMain.h"
#include "../../Headers/Object/CPolygon2DBase.h"

CPolygon2DBase::CPolygon2DBase()
{
	m_2DPosition = CustomType::Vector4(0, 0, 0, 0);
	m_Time = 0.f;
}
CPolygon2DBase::~CPolygon2DBase()
{
}
void CPolygon2DBase::SetScreenPosition(const CustomType::Vector4& rect)
{
	m_2DPosition = rect;
}