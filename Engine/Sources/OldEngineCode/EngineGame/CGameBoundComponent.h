#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "./CGameStructCommon.h"
#include "./CComponent.h"

class CCamera;

class CGameBoundBaseComponent : public CBaseComponent
{
public:
	virtual BOOL					InsertCameraFrustum(const CustomStruct::CCullingFrustumInfo& cullingFrustum)const = 0;
	virtual CustomStruct::CRect		GetScreenCoordRect(const CCamera* camera)const = 0;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual BOOL	SelectedInEditorScreen(const CustomType::Vector2& mousePos, const CCamera* camera, const CustomStruct::CCullingFrustumInfo& cullingFrustum, CustomType::Vector2& screenAnchor, FLOAT& viewDepth)const = 0;
#endif
public:
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd)const = 0;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen)const = 0;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd, std::vector<FLOAT>& intersectPoints)const = 0;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen, std::vector<FLOAT>& intersectPoints)const = 0;
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
#endif
public:
	CGameBoundBaseComponent();
	virtual ~CGameBoundBaseComponent();
};

class CGameBoundSphereComponent : public CGameBoundBaseComponent
{
public:
	CustomType::Vector3			GetWorldAnchor()const;
	const CustomType::Vector3&	GetLocalAnchor()const;
	const FLOAT&				GetRadius()const;
public:
	void	SetLocalAnchorRadius(const CustomType::Vector3& p, const FLOAT& r);
	void	SetLocalAnchor(const CustomType::Vector3& v);
	void	SetRadius(const FLOAT& v);
public:
	virtual BOOL					InsertCameraFrustum(const CustomStruct::CCullingFrustumInfo& cullingFrustum)const override;
	virtual CustomStruct::CRect		GetScreenCoordRect(const CCamera* camera)const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual BOOL	SelectedInEditorScreen(const CustomType::Vector2& mousePos, const CCamera* camera, const CustomStruct::CCullingFrustumInfo& cullingFrustum, CustomType::Vector2& screenAnchor, FLOAT& viewDepth)const override;
#endif
public:
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd, std::vector<FLOAT>& intersectPoints)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen, std::vector<FLOAT>& intersectPoints)const override;
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
#endif
protected:
	CustomType::Vector3		m_Anchor;
	FLOAT					m_Radius;
public:
	CGameBoundSphereComponent();
	CGameBoundSphereComponent(const CustomType::Vector3& anchor, const FLOAT& radius);
	virtual ~CGameBoundSphereComponent();
};

class CGameBoundBoxComponent : public CGameBoundBaseComponent
{
public:
	CustomType::Vector3			GetWorldAnchor()const;
	const CustomType::Vector3&	GetLocalAnchor()const;
	CustomType::Vector3			GetExtent()const;
	void						GetExtent(FLOAT& x, FLOAT& y, FLOAT& z)const;
public:
	void	SetLocalAnchorExtent(const CustomType::Vector3& p, const CustomType::Vector3& e);
	void	SetLocalAnchorExtent(const CustomType::Vector3& p, const FLOAT& x, const FLOAT& y, const FLOAT& z);
	void	SetLocalAnchor(const CustomType::Vector3& v);
	void	SetExtent(const CustomType::Vector3& v);
	void	SetExtent(const FLOAT& x, const FLOAT& y, const FLOAT& z);
public:
	virtual BOOL					InsertCameraFrustum(const CustomStruct::CCullingFrustumInfo& cullingFrustum)const override;
	virtual CustomStruct::CRect		GetScreenCoordRect(const CCamera* camera)const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual BOOL	SelectedInEditorScreen(const CustomType::Vector2& mousePos, const CCamera* camera, const CustomStruct::CCullingFrustumInfo& cullingFrustum, CustomType::Vector2& screenAnchor, FLOAT& viewDepth)const override;
#endif
public:
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayEnd, std::vector<FLOAT>& intersectPoints)const override;
	virtual BOOL	RayCast(const CustomType::Vector3& rayStart, const CustomType::Vector3& rayDir, const FLOAT& rayLen, std::vector<FLOAT>& intersectPoints)const override;
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
#endif
protected:
	CustomType::Vector3		m_Anchor;
	FLOAT					m_Extent[3];
public:
	CGameBoundBoxComponent();
	CGameBoundBoxComponent(const CustomType::Vector3& anchor, const CustomType::Vector3& extent);
	CGameBoundBoxComponent(const CustomType::Vector3& anchor, const FLOAT& x, const FLOAT& y, const FLOAT& z);
	virtual ~CGameBoundBoxComponent();
};