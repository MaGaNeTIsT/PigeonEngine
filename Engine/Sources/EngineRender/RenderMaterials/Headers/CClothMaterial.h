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
		DirectX::XMFLOAT4	SheenColorMetallicness;
		DirectX::XMFLOAT4	SubsurfaceColor;
	};
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#if _DEVELOPMENT_EDITOR
public:
	virtual void			SelectedEditorUpdate()override;
protected:
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
	INT					m_MetallicnessTextureSelect;
	CHAR				m_MetallicnessTexturePath[512];
	INT					m_AmbientOcclusionTextureSelect;
	CHAR				m_AmbientOcclusionTexturePath[512];
#endif
protected:
	RenderParams		m_RenderParams;
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_EmissiveTexture;
	class CTexture2D*	m_SheenColorTexture;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_RoughnessTexture;
	class CTexture2D*	m_MetallicnessTexture;
	class CTexture2D*	m_AmbientOcclusionTexture;
public:
	CClothMaterial();
	virtual ~CClothMaterial();
};