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
	virtual void	SetIsGlossyRoughness(const BOOL& v);
	virtual void	SetRoughness(const FLOAT& v);
	virtual void	SetMetallicness(const FLOAT& v);
	virtual void	SetReflectance(const FLOAT& v);
	virtual void	SetAmbientOcclusion(const FLOAT& v);
	virtual void	SetBaseColor(const CustomStruct::CColor& clr);
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr);
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
		DirectX::XMFLOAT4	MetallicnessReflectanceIsGlossyAnisotropy;
		DirectX::XMFLOAT4	AnisotropyVerticalHorizontal;
	};
public:
	void	SetAnisotropyTexture(class CTexture2D* tex);
	void	SetAnisotropyDirectionTexture(class CTexture2D* tex);
	void	SetAnisotropyVerticalTexture(class CTexture2D* tex);
	void	SetAnisotropyHorizontalTexture(class CTexture2D* tex);
public:
	void			SetAnisotropyVertical(const FLOAT& v);
	void			SetAnisotropyHorizontal(const FLOAT& v);
	void			SetAnisotropy(const FLOAT& v);
	virtual void	SetIsGlossyRoughness(const BOOL& v)override;
	virtual void	SetRoughness(const FLOAT& v)override;
	virtual void	SetMetallicness(const FLOAT& v)override;
	virtual void	SetReflectance(const FLOAT& v)override;
	virtual void	SetAmbientOcclusion(const FLOAT& v)override;
	virtual void	SetBaseColor(const CustomStruct::CColor& clr)override;
	virtual void	SetEmissiveColor(const CustomStruct::CColor& clr)override;
public:
	virtual void	Init()override;
	virtual void	Bind()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void	HookSelectedTextureUpdate()override;
	virtual void	HookApplyTextureUpdate()override;
	virtual void	SelectedEditorUpdate()override;
protected:
	INT			m_AnisotropySelect;
	INT			m_AnisotropyDirectionSelect;
	INT			m_AnisotropyVerticalSelect;
	INT			m_AnisotropyHorizontalSelect;
	CHAR		m_AnisotropyPath[512];
	CHAR		m_AnisotropyDirectionPath[512];
	CHAR		m_AnisotropyVerticalPath[512];
	CHAR		m_AnisotropyHorizontalPath[512];
#endif
protected:
	class CTexture2D*	m_AnisotropyTexture;
	class CTexture2D*	m_AnisotropyDirectionTexture;
	class CTexture2D*	m_AnisotropyVerticalTexture;
	class CTexture2D*	m_AnisotropyHorizontalTexture;
public:
	CAnisotropicMaterial();
	virtual ~CAnisotropicMaterial();
};