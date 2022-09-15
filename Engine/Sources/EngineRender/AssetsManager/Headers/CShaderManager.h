#pragma once

#include "../../../../../Entry/EngineMain.h"

class CShaderManager
{
public:
	static const CShaderManager* const GetShaderManager() { return m_ShaderManager; }
	static void		Uninit() { delete m_ShaderManager; }
public:
	static std::shared_ptr<class CVertexShader>		LoadVertexShader(const std::string& name, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout = NULL);
	static std::shared_ptr<class CPixelShader>		LoadPixelShader(const std::string& name);
	static std::shared_ptr<class CComputeShader>	LoadComputeShader(const std::string& name);
public:
	static void		ClearVertexShaderData();
	static void		ClearPixelShaderData();
	static void		ClearComputeShaderData();
private:
	static void		AddVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> shader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout);
	static void		AddPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> shader);
	static void		AddComputeShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader);
	static void		FindVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout);
	static void		FindPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader);
	static void		FindComputeShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& shader);
private:
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11VertexShader>>	m_VertexShaderData;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11InputLayout>>	m_InputLayoutData;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>>	m_PixelShaderData;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11ComputeShader>>	m_ComputeShaderData;
private:
	CShaderManager() {}
	CShaderManager(const CShaderManager&) {}
	~CShaderManager() {}
private:
	static CShaderManager* m_ShaderManager;
};