#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CMaterialBase.h"

class CDefaultLitMaterial : public CMaterialBase
{
public:
	void	SetAlbedoTexture(class CTexture2D* tex);
	void	SetNormalTexture(class CTexture2D* tex);
	void	SetPropertyTexture(class CTexture2D* tex);
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
	INT					m_NormalTextureSelect;
	std::string			m_NormalTexturePath;
	INT					m_PropertyTextureSelect;
	std::string			m_PropertyTexturePath;
#endif
protected:
	class CTexture2D*	m_AlbedoTexture;
	class CTexture2D*	m_NormalTexture;
	class CTexture2D*	m_PropertyTexture;
public:
	CDefaultLitMaterial();
	virtual ~CDefaultLitMaterial();
};