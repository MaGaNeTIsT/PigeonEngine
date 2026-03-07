/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderMaterials/Headers/CDefaultLitMaterial.h"

class CClearCoatMaterial : public CDefaultLitMaterial
{
public:
	struct RenderParams
	{
		RenderParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	BaseColorRoughness;
		DirectX::XMFLOAT4	EmissiveAmbientOcclusion;
		DirectX::XMFLOAT4	MetallicnessReflectanceIsGlossy;
		DirectX::XMFLOAT4	ClearCoatStrengthRoughness;
	};
public:
	void	SetClearCoatStrengthTexture(class CTexture2D* tex);
	void	SetClearCoatRoughnessTexture(class CTexture2D* tex);
	void	SetClearCoatNormalTexture(class CTexture2D* tex);
public:
	class CTexture2D*	GetClearCoatStrengthTexture()const;
	class CTexture2D*	GetClearCoatRoughnessTexture()const;
	class CTexture2D*	GetClearCoatNormalTexture()const;
public:
	void			SetClearCoatStrength(const FLOAT& v);
	void			SetClearCoatRoughness(const FLOAT& v);
	virtual void	SetIsGlossyRoughness(const BOOL32& v)override;
	virtual void	SetRoughness(const FLOAT& v)override;
	virtual void	SetMetallicness(const FLOAT& v)override;
	virtual void	SetReflectance(const FLOAT& v)override;
	virtual void	SetAmbientOcclusion(const FLOAT& v)override;
	virtual void	SetBaseColor(const CustomStruct::CColor& clr)override;
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr)override;
public:
	FLOAT							GetClearCoatStrength()const;
	FLOAT							GetClearCoatRoughness()const;
	virtual BOOL32					GetIsGlossyRoughness()const override;
	virtual FLOAT					GetRoughness()const override;
	virtual FLOAT					GetMetallicness()const override;
	virtual FLOAT					GetReflectance()const override;
	virtual FLOAT					GetAmbientOcclusion()const override;
	virtual CustomStruct::CColor	GetBaseColor()const override;
	virtual CustomStruct::CColor	GetEmissiveColor()const override;
public:
	virtual void	Init()override;
	virtual void	Bind()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	HookSelectedTextureUpdate()override;
	virtual void	HookApplyTextureUpdate()override;
	virtual void	SelectedEditorUpdate()override;
protected:
	INT32			m_ClearCoatStrengthSelect;
	INT32			m_ClearCoatRoughnessSelect;
	INT32			m_ClearCoatNormalSelect;
	CHAR		m_ClearCoatStrengthPath[512];
	CHAR		m_ClearCoatRoughnessPath[512];
	CHAR		m_ClearCoatNormalPath[512];
#endif
protected:
	class CTexture2D*	m_ClearCoatStrengthTexture;
	class CTexture2D*	m_ClearCoatRoughnessTexture;
	class CTexture2D*	m_ClearCoatNormalTexture;
public:
	CClearCoatMaterial();
	virtual ~CClearCoatMaterial();
};
*/