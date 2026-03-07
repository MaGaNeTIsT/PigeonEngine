/*
#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CMaterialBase.h"

class CClothMaterial : public CMaterialBase
{
public:
	struct RenderParams
	{
		RenderParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	BaseColorRoughness;
		DirectX::XMFLOAT4	EmissiveAmbientOcclusion;
		DirectX::XMFLOAT4	SheenColorIsGlossy;
		DirectX::XMFLOAT4	SubsurfaceColor;
	};
public:
	void	SetNormalTexture(class CTexture2D* tex);
	void	SetAlbedoTexture(class CTexture2D* tex);
	void	SetEmissiveTexture(class CTexture2D* tex);
	void	SetSheenColorTexture(class CTexture2D* tex);
	void	SetRoughnessTexture(class CTexture2D* tex);
	void	SetAmbientOcclusionTexture(class CTexture2D* tex);
	void	SetSubsurfaceTexture(class CTexture2D* tex);
public:
	class CTexture2D*	GetNormalTexture()const;
	class CTexture2D*	GetAlbedoTexture()const;
	class CTexture2D*	GetEmissiveTexture()const;
	class CTexture2D*	GetSheenColorTexture()const;
	class CTexture2D*	GetRoughnessTexture()const;
	class CTexture2D*	GetAmbientOcclusionTexture()const;
	class CTexture2D*	GetSubsurfaceTexture()const;
public:
	virtual void	SetIsGlossyRoughness(const BOOL32& v);
	virtual void	SetRoughness(const FLOAT& v);
	virtual void	SetAmbientOcclusion(const FLOAT& v);
	virtual void	SetBaseColor(const CustomStruct::CColor& clr);
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr);
	virtual void	SetSheenColor(const CustomStruct::CColor& clr);
	virtual void	SetSubsurfaceColor(const CustomStruct::CColor& clr);
public:
	virtual BOOL32					GetIsGlossyRoughness()const;
	virtual FLOAT					GetRoughness()const;
	virtual FLOAT					GetAmbientOcclusion()const;
	virtual CustomStruct::CColor	GetBaseColor()const;
	virtual CustomStruct::CColor	GetEmissiveColor()const;
	virtual CustomStruct::CColor	GetSheenColor()const;
	virtual CustomStruct::CColor	GetSubsurfaceColor()const;
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void			HookSelectedTextureUpdate();
	virtual void			HookApplyTextureUpdate();
	virtual void			SelectedClothUpdate();
	virtual void			SelectedEditorUpdate()override;
protected:
	BOOL32				m_IsGlossy;
	INT32					m_AlbedoTextureSelect;
	CHAR				m_AlbedoTexturePath[512];
	INT32					m_EmissiveTextureSelect;
	CHAR				m_EmissiveTexturePath[512];
	INT32					m_SheenColorTextureSelect;
	CHAR				m_SheenColorTexturePath[512];
	INT32					m_NormalTextureSelect;
	CHAR				m_NormalTexturePath[512];
	INT32					m_RoughnessTextureSelect;
	CHAR				m_RoughnessTexturePath[512];
	INT32					m_AmbientOcclusionTextureSelect;
	CHAR				m_AmbientOcclusionTexturePath[512];
	INT32					m_SubsurfaceTextureSelect;
	CHAR				m_SubsurfaceTexturePath[512];
#endif
protected:
	void*				m_RenderParams;
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_EmissiveTexture;
	class CTexture2D*	m_SheenColorTexture;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_RoughnessTexture;
	class CTexture2D*	m_AmbientOcclusionTexture;
	class CTexture2D*	m_SubsurfaceTexture;
public:
	CClothMaterial();
	CClothMaterial(const std::string& name, MaterialType materialType, const UINT32& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT32& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName);
	virtual ~CClothMaterial();
};

class CClothAnisotropicMaterial : public CClothMaterial
{
public:
	struct RenderParams
	{
		RenderParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	BaseColorRoughness;
		DirectX::XMFLOAT4	EmissiveAmbientOcclusion;
		DirectX::XMFLOAT4	SheenColorIsGlossy;
		DirectX::XMFLOAT4	SubsurfaceColor;
		DirectX::XMFLOAT4	AnisotropyStrengthDirection;
	};
public:
	void	SetAnisotropyStrengthTexture(class CTexture2D* tex);
	void	SetAnisotropyDirectionTexture(class CTexture2D* tex);
public:
	class CTexture2D*	GetAnisotropyStrengthTexture()const;
	class CTexture2D*	GetAnisotropyDirectionTexture()const;
public:
	void			SetAnisotropyStrength(const FLOAT& v);
	void			SetAnisotropyDirection(const FLOAT& v);
	virtual void	SetIsGlossyRoughness(const BOOL32& v)override;
	virtual void	SetRoughness(const FLOAT& v)override;
	virtual void	SetAmbientOcclusion(const FLOAT& v)override;
	virtual void	SetBaseColor(const CustomStruct::CColor& clr)override;
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr)override;
	virtual void	SetSheenColor(const CustomStruct::CColor& clr)override;
	virtual void	SetSubsurfaceColor(const CustomStruct::CColor& clr)override;
public:
	FLOAT							GetAnisotropyStrength()const;
	FLOAT							GetAnisotropyDirection()const;
	virtual BOOL32					GetIsGlossyRoughness()const override;
	virtual FLOAT					GetRoughness()const override;
	virtual FLOAT					GetAmbientOcclusion()const override;
	virtual CustomStruct::CColor	GetBaseColor()const override;
	virtual CustomStruct::CColor	GetEmissiveColor()const override;
	virtual CustomStruct::CColor	GetSheenColor()const override;
	virtual CustomStruct::CColor	GetSubsurfaceColor()const override;
public:
	virtual void	Init()override;
	virtual void	Bind()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	HookSelectedTextureUpdate()override;
	virtual void	HookApplyTextureUpdate()override;
	virtual void	SelectedEditorUpdate()override;
protected:
	INT32					m_AnisotropyStrengthSelect;
	CHAR				m_AnisotropyStrengthPath[512];
	INT32					m_AnisotropyDirectionSelect;
	CHAR				m_AnisotropyDirectionPath[512];
#endif
protected:
	class CTexture2D*	m_AnisotropyStrengthTexture;
	class CTexture2D*	m_AnisotropyDirectionTexture;
public:
	CClothAnisotropicMaterial();
	virtual ~CClothAnisotropicMaterial();
};
*/