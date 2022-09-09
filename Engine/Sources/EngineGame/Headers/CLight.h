#pragma once

#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "./CGameObject.h"

class CLight : public CGameObject
{
public:
	const CustomStruct::CLightData* GetLightData()const { return (&(this->m_Data)); }
	void SetLightColor(const CustomStruct::CColor& color) { this->m_Data.Color = color; }
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
	virtual void	Update()override;
public:
	CLight();
	CLight(const CustomType::Quaternion& rotation);
	CLight(const CustomType::Quaternion& rotation, const CustomStruct::CColor& color);
	CLight(const CLight& light);
	virtual ~CLight();
protected:
	CustomStruct::CLightData m_Data;
};