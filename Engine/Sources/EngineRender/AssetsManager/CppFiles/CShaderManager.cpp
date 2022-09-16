#include "../Headers/CShaderManager.h"
#include "../../RenderBase/Headers/CRenderDevice.h"
#include "../Headers/CShader.h"

CShaderManager* CShaderManager::m_ShaderManager = new CShaderManager();

void CShaderManager::ShutDown()
{
	delete (CShaderManager::m_ShaderManager);
}
std::shared_ptr<CVertexShader> CShaderManager::LoadVertexShader(const std::string& name, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout)
{
	Microsoft::WRL::ComPtr<ID3D11VertexShader> shader = nullptr;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout = nullptr;
	CShaderManager::FindVertexShaderData(name, shader, inputLayout);
	if (shader != nullptr && inputLayout != nullptr)
		return (std::shared_ptr<CVertexShader>(new CVertexShader(name, shader, inputLayout)));

	if (CRenderDevice::LoadVertexShader(name, shader, inputLayout, layout))
	{
		CShaderManager::AddVertexShaderData(name, shader, inputLayout);
		return (std::shared_ptr<CVertexShader>(new CVertexShader(name, shader, inputLayout)));
	}
	return nullptr;
}
std::shared_ptr<CPixelShader> CShaderManager::LoadPixelShader(const std::string& name)
{
	Microsoft::WRL::ComPtr<ID3D11PixelShader> shader = nullptr;
	CShaderManager::FindPixelShaderData(name, shader);
	if (shader != nullptr)
		return (std::shared_ptr<CPixelShader>(new CPixelShader(name, shader)));

	if (CRenderDevice::LoadPixelShader(name, shader))
	{
		CShaderManager::AddPixelShaderData(name, shader);
		return (std::shared_ptr<CPixelShader>(new CPixelShader(name, shader)));
	}
	return nullptr;
}
std::shared_ptr<CComputeShader> CShaderManager::LoadComputeShader(const std::string& name)
{
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader = nullptr;
	CShaderManager::FindComputeShaderData(name, shader);
	if (shader != nullptr)
		return (std::shared_ptr<CComputeShader>(new CComputeShader(name, shader)));

	if (CRenderDevice::LoadComputeShader(name, shader))
	{
		CShaderManager::AddComputeShaderData(name, shader);
		return (std::shared_ptr<CComputeShader>(new CComputeShader(name, shader)));
	}
	return nullptr;
}
void CShaderManager::ClearVertexShaderData()
{
	m_ShaderManager->m_VertexShaderData.clear();
	m_ShaderManager->m_InputLayoutData.clear();
}
void CShaderManager::ClearPixelShaderData()
{
	m_ShaderManager->m_PixelShaderData.clear();
}
void CShaderManager::ClearComputeShaderData()
{
	m_ShaderManager->m_ComputeShaderData.clear();
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
void CShaderManager::AddComputeShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader)
{
	m_ShaderManager->m_ComputeShaderData[name] = shader;
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
void CShaderManager::FindComputeShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& shader)
{
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ComputeShader>>::iterator is = m_ShaderManager->m_ComputeShaderData.find(name);
	shader = (is == m_ShaderManager->m_ComputeShaderData.end()) ? nullptr : (is->second);
}