/*
#include "../Headers/CShader.h"
#include "../../RenderBase/Headers/CRenderDevice.h"



CShader::CShader(const std::string& name)
{
	this->m_Name = name;
}
CShader::~CShader()
{
}



CVertexShader::CVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> shader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout) : CShader(name)
{
	this->m_Shader		= shader;
	this->m_InputLayout	= inputLayout;
}
CVertexShader::~CVertexShader()
{
}
void CVertexShader::Bind()const
{
	CRenderDevice::SetVSShader(this->m_Shader);
	CRenderDevice::SetInputLayout(this->m_InputLayout);
}



CPixelShader::CPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> shader) : CShader(name)
{
	this->m_Shader = shader;
}
CPixelShader::~CPixelShader()
{
}
void CPixelShader::Bind()const
{
	CRenderDevice::SetPSShader(this->m_Shader);
}



CComputeShader::CComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader> shader) : CShader(name)
{
	this->m_Shader = shader;
}
CComputeShader::~CComputeShader()
{
}
void CComputeShader::Bind()const
{
	CRenderDevice::SetCSShader(this->m_Shader);
}
*/