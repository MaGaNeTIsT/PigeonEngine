#include "../../../../../Entry/EngineMain.h"
#include "../Headers/CTextureType.h"
#include "../Headers/CTextureManager.h"

CTextureBase::CTextureBase(const std::string& name)
{
	this->m_Name = name;
}
CTextureBase::CTextureBase(const CTextureBase& v)
{
	this->m_Name = v.m_Name;
}



Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CTexture2D::GetShaderResourceView()const
{
	return (this->m_Texture2D.ShaderResourceView);
}
CTexture2D::CTexture2D(const std::string& name, const CRenderDevice::Texture2DViewInfo& v) : CTextureBase(name)
{
	this->m_Texture2D = v;
}
CTexture2D::CTexture2D(const CTexture2D& v) : CTextureBase(v.m_Name)
{
	this->m_Texture2D = v.m_Texture2D;
}



CTextureCube::CTextureCube(const std::string& name, const CRenderDevice::TextureCubeViewInfo& v) : CTextureBase(name)
{
	this->m_TextureCube = v;
}
CTextureCube::CTextureCube(const CTextureCube& v) : CTextureBase(v.m_Name)
{
	this->m_TextureCube = v.m_TextureCube;
}
Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> CTextureCube::GetShaderResourceView()const
{
	return (this->m_TextureCube.ShaderResourceView);
}



CTextureCubeCombine::CTextureCubeCombine(const std::string* nameFace, const CRenderDevice::TextureCubeViewInfo& v) : CTextureCube(nameFace[0], v)
{
	this->m_NameFace[0] = nameFace[1];
	this->m_NameFace[1] = nameFace[2];
	this->m_NameFace[2] = nameFace[3];
	this->m_NameFace[3] = nameFace[4];
	this->m_NameFace[4] = nameFace[5];
}
CTextureCubeCombine::CTextureCubeCombine(const CTextureCubeCombine& v) : CTextureCube(v.m_Name, v.m_TextureCube)
{
	this->m_NameFace[0] = v.m_NameFace[0];
	this->m_NameFace[1] = v.m_NameFace[1];
	this->m_NameFace[2] = v.m_NameFace[2];
	this->m_NameFace[3] = v.m_NameFace[3];
	this->m_NameFace[4] = v.m_NameFace[4];
}