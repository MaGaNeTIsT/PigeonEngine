#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CMaterialBase.h"

class CDefaultLitMaterial : public CMaterialBase
{
public:
	struct RenderParams
	{
		RenderParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	BaseColorRoughness;
		DirectX::XMFLOAT4	EmissiveAmbientOcclusion;
		DirectX::XMFLOAT4	MetallicnessReflectanceIsGlossy;
	};
public:
	void	SetNormalTexture(class CTexture2D* tex);
	void	SetAlbedoTexture(class CTexture2D* tex);
	void	SetEmissiveTexture(class CTexture2D* tex);
	void	SetRoughnessTexture(class CTexture2D* tex);
	void	SetMetallicnessTexture(class CTexture2D* tex);
	void	SetAmbientOcclusionTexture(class CTexture2D* tex);
	void	SetReflectanceTexture(class CTexture2D* tex);
public:
	class CTexture2D*	GetNormalTexture()const;
	class CTexture2D*	GetAlbedoTexture()const;
	class CTexture2D*	GetEmissiveTexture()const;
	class CTexture2D*	GetRoughnessTexture()const;
	class CTexture2D*	GetMetallicnessTexture()const;
	class CTexture2D*	GetAmbientOcclusionTexture()const;
	class CTexture2D*	GetReflectanceTexture()const;
public:
	virtual void	SetIsGlossyRoughness(const BOOL& v);
	virtual void	SetRoughness(const FLOAT& v);
	virtual void	SetMetallicness(const FLOAT& v);
	virtual void	SetReflectance(const FLOAT& v);
	virtual void	SetAmbientOcclusion(const FLOAT& v);
	virtual void	SetBaseColor(const CustomStruct::CColor& clr);
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr);
public:
	virtual BOOL					GetIsGlossyRoughness()const;
	virtual FLOAT					GetRoughness()const;
	virtual FLOAT					GetMetallicness()const;
	virtual FLOAT					GetReflectance()const;
	virtual FLOAT					GetAmbientOcclusion()const;
	virtual CustomStruct::CColor	GetBaseColor()const;
	virtual CustomStruct::CColor	GetEmissiveColor()const;
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void			HookSelectedTextureUpdate();
	virtual void			HookApplyTextureUpdate();
	virtual void			SelectedDefaultLitUpdate();
	virtual void			SelectedEditorUpdate()override;
protected:
	BOOL		m_IsGlossy;
	INT			m_NormalSelect;
	INT			m_AlbedoSelect;
	INT			m_EmissiveSelect;
	INT			m_RoughnessSelect;
	INT			m_MetallicnessSelect;
	INT			m_AmbientOcclusionSelect;
	INT			m_ReflectanceSelect;
	CHAR		m_NormalPath[512];
	CHAR		m_AlbedoPath[512];
	CHAR		m_EmissivePath[512];
	CHAR		m_RoughnessPath[512];
	CHAR		m_MetallicnessPath[512];
	CHAR		m_AmbientOcclusionPath[512];
	CHAR		m_ReflectancePath[512];
#endif
protected:
	void*				m_RenderParams;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_EmissiveTexture;
	class CTexture2D*	m_RoughnessTexture;
	class CTexture2D*	m_MetallicnessTexture;
	class CTexture2D*	m_AmbientOcclusionTexture;
	class CTexture2D*	m_ReflectanceTexture;
public:
	CDefaultLitMaterial();
	CDefaultLitMaterial(const std::string& name, MaterialType materialType, const UINT& constantSize, const CustomStruct::CRenderInputLayoutDesc* inputLayout, const UINT& inputLayoutNum, const std::string& vertexShaderName, const std::string& pixelShaderName);
	virtual ~CDefaultLitMaterial();
};

class CAnisotropicMaterial : public CDefaultLitMaterial
{
public:
	struct RenderParams
	{
		RenderParams() { ::ZeroMemory(this, sizeof(*this)); }
		DirectX::XMFLOAT4	BaseColorRoughness;
		DirectX::XMFLOAT4	EmissiveAmbientOcclusion;
		DirectX::XMFLOAT4	MetallicnessReflectanceIsGlossy;
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
	virtual void	SetIsGlossyRoughness(const BOOL& v)override;
	virtual void	SetRoughness(const FLOAT& v)override;
	virtual void	SetMetallicness(const FLOAT& v)override;
	virtual void	SetReflectance(const FLOAT& v)override;
	virtual void	SetAmbientOcclusion(const FLOAT& v)override;
	virtual void	SetBaseColor(const CustomStruct::CColor& clr)override;
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr)override;
public:
	FLOAT							GetAnisotropyStrength()const;
	FLOAT							GetAnisotropyDirection()const;
	virtual BOOL					GetIsGlossyRoughness()const override;
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
	INT			m_AnisotropyStrengthSelect;
	INT			m_AnisotropyDirectionSelect;
	CHAR		m_AnisotropyStrengthPath[512];
	CHAR		m_AnisotropyDirectionPath[512];
#endif
protected:
	class CTexture2D*	m_AnisotropyStrengthTexture;
	class CTexture2D*	m_AnisotropyDirectionTexture;
public:
	CAnisotropicMaterial();
	virtual ~CAnisotropicMaterial();
};