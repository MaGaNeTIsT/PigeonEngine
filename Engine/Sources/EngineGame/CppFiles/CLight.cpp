#include "../../../../Entry/EngineMain.h"
#include "../Headers/CLight.h"

CLight::CLight()
{
	this->m_Position.Reset();
	this->m_Rotation.Reset();
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_ShadowSize = CustomType::Vector2Int(ENGINE_SHADOW_MAP_SIZE, ENGINE_SHADOW_MAP_SIZE);
}
CLight::CLight(const CustomType::Quaternion& rotation)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_ShadowSize = CustomType::Vector2Int(ENGINE_SHADOW_MAP_SIZE, ENGINE_SHADOW_MAP_SIZE);
}
CLight::CLight(const CustomType::Vector2Int& shadowSize)
{
	this->m_Position.Reset();
	this->m_Rotation.Reset();
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_ShadowSize = shadowSize;
}
CLight::CLight(const CustomType::Quaternion& rotation, const CustomType::Vector2Int& shadowSize)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = CustomStruct::CColor(1.f, 1.f, 1.f, 1.f);
	this->m_ShadowSize = shadowSize;
}
CLight::CLight(const CustomType::Quaternion& rotation, const CustomStruct::CColor& color)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = color;
	this->m_ShadowSize = CustomType::Vector2Int(ENGINE_SHADOW_MAP_SIZE, ENGINE_SHADOW_MAP_SIZE);
}
CLight::CLight(const CustomType::Quaternion& rotation, const CustomStruct::CColor& color, const CustomType::Vector2Int& shadowSize)
{
	this->m_Position.Reset();
	this->m_Rotation = rotation;
	this->m_Scale = 1.f;
	this->m_Data.Color = color;
	this->m_ShadowSize = shadowSize;
}
CLight::CLight(const CLight& light)
{
	this->m_Position = light.m_Position;
	this->m_Rotation = light.m_Rotation;
	this->m_Scale = 1.f;
	this->m_Data = light.m_Data;
	this->m_ShadowSize = light.m_ShadowSize;
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
	FLOAT tempLightColor[3] = { this->m_Data.Color.r,this->m_Data.Color.g,this->m_Data.Color.b };
	ImGui::Begin("Light data");
	ImGui::ColorEdit3("Color", (FLOAT*)&tempLightColor);
	ImGui::End();
	this->m_Data.Color.r = tempLightColor[0];
	this->m_Data.Color.g = tempLightColor[1];
	this->m_Data.Color.b = tempLightColor[2];
	this->m_Data.Direction = this->GetForwardVector().GetXMFLOAT4();
}