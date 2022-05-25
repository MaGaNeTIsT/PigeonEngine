#pragma once

#include "../../Entry/MyMain.h"

class CShader
{
public:
	void	SetVertexShader(const string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader> vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout> inputLayout);
	void	SetPixelShader(const string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader> pixelShader);
	BOOL	CreateConstantBuffer(const UINT& sizeData);
	BOOL	CreateConstantBufferExtra(const UINT& sizeData);
	void	UploadConstantBuffer(const void* ptrData);
	void	UploadConstantBufferExtra(const void* ptrData);
	void	BindConstantBuffer(const UINT& startSlot);
	void	BindConstantBufferExtra(const UINT& startSlot);
	void	BindShaderInputLayout();
private:
	std::string									m_VertexShaderPath;
	std::string									m_PixelShaderPath;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_VertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_PixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_InputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_ConstantBufferExtra;
public:
	CShader();
	virtual ~CShader();
};