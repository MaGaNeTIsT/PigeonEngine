#pragma once

#include "../../../../Entry/EngineMain.h"
#include "../../EngineBase/Headers/CBaseType.h"
#include "./CGameStructCommon.h"
#include "./CComponent.h"

class CGameBoundBase : public CBaseComponent
{
public:
	virtual void	Init()override;
	virtual void	Uninit()override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	SelectedEditorUpdate()override;
#endif
public:
	CGameBoundBase();
	virtual ~CGameBoundBase();
};

class CGameBoundSphere : public CGameBoundBase
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
	CGameBoundSphere();
	CGameBoundSphere(const CustomType::Vector3& anchor, const FLOAT& radius);
	virtual ~CGameBoundSphere();
};

class CGameBoundBox : public CGameBoundBase
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
	CGameBoundBox();
	CGameBoundBox(const CustomType::Vector3& anchor, const CustomType::Vector3& extent);
	CGameBoundBox(const CustomType::Vector3& anchor, const FLOAT& x, const FLOAT& y, const FLOAT& z);
	virtual ~CGameBoundBox();
};