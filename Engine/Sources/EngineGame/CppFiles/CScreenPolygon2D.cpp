#include "../Headers/CScreenPolygon2D.h"

CScreenPolygon2D::CScreenPolygon2D(const std::string& vertexShaderName, const std::string& pixelShaderName, CustomType::Vector4 screenPosition)
{
	this->m_2DPosition = screenPosition;
	this->m_Mesh = CMeshManager::LoadPolygon2DMesh(this->m_2DPosition);
	this->m_MeshRenderer = new CMeshRenderer(this, vertexShaderName, pixelShaderName);
	this->m_MeshRenderer->LoadShader();
}
CScreenPolygon2D::~CScreenPolygon2D()
{
	if (this->m_MeshRenderer != NULL)
	{
		delete (this->m_MeshRenderer);
		this->m_MeshRenderer = NULL;
	}
}
void CScreenPolygon2D::Draw()
{
	this->m_MeshRenderer->Draw();
}