#include "../../Headers/Base/CShaderManager.h"
#include "../../Headers/Base/CRenderDevice.h"
#include "../../Headers/Base/CShader.h"

CShaderManager* CShaderManager::m_ShaderManager = new CShaderManager();
void CShaderManager::ClearVertexShaderData()
{
	m_ShaderManager->m_VertexShaderData.clear();
	m_ShaderManager->m_InputLayoutData.clear();
}
void CShaderManager::ClearPixelShaderData()
{
	m_ShaderManager->m_PixelShaderData.clear();
}
CShader* CShaderManager::LoadShader(const std::string& vertexShaderName, const std::string& pixelShaderName)
{
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader	= nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	inputLayout		= nullptr;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader		= nullptr;
	CShaderManager::FindVertexShaderData(vertexShaderName, vertexShader, inputLayout);
	CShaderManager::FindPixelShaderData(pixelShaderName, pixelShader);
	BOOL loadVertexShaderResult	= true;
	BOOL loadPixelShaderResult	= true;
	if (vertexShader == nullptr || inputLayout == nullptr)
		loadVertexShaderResult = CRenderDevice::LoadVertexShader(vertexShaderName, vertexShader, inputLayout);
	if (pixelShader == nullptr)
		loadPixelShaderResult = CRenderDevice::LoadPixelShader(pixelShaderName, pixelShader);
	if (!loadVertexShaderResult || !loadPixelShaderResult)
		return NULL;
	CShaderManager::AddVertexShaderData(vertexShaderName, vertexShader, inputLayout);
	CShaderManager::AddPixelShaderData(pixelShaderName, pixelShader);
	CShader* shader = new CShader();
	shader->SetVertexShader(vertexShaderName, vertexShader, inputLayout);
	shader->SetPixelShader(pixelShaderName, pixelShader);
	return shader;
}
void CShaderManager::AddVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> shader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout)
{
	m_ShaderManager->m_VertexShaderData[name] = shader;
	m_ShaderManager->m_InputLayoutData[name] = inputLayout;
}
void CShaderManager::AddPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> shader)
{
	m_ShaderManager->m_PixelShaderData[name] = shader;
}
void CShaderManager::FindVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout)
{
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11VertexShader>>::iterator is = m_ShaderManager->m_VertexShaderData.find(name);
	shader = (is == m_ShaderManager->m_VertexShaderData.end()) ? nullptr : (is->second);

	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11InputLayout>>::iterator ii = m_ShaderManager->m_InputLayoutData.find(name);
	inputLayout = (ii == m_ShaderManager->m_InputLayoutData.end()) ? nullptr : (ii->second);
}
void CShaderManager::FindPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader)
{
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>>::iterator is = m_ShaderManager->m_PixelShaderData.find(name);
	shader = (is == m_ShaderManager->m_PixelShaderData.end()) ? nullptr : (is->second);
}