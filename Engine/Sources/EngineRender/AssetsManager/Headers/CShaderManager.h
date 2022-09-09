#pragma once

#include "../../../../../Entry/EngineMain.h"

class CShader;

class CShaderManager
{
public:
	static const CShaderManager* const GetShaderManager() { return m_ShaderManager; }
	static void					Uninit() { delete m_ShaderManager; }
public:
	static void					ClearVertexShaderData();
	static void					ClearPixelShaderData();
	static CShader*				LoadShader(const std::string& vertexShaderName, const std::string& pixelShaderName);
private:
	static void					AddVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> shader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout);
	static void					AddPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> shader);
	static void					FindVertexShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& shader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout);
	static void					FindPixelShaderData(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& shader);
private:
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11VertexShader>>	m_VertexShaderData;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11InputLayout>>	m_InputLayoutData;
	std::map<std::string, Microsoft::WRL::ComPtr<ID3D11PixelShader>>	m_PixelShaderData;
private:
	CShaderManager() {}
	CShaderManager(const CShaderManager&) {}
	~CShaderManager() {}
private:
	static CShaderManager* m_ShaderManager;
};