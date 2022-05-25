#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CRenderDevice.h"

CShader::CShader()
{
	this->m_VertexShaderPath	= ENGINE_SHADER_NONE;
	this->m_PixelShaderPath		= ENGINE_SHADER_NONE;
	this->m_VertexShader		= nullptr;
	this->m_PixelShader			= nullptr;
	this->m_InputLayout			= nullptr;
	this->m_ConstantBuffer		= nullptr;
	this->m_ConstantBufferExtra	= nullptr;
}
CShader::~CShader()
{
}
void CShader::SetVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout)
{
	this->m_VertexShaderPath	= name;
	this->m_VertexShader		= vertexShader;
	this->m_InputLayout			= inputLayout;
}
void CShader::SetPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader)
{
	this->m_PixelShaderPath	= name;
	this->m_PixelShader		= pixelShader;
}
BOOL CShader::CreateConstantBuffer(const UINT& sizeData)
{
	return (CRenderDevice::CreateConstantBuffer(this->m_ConstantBuffer, sizeData));
}
BOOL CShader::CreateConstantBufferExtra(const UINT& sizeData)
{
	return (CRenderDevice::CreateConstantBuffer(this->m_ConstantBufferExtra, sizeData));
}
void CShader::UploadConstantBuffer(const void* ptrData)
{
	CRenderDevice::UploadConstantBuffer(this->m_ConstantBuffer, ptrData);
}
void CShader::UploadConstantBufferExtra(const void* ptrData)
{
	CRenderDevice::UploadConstantBuffer(this->m_ConstantBufferExtra, ptrData);
}
void CShader::BindConstantBuffer(const UINT& startSlot)
{
	CRenderDevice::BindConstantBuffer(this->m_ConstantBuffer, startSlot);
}
void CShader::BindConstantBufferExtra(const UINT& startSlot)
{
	CRenderDevice::BindConstantBuffer(this->m_ConstantBufferExtra, startSlot);
}
void CShader::BindShaderInputLayout()
{
	CRenderDevice::GetDeviceContext()->IASetInputLayout(this->m_InputLayout.Get());
	CRenderDevice::GetDeviceContext()->VSSetShader(this->m_VertexShader.Get(), NULL, 0u);
	CRenderDevice::GetDeviceContext()->PSSetShader(this->m_PixelShader.Get(), NULL, 0u);
}