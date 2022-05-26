#include "../../Headers/Game/CMeshRenderer.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShaderManager.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Game/CGameObject.h"
#include "../../Headers/Game/CMesh.h"
#include "../../Headers/Game/CCamera.h"

CMeshRenderer::CMeshRenderer(CGameObject* gameObject, const std::string& vertexShaderName, const std::string& pixelShaderName, CRenderTypeEnum type)
{
	this->m_RenderType			= type;
	this->m_VertexShaderName	= vertexShaderName;
	this->m_PixelShaderName		= pixelShaderName;
	this->m_Shader				= NULL;
	this->m_ExtraShader			= NULL;
	this->m_GameObject			= gameObject;
}
CMeshRenderer::~CMeshRenderer()
{
	if (this->m_Shader != NULL)
	{
		delete (this->m_Shader);
		this->m_Shader = NULL;
	}
	if (this->m_ExtraShader != NULL)
	{
		delete (this->m_ExtraShader);
		this->m_ExtraShader = NULL;
	}
}
void CMeshRenderer::LoadShader()
{
	if (this->m_Shader != NULL)
		delete (this->m_Shader);
	this->m_Shader = CShaderManager::LoadShader(this->m_VertexShaderName, this->m_PixelShaderName);
}
BOOL CMeshRenderer::CreateConstantBuffer(const UINT& sizeData)
{
	if (m_Shader == NULL)
		return false;
	return (this->m_Shader->CreateConstantBuffer(sizeData));
}
void CMeshRenderer::UploadConstantBuffer(const void* ptrData)
{
	if (this->m_Shader == NULL)
		return;
	this->m_Shader->UploadConstantBuffer(ptrData);
}
void CMeshRenderer::BindConstantBuffer(const UINT& startSlot)
{
	if (this->m_Shader == NULL)
		return;
	this->m_Shader->BindConstantBuffer(startSlot);
}
void CMeshRenderer::LoadExtraShader(const std::string& vertexShaderName, const std::string& pixelShaderName)
{
	if (this->m_ExtraShader != NULL)
		delete (this->m_ExtraShader);
	this->m_ExtraShader = CShaderManager::LoadShader(vertexShaderName, pixelShaderName);
}
void CMeshRenderer::Draw()
{
	if (this->m_Shader == NULL || this->m_GameObject == NULL || this->m_GameObject->GetMesh() == NULL)
		return;
	this->m_Shader->BindShaderInputLayout();
	CRenderDevice::BindVertexBuffer(this->m_GameObject->GetMesh()->GetVertexBuffer());
	CRenderDevice::BindIndexBuffer(this->m_GameObject->GetMesh()->GetIndexBuffer());
	CRenderDevice::DrawIndexed(this->m_GameObject->GetMesh()->GetIndexCount());
}
void CMeshRenderer::DrawExtra()
{
	if (this->m_ExtraShader == NULL || this->m_GameObject == NULL || this->m_GameObject->GetMesh() == NULL)
		return;
	this->m_ExtraShader->BindShaderInputLayout();
	CRenderDevice::BindVertexBuffer(this->m_GameObject->GetMesh()->GetVertexBuffer());
	CRenderDevice::BindIndexBuffer(this->m_GameObject->GetMesh()->GetIndexBuffer());
	CRenderDevice::DrawIndexed(this->m_GameObject->GetMesh()->GetIndexCount());
}