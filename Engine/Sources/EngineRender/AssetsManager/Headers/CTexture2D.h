#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

class CTexture2D
{
public:
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> GetShaderResourceView() { return (this->m_Texture.ShaderResourceView); }
public:
	CTexture2D(const std::string& name, const CRenderDevice::Texture2DViewInfo& tex);
	virtual ~CTexture2D();
protected:
	std::string							m_Name;
	CRenderDevice::Texture2DViewInfo	m_Texture;
};