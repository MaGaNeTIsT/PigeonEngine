#include "../../Headers/Object/CScreenPolygon2D.h"
#include "../../Headers/Base/CTimer.h"
#include "../../Headers/Base/CManager.h"

CScreenPolygon2D::CScreenPolygon2D(const std::string& vertexShaderName, const std::string& pixelShaderName, CustomType::Vector4 screenPosition)
{
	this->m_2DPosition = screenPosition;
	this->m_Mesh = CMeshManager::LoadPolygon2DMesh(this->m_2DPosition);
	this->m_MeshRenderer = new CMeshRenderer(this, vertexShaderName, vertexShaderName);
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
void CScreenPolygon2D::Init()
{
	m_Time = 0.f;
}
void CScreenPolygon2D::Uninit()
{
}
void CScreenPolygon2D::Update()
{
	m_Time += static_cast<FLOAT>(CManager::GetTimer()->GetDeltaTime());
}
void CScreenPolygon2D::PrepareDraw()
{
}
void CScreenPolygon2D::Draw()
{
	this->PrepareDraw();
	this->m_MeshRenderer->Draw();
}