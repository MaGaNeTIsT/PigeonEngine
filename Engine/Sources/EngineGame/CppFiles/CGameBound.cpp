#include "../Headers/CGameBound.h"
#include "../Headers/CGameObject.h"

CGameBoundBase::CGameBoundBase() : CBaseComponent(TRUE, FALSE, FALSE)
{

}
CGameBoundBase::~CGameBoundBase()
{

}
void CGameBoundBase::Init()
{

}
void CGameBoundBase::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundBase::SelectedEditorUpdate()
{

}
#endif



CGameBoundSphere::CGameBoundSphere()
{
	this->m_Anchor = CustomType::Vector3::Zero();
	this->m_Radius = CustomStruct::CGameBaseSetting::GameBoundMinimum;
}
CGameBoundSphere::CGameBoundSphere(const CustomType::Vector3& anchor, const FLOAT& radius)
{
	this->m_Anchor = anchor;
	this->m_Radius = radius;
}
CGameBoundSphere::~CGameBoundSphere()
{

}
CustomType::Vector3 CGameBoundSphere::GetWorldAnchor()const
{
	if (this->m_GameObject != NULL)
	{
		CustomType::Vector3 result(this->m_GameObject->GetWorldPosition() + this->m_Anchor);
		return result;
	}
	return (this->m_Anchor);
}
const CustomType::Vector3& CGameBoundSphere::GetLocalAnchor()const
{
	return (this->m_Anchor);
}
const FLOAT& CGameBoundSphere::GetRadius()const
{
	return (this->m_Radius);
}
void CGameBoundSphere::SetLocalAnchorRadius(const CustomType::Vector3& p, const FLOAT& r)
{
	this->m_Anchor = p;
	this->m_Radius = r;
}
void CGameBoundSphere::SetLocalAnchor(const CustomType::Vector3& v)
{
	this->m_Anchor = v;
}
void CGameBoundSphere::SetRadius(const FLOAT& v)
{
	this->m_Radius = v;
}
void CGameBoundSphere::Init()
{

}
void CGameBoundSphere::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundSphere::SelectedEditorUpdate()
{

}
#endif



CGameBoundBox::CGameBoundBox()
{
	this->m_Anchor		= CustomType::Vector3::Zero();
	this->m_Extent[0]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
	this->m_Extent[1]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
	this->m_Extent[2]	= CustomStruct::CGameBaseSetting::GameBoundHalfMinimum;
}
CGameBoundBox::CGameBoundBox(const CustomType::Vector3& anchor, const CustomType::Vector3& extent)
{
	this->m_Anchor		= anchor;
	this->m_Extent[0]	= extent.X();
	this->m_Extent[1]	= extent.Y();
	this->m_Extent[2]	= extent.Z();
}
CGameBoundBox::CGameBoundBox(const CustomType::Vector3& anchor, const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Anchor		= anchor;
	this->m_Extent[0]	= x;
	this->m_Extent[1]	= y;
	this->m_Extent[2]	= z;
}
CGameBoundBox::~CGameBoundBox()
{

}
CustomType::Vector3 CGameBoundBox::GetWorldAnchor()const
{
	if (this->m_GameObject != NULL)
	{
		CustomType::Vector3 result(this->m_GameObject->GetWorldPosition() + this->m_Anchor);
		return result;
	}
	return (this->m_Anchor);
}
const CustomType::Vector3& CGameBoundBox::GetLocalAnchor()const
{
	return (this->m_Anchor);
}
CustomType::Vector3 CGameBoundBox::GetExtent()const
{
	return (CustomType::Vector3(this->m_Extent[0], this->m_Extent[1], this->m_Extent[2]));
}
void CGameBoundBox::GetExtent(FLOAT& x, FLOAT& y, FLOAT& z)const
{
	x = this->m_Extent[0]; y = this->m_Extent[1]; z = this->m_Extent[2];
}
void CGameBoundBox::SetLocalAnchorExtent(const CustomType::Vector3& p, const CustomType::Vector3& e)
{
	this->m_Anchor		= p;
	this->m_Extent[0]	= e.X();
	this->m_Extent[1]	= e.Y();
	this->m_Extent[2]	= e.Z();
}
void CGameBoundBox::SetLocalAnchorExtent(const CustomType::Vector3& p, const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Anchor		= p;
	this->m_Extent[0]	= x;
	this->m_Extent[1]	= y;
	this->m_Extent[2]	= z;
}
void CGameBoundBox::SetLocalAnchor(const CustomType::Vector3& v)
{
	this->m_Anchor = v;
}
void CGameBoundBox::SetExtent(const CustomType::Vector3& v)
{
	this->m_Extent[0] = v.X();
	this->m_Extent[1] = v.Y();
	this->m_Extent[2] = v.Z();
}
void CGameBoundBox::SetExtent(const FLOAT& x, const FLOAT& y, const FLOAT& z)
{
	this->m_Extent[0] = x;
	this->m_Extent[1] = y;
	this->m_Extent[2] = z;
}
void CGameBoundBox::Init()
{

}
void CGameBoundBox::Uninit()
{

}
#ifdef _DEVELOPMENT_EDITOR
void CGameBoundBox::SelectedEditorUpdate()
{

}
#endif