#pragma once

#include "../../EngineRender/RenderBase/Headers/CRenderDevice.h"
#include "./CGameObject.h"

class CLightBase : public CGameObject
{
public:
	struct LightShadowInfo
	{
		CustomType::Vector2Int					Size;
		UINT									Depth;
		CRenderDevice::RenderTexture2DViewInfo	Texture;
	};
	enum LightType
	{
		LIGHT_TYPE_DIRECTIONAL	= 0,
		LIGHT_TYPE_POINT		= 1,
		LIGHT_TYPE_SPOT			= 2
	};
public:
	BOOL		IsTransmitShadow() { return (this->m_ShadowInfo != nullptr); }
	LightType	GetLightType() { return (this->m_LightType); }
	void		GetColor(CustomStruct::CColor& color, FLOAT& intensity);
	void		SetColor(const CustomStruct::CColor& color, const FLOAT& intensity);
	BOOL		GetShadowSize(CustomType::Vector2Int& output);
	BOOL		SetShadowInfo(const CustomType::Vector2Int& shadowSize, const UINT& shadowDepth);
public:
	virtual CustomType::Matrix4x4	GetCurrentViewMatrix(const UINT& extraIndex = 0u) = 0;
	virtual CustomType::Matrix4x4	GetPreviousViewMatrix(const UINT& extraIndex = 0u) = 0;
protected:
	static BOOL		CreateShadowTexture(CRenderDevice::RenderTexture2DViewInfo& output, const CustomType::Vector2Int& shadowSize, const UINT& shadowDepth);
	void			SetLightType(LightType type) { this->m_LightType = type; }
public:
	virtual void	Init()override {}
	virtual void	Uninit()override {}
	virtual void	Update()override {}
public:
	CLightBase();
	CLightBase(const CLightBase& light);
	virtual ~CLightBase() {}
protected:
	LightType							m_LightType;
	CustomStruct::CColor				m_Color;
	FLOAT								m_Intensity;
	std::shared_ptr<LightShadowInfo>	m_ShadowInfo;
};

class CLightDirectional : public CLightBase
{
public:
	void	SetCurrentCamera(class Camera* camera);
public:
	virtual CustomType::Matrix4x4	GetCurrentViewMatrix(const UINT& extraIndex = 0u)override;
	virtual CustomType::Matrix4x4	GetPreviousViewMatrix(const UINT& extraIndex = 0u)override;
public:
	virtual void	Update()override;
public:
	CLightDirectional();
	CLightDirectional(const CLightDirectional& light);
	virtual ~CLightDirectional() {}
protected:
	Camera*					m_CurrentCamera;
	INT						m_FrameCounter;
	CustomType::Matrix4x4	m_ViewMatrix[2u];
};

class CLightPoint : public CLightBase
{
public:
	const FLOAT&	GetRadius()const { return (this->m_Radius); }
	void			SetRadius(const FLOAT& value) { this->m_Radius = value; }
	const FLOAT&	GetAttenuationExponent()const { return (this->m_AttenuationExponent); }
	void			SetAttenuationExponent(const FLOAT& value) { this->m_AttenuationExponent = value; }
public:
	virtual CustomType::Matrix4x4	GetCurrentViewMatrix(const UINT& extraIndex = 0u)override;
	virtual CustomType::Matrix4x4	GetPreviousViewMatrix(const UINT& extraIndex = 0u)override;
public:
	CLightPoint();
	CLightPoint(const CLightPoint& light);
	virtual ~CLightPoint() {}
protected:
	FLOAT	m_AttenuationExponent;
	FLOAT	m_Radius;
};

class CLightSpot : public CLightBase
{
public:
	const FLOAT&	GetRange()const { return (this->m_Range); }
	void			SetRange(const FLOAT& v) { this->m_Range = v; }
	const FLOAT&	GetHalfRadian()const { return (this->m_HalfRadian); }
	void			SetHalfRadian(const FLOAT& v) { this->m_HalfRadian = v; }
	const FLOAT&	GetCosHalfRadian()const { return (this->m_CosHalfRadian); }
	void			SetCosHalfRadian(const FLOAT& v) { this->m_CosHalfRadian = v; }
public:
	virtual CustomType::Matrix4x4	GetCurrentViewMatrix(const UINT& extraIndex = 0u)override;
	virtual CustomType::Matrix4x4	GetPreviousViewMatrix(const UINT& extraIndex = 0u)override;
public:
	virtual void	Update()override;
public:
	CLightSpot();
	CLightSpot(const CLightSpot& light);
	virtual ~CLightSpot() {}
protected:
	FLOAT					m_Range;
	FLOAT					m_HalfRadian;
	FLOAT					m_CosHalfRadian;
protected:
	INT						m_FrameCounter;
	CustomType::Matrix4x4	m_ViewMatrix[2];
};