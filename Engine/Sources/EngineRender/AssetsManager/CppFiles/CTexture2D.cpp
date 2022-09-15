#include "../Headers/CTexture2D.h"

CTexture2D::CTexture2D(const std::string& name, const CRenderDevice::Texture2DViewInfo& tex)
{
	this->m_Name	= name;
	this->m_Texture	= tex;
}
CTexture2D::~CTexture2D()
{
}