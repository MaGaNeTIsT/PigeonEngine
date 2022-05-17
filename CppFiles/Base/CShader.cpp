#define _CRT_SECURE_NO_WARNINGS
#include "../../Entry/MyMain.h"
#include "../../Headers/Base/CShader.h"
#include "../../Headers/Base/CRenderDevice.h"

void CShader::Init(const CHAR* VertexShader, const CHAR* PixelShader)
{
	{
		FILE* file;
		ULONGLONG fsize;

		file = fopen(VertexShader, "rb");
		fsize = _filelength(_fileno(file));
		BYTE* buffer = new BYTE[fsize];
		fread(buffer, fsize, 1, file);
		fclose(file);

		HRESULT hr = CRenderDevice::GetDevice()->CreateVertexShader(static_cast<void*>(buffer), fsize, NULL, &m_VertexShader);
		if (FAILED(hr))
		{
			assert(file);
			delete[]buffer;
			return;
		}

		{
			D3D11_INPUT_ELEMENT_DESC layout[] =
			{
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }
			};
			UINT numElements = ARRAYSIZE(layout);

			hr = CRenderDevice::GetDevice()->CreateInputLayout(layout,
				numElements,
				static_cast<void*>(buffer),
				fsize,
				&m_VertexLayout);
			if (FAILED(hr))
			{
				assert(file);
				delete[]buffer;
				return;
			}
		}

		delete[]buffer;
	}

	{
		FILE* file;
		ULONGLONG fsize;

		file = fopen(PixelShader, "rb");
		fsize = _filelength(_fileno(file));
		BYTE* buffer = new BYTE[fsize];
		fread(buffer, fsize, 1, file);
		fclose(file);

		HRESULT hr = CRenderDevice::GetDevice()->CreatePixelShader(static_cast<void*>(buffer), fsize, NULL, &m_PixelShader);
		if (FAILED(hr))
		{
			assert(file);
			delete[]buffer;
			return;
		}

		delete[] buffer;
	}

	{
		D3D11_BUFFER_DESC hBufferDesc;
		hBufferDesc.Usage = D3D11_USAGE_DEFAULT;
		hBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		hBufferDesc.CPUAccessFlags = 0;
		hBufferDesc.MiscFlags = 0;
		hBufferDesc.StructureByteStride = sizeof(FLOAT);

		hBufferDesc.ByteWidth = sizeof(CONSTANT);
		CRenderDevice::GetDevice()->CreateBuffer(&hBufferDesc, NULL, &m_ConstantBuffer);

		hBufferDesc.ByteWidth = sizeof(LIGHT);
		CRenderDevice::GetDevice()->CreateBuffer(&hBufferDesc, NULL, &m_LightBuffer);
	}

	m_Light.Direction = XMFLOAT4( 1.f, -1.f, 1.f, 0.f );

	XMVECTOR direction;
	direction = XMLoadFloat4(&m_Light.Direction);
	direction = XMVector3Normalize(direction);

	DirectX::XMStoreFloat4(&m_Light.Direction, direction);

	m_Light.Color = COLOR(1.f, 1.f, 1.f, 1.f);
}

void CShader::Uninit()
{
	if (m_ConstantBuffer)
		m_ConstantBuffer->Release();
	if (m_LightBuffer)
		m_LightBuffer->Release();

	if (m_VertexLayout)
		m_VertexLayout->Release();
	if (m_VertexShader)
		m_VertexShader->Release();
	if (m_PixelShader)
		m_PixelShader->Release();
}

void CShader::Bind()
{
	CRenderDevice::GetDeviceContext()->VSSetShader(m_VertexShader, NULL, 0);
	CRenderDevice::GetDeviceContext()->PSSetShader(m_PixelShader, NULL, 0);

	CRenderDevice::GetDeviceContext()->IASetInputLayout(m_VertexLayout);

	CRenderDevice::GetDeviceContext()->UpdateSubresource(m_ConstantBuffer, 0, NULL, &m_Constant, 0, 0);
	CRenderDevice::GetDeviceContext()->UpdateSubresource(m_LightBuffer, 0, NULL, &m_Light, 0, 0);

	CRenderDevice::GetDeviceContext()->VSSetConstantBuffers(0, 1, &m_ConstantBuffer);
	CRenderDevice::GetDeviceContext()->VSSetConstantBuffers(1, 1, &m_LightBuffer);

	CRenderDevice::GetDeviceContext()->PSSetConstantBuffers(0, 1, &m_ConstantBuffer);
	CRenderDevice::GetDeviceContext()->PSSetConstantBuffers(1, 1, &m_LightBuffer);
}