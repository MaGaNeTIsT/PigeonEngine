#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CTexture2D.h"
#include "../../RenderBase/Headers/CRenderDevice.h"

CTexture2D::CTexture2D(const std::string& name, Microsoft::WRL::ComPtr<ID3D11Texture2D> texture2D, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv)
{
	this->m_TexturePath			= name;
	this->m_Texture2D			= texture2D;
	this->m_ShaderResourceView	= srv;
}
CTexture2D::~CTexture2D()
{
}