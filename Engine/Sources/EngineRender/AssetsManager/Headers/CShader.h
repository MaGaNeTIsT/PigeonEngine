#pragma once

#include "../../../../../Entry/EngineMain.h"
#include "../../RenderBase/Headers/CStructCommon.h"

class CShader
{
public:
	virtual void Bind() = 0;
public:
	CShader(const std::string& name);
	virtual ~CShader();
protected:
	std::string	m_Name;
};

class CVertexShader : public CShader
{
public:
	virtual void Bind()override;
public:
	CVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> shader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout);
	virtual ~CVertexShader();
protected:
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_Shader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_InputLayout;
};

class CPixelShader : public CShader
{
public:
	virtual void Bind()override;
public:
	CPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> shader);
	virtual ~CPixelShader();
protected:
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_Shader;
};

class CComputeShader : public CShader
{
public:
	virtual void Bind()override;
public:
	CComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader);
	virtual ~CComputeShader();
protected:
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_Shader;
};