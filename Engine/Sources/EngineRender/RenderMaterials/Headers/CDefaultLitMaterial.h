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
		DirectX::XMFLOAT4	MetallicnessReflectanceAnisotropy;
		DirectX::XMFLOAT4	ClearCoatStrengthRoughness;
	};
public:
	void	SetNormalTexture(class CTexture2D* tex);
	void	SetAlbedoTexture(class CTexture2D* tex);
	void	SetEmissiveTexture(class CTexture2D* tex);
	void	SetRoughnessTexture(class CTexture2D* tex);
	void	SetMetallicnessTexture(class CTexture2D* tex);
	void	SetAmbientOcclusionTexture(class CTexture2D* tex);
	void	SetAnisotropyTexture(class CTexture2D* tex);
	void	SetAnisotropyDirectionTexture(class CTexture2D* tex);
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void			SelectedEditorUpdate()override;
protected:
	INT				m_NormalSelect;
	INT				m_AlbedoSelect;
	INT				m_EmissiveSelect;
	INT				m_RoughnessSelect;
	INT				m_MetallicnessSelect;
	INT				m_AmbientOcclusionSelect;
	INT				m_AnisotropySelect;
	INT				m_AnisotropyDirectionSelect;
	std::string		m_NormalPath;
	std::string		m_AlbedoPath;
	std::string		m_EmissivePath;
	std::string		m_RoughnessPath;
	std::string		m_MetallicnessPath;
	std::string		m_AmbientOcclusionPath;
	std::string		m_AnisotropyPath;
	std::string		m_AnisotropyDirectionPath;
#endif
protected:
	RenderParams		m_RenderParams;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_EmissiveTexture;
	class CTexture2D*	m_RoughnessTexture;
	class CTexture2D*	m_MetallicnessTexture;
	class CTexture2D*	m_AmbientOcclusionTexture;
	class CTexture2D*	m_AnisotropyTexture;
	class CTexture2D*	m_AnisotropyDirectionTexture;
public:
	CDefaultLitMaterial();
	virtual ~CDefaultLitMaterial();
};