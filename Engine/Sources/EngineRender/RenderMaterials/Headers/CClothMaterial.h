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
	std::string			m_AlbedoTexturePath;
	INT					m_EmissiveTextureSelect;
	std::string			m_EmissiveTexturePath;
	INT					m_SheenColorTextureSelect;
	std::string			m_SheenColorTexturePath;
	INT					m_NormalTextureSelect;
	std::string			m_NormalTexturePath;
	INT					m_RoughnessTextureSelect;
	std::string			m_RoughnessTexturePath;
	INT					m_MetallicnessTextureSelect;
	std::string			m_MetallicnessTexturePath;
	INT					m_AmbientOcclusionTextureSelect;
	std::string			m_AmbientOcclusionTexturePath;
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