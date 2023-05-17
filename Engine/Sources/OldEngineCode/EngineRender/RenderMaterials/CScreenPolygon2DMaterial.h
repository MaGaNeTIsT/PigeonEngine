#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../../EngineBase/Headers/CBaseType.h"
#include "../../RenderBase/Headers/CMaterialBase.h"

class CScreenPolygon2DMaterial : public CMaterialBase
{
public:
	void	SetPixelShader(class CPixelShader* shader);
	void	SetAlbedoTexture(class CTexture2D* tex);
public:
	virtual void			Init()override;
	virtual void			Bind()const override;
	virtual const void*		GetConstantData()const override;
#ifdef _DEVELOPMENT_EDITOR
public:
	virtual void			SelectedEditorUpdate()override;
#endif
protected:
	class CTexture2D*	m_AlbedoTexture;
public:
	CScreenPolygon2DMaterial();
	virtual ~CScreenPolygon2DMaterial();
};