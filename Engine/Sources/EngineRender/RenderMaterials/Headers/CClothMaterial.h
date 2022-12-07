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
	void	SetIsGlossyRoughness(const BOOL& v);
	void	SetRoughness(const FLOAT& v);
	void	SetAmbientOcclusion(const FLOAT& v);
	void	SetBaseColor(const CustomStruct::CColor& clr);
	void	SetEmissiveColor(const CustomStruct::CColor& clr);
	void	SetSheenColor(const CustomStruct::CColor& clr);
	void	SetSubsurfaceColor(const CustomStruct::CColor& clr);
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void			SelectedEditorUpdate()override;
protected:
	BOOL				m_IsGlossy;
	INT					m_AlbedoTextureSelect;
	CHAR				m_AlbedoTexturePath[512];
	INT					m_EmissiveTextureSelect;
	CHAR				m_EmissiveTexturePath[512];
	INT					m_SheenColorTextureSelect;
	CHAR				m_SheenColorTexturePath[512];
	INT					m_NormalTextureSelect;
	CHAR				m_NormalTexturePath[512];
	INT					m_RoughnessTextureSelect;
	CHAR				m_RoughnessTexturePath[512];
	INT					m_AmbientOcclusionTextureSelect;
	CHAR				m_AmbientOcclusionTexturePath[512];
	INT					m_SubsurfaceTextureSelect;
	CHAR				m_SubsurfaceTexturePath[512];
#endif
protected:
	RenderParams		m_RenderParams;
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_EmissiveTexture;
	class CTexture2D*	m_SheenColorTexture;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_RoughnessTexture;
	class CTexture2D*	m_AmbientOcclusionTexture;
	class CTexture2D*	m_SubsurfaceTexture;
public:
	CClothMaterial();
	virtual ~CClothMaterial();
};