#include "../../Entry/MyMain.h"
#include "../../Headers/Game/CLight.h"

CLight::CLight()
{
	this->m_Position.Reset();
	this->m_Rotation.Reset();
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
}
CLight::CLight(const CustomType::Quaternion& rotation)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
}
CLight::CLight(const CustomType::Quaternion& rotation, const CustomStruct::CColor& color)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = color;
}
CLight::CLight(const CLight& light)
{
	this->m_Position = light.m_Position;
	this->m_Rotation = light.m_Rotation;
	this->m_Scale = 1.f;
	this->m_Data = light.m_Data;
}
CLight::~CLight()
{
}
void CLight::Init()
{
}
void CLight::Uninit()
{
}
void CLight::Update()
{
	this->m_Data.Direction = this->GetForwardVector().GetXMFLOAT4();
}