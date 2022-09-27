#include "../Headers/CRenderDevice.h"

CRenderDevice* CRenderDevice::m_RenderDevice = NULL;
void CRenderDevice::Initialize()
{
	if (CRenderDevice::m_RenderDevice == NULL)
	{
		CRenderDevice::m_RenderDevice = new CRenderDevice();
	}
}
void CRenderDevice::ShutDown()
{
	if (CRenderDevice::m_RenderDevice != NULL)
	{
		CRenderDevice::m_RenderDevice->Uninit();
		delete (CRenderDevice::m_RenderDevice);
	}
}
void CRenderDevice::Init(HWND hWnd, const CustomType::Vector2Int& bufferSize, const UINT& bufferDepth, const UINT& frameNum, const BOOL& windowed)
{
	if (bufferSize.X() < 1 || bufferSize.Y() < 1 || !(bufferDepth == 24u || bufferDepth == 32u) || frameNum < 2u)
	{
		//TODO Invalid Arguments log.
		return;
	}

	UINT bufferWidth = static_cast<UINT>(bufferSize.X());
	UINT bufferHeight = static_cast<UINT>(bufferSize.Y());
	HRESULT hr = S_FALSE;

	DXGI_SWAP_CHAIN_DESC sd;
	{
		::ZeroMemory(&sd, sizeof(sd));
		sd.BufferCount = 1u;
		sd.BufferDesc.Width = bufferWidth;
		sd.BufferDesc.Height = bufferHeight;
		sd.BufferDesc.Format = DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM;
		sd.BufferDesc.RefreshRate.Numerator = frameNum;
		sd.BufferDesc.RefreshRate.Denominator = 1u;
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.OutputWindow = hWnd;
		sd.SampleDesc.Count = 1u;
		sd.SampleDesc.Quality = 0u;
		sd.Windowed = windowed;
	}
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1, };
	hr = D3D11CreateDeviceAndSwapChain(
		NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		0u,
		featureLevels,
		7u,
		D3D11_SDK_VERSION,
		&sd,
		CRenderDevice::m_RenderDevice->m_SwapChain.ReleaseAndGetAddressOf(),
		CRenderDevice::m_RenderDevice->m_Device.ReleaseAndGetAddressOf(),
		&(CRenderDevice::m_RenderDevice->m_FeatureLevel),
		CRenderDevice::m_RenderDevice->m_ImmediateContext.ReleaseAndGetAddressOf());	//D3D11_CREATE_DEVICE_FLAG
	if (FAILED(hr) || !(CRenderDevice::m_RenderDevice->m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 || CRenderDevice::m_RenderDevice->m_FeatureLevel == D3D_FEATURE_LEVEL_11_0))
	{
		//TODO Create d3d device failed log.
		return;
	}

	CRenderDevice::m_RenderDevice->m_Viewport.Width = static_cast<FLOAT>(bufferWidth);
	CRenderDevice::m_RenderDevice->m_Viewport.Height = static_cast<FLOAT>(bufferHeight);
	CRenderDevice::m_RenderDevice->m_Viewport.MinDepth = 0.f;
	CRenderDevice::m_RenderDevice->m_Viewport.MaxDepth = 1.f;
	CRenderDevice::m_RenderDevice->m_Viewport.TopLeftX = 0.f;
	CRenderDevice::m_RenderDevice->m_Viewport.TopLeftY = 0.f;

	ID3D11Texture2D* pBackBuffer = NULL;
	hr = CRenderDevice::m_RenderDevice->m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)(&pBackBuffer));
	if (FAILED(hr))
	{
		//TODO Get buffer failed from swap chain log.
		return;
	}
	hr = CRenderDevice::m_RenderDevice->m_Device->CreateRenderTargetView(pBackBuffer, NULL, CRenderDevice::m_RenderDevice->m_RenderTargetView.ReleaseAndGetAddressOf());
	pBackBuffer->Release();
	if (FAILED(hr))
	{
		//TODO Create buffer RTV failed from swap chain log.
		return;
	}

	{
		D3D11_TEXTURE2D_DESC td;
		{
			::ZeroMemory(&td, sizeof(td));
			td.Width = sd.BufferDesc.Width;
			td.Height = sd.BufferDesc.Height;
			td.MipLevels = 1u;
			td.ArraySize = 1u;
			if (bufferDepth == 24u)
			{
				td.Format = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
			}
			else if (bufferDepth == 32u)
			{
				td.Format = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
			}
			td.SampleDesc = sd.SampleDesc;
			td.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
			td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
			td.CPUAccessFlags = 0u;
			td.MiscFlags = 0u;
		}
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateTexture2D(&td, NULL, CRenderDevice::m_RenderDevice->m_DepthTexture.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			//TODO Create depth buffer failed log.
			return;
		}
		D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
		{
			::ZeroMemory(&dsvd, sizeof(dsvd));
			if (bufferDepth == 24u)
			{
				dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
			}
			else if (bufferDepth == 32u)
			{
				dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
			}
			dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
			dsvd.Flags = 0u;	//D3D11_DSV_FLAG
		}
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateDepthStencilView(CRenderDevice::m_RenderDevice->m_DepthTexture.Get(), &dsvd, CRenderDevice::m_RenderDevice->m_DepthStencilView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			//TODO Create depth stencil view failed log.
			return;
		}
	}
}
void CRenderDevice::Uninit()
{
	if (CRenderDevice::m_RenderDevice->m_RenderTargetView)
	{
		CRenderDevice::m_RenderDevice->m_RenderTargetView->Release();
		CRenderDevice::m_RenderDevice->m_RenderTargetView = nullptr;
	}
	if (CRenderDevice::m_RenderDevice->m_DepthStencilView)
	{
		CRenderDevice::m_RenderDevice->m_DepthStencilView->Release();
		CRenderDevice::m_RenderDevice->m_DepthStencilView = nullptr;
	}
	if (CRenderDevice::m_RenderDevice->m_SwapChain)
	{
		CRenderDevice::m_RenderDevice->m_SwapChain->Release();
		CRenderDevice::m_RenderDevice->m_SwapChain = nullptr;
	}
	if (CRenderDevice::m_RenderDevice->m_ImmediateContext)
	{
		CRenderDevice::m_RenderDevice->m_ImmediateContext->Release();
		CRenderDevice::m_RenderDevice->m_ImmediateContext = nullptr;
	}
	if (CRenderDevice::m_RenderDevice->m_Device)
	{
		CRenderDevice::m_RenderDevice->m_Device->Release();
		CRenderDevice::m_RenderDevice->m_Device = nullptr;
	}
}
BOOL CRenderDevice::LoadVertexShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11VertexShader>& vertexShader, Microsoft::WRL::ComPtr<ID3D11InputLayout>& inputLayout, const std::vector<D3D11_INPUT_ELEMENT_DESC>* layout)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateVertexShader(static_cast<void*>(buffer), fsize, NULL, vertexShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create vertex shader object failed log.
			return FALSE;
		}
	}

	{
		UINT numElements = 5u;
		std::vector<D3D11_INPUT_ELEMENT_DESC> tempLayout;
		if (layout == NULL)
		{
			tempLayout.resize(5u);
			tempLayout[0] = { "POSITION", 0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[1] = { "NORMAL",   0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[2] = { "TANGENT",  0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[3] = { "COLOR",    0u, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT, 0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
			tempLayout[4] = { "TEXCOORD", 0u, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT,       0u, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA, 0u };
		}
		else
		{
			numElements = static_cast<UINT>(layout->size());
			tempLayout.resize(layout->size());
			for (UINT i = 0u; i < numElements; i++)
				tempLayout[i] = (*layout)[i];
		}
		
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateInputLayout(tempLayout.data(), numElements, static_cast<void*>(buffer), fsize, inputLayout.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create input layout object failed log.
			return FALSE;
		}
	}

	delete[]buffer;
	return TRUE;
}
BOOL CRenderDevice::LoadPixelShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11PixelShader>& pixelShader)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreatePixelShader(static_cast<void*>(buffer), fsize, NULL, pixelShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create pixel shader object failed log.
			return FALSE;
		}
	}

	delete[] buffer;
	return TRUE;
}
BOOL CRenderDevice::LoadComputeShader(const std::string& name, Microsoft::WRL::ComPtr<ID3D11ComputeShader>& computeShader)
{
	FILE* file;
	LONG fsize;
	BYTE* buffer;
	{
		fopen_s(&file, name.c_str(), "rb");
		if (file == NULL)
		{
			//TODO Can not read file log.
			return FALSE;
		}
		fsize = _filelength(_fileno(file));
		buffer = new BYTE[fsize];
		fread_s(buffer, fsize, fsize, 1u, file);
		fclose(file);
	}

	{
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateComputeShader(static_cast<void*>(buffer), fsize, NULL, computeShader.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			delete[]buffer;
			//TODO Create compute shader object failed log.
			return FALSE;
		}
	}

	delete[] buffer;
	return TRUE;
}
BOOL CRenderDevice::CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const CustomStruct::CRenderBufferDesc& bufferDesc, const CustomStruct::CRenderSubresourceData* subData)
{
	D3D11_BUFFER_DESC bd;
	{
		::ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = bufferDesc.ByteWidth;
		CRenderDevice::TranslateUsage(bd.Usage, bufferDesc.Usage);
		CRenderDevice::TranslateBindFlag(bd.BindFlags, bufferDesc.BindFlags);
		CRenderDevice::TranslateCPUAccessFlag(bd.CPUAccessFlags, bufferDesc.CPUAccessFlags);
		CRenderDevice::TranslateResourceMiscFlag(bd.MiscFlags, bufferDesc.MiscFlags);
		bd.StructureByteStride = bufferDesc.StructureByteStride;
	}
	HRESULT hr = S_FALSE;
	if (subData)
	{
		D3D11_SUBRESOURCE_DATA sd;
		{
			::ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = subData->pSysMem;
			sd.SysMemPitch = subData->SysMemPitch;
			sd.SysMemSlicePitch = subData->SysMemSlicePitch;
		}
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, &sd, buffer.ReleaseAndGetAddressOf());
	}
	else
	{
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, NULL, buffer.ReleaseAndGetAddressOf());
	}
	if (FAILED(hr))
	{
		//TODO Create buffer object failed log.
		return FALSE;
	}
	return TRUE;
}
void CRenderDevice::UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT srcRowPitch, UINT srcDepthPitch, UINT dstSubresource, const D3D11_BOX* dstBox)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
}
BOOL CRenderDevice::CreateStructuredBuffer(StructuredBufferInfo& output, const CustomStruct::CRenderStructuredBufferDesc& structuredBufferDesc, const CustomStruct::CRenderSubresourceData* subData)
{
	if (structuredBufferDesc.NumElements < 1u || structuredBufferDesc.FirstElement >= structuredBufferDesc.NumElements || structuredBufferDesc.StructureSize < 4u)
	{
		//TODO Error desc log.
		return FALSE;
	}

	{
		output.AccessMapRead = structuredBufferDesc.AccessFlag == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ || structuredBufferDesc.AccessFlag == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ_WRITE;
		output.AccessMapWrite = structuredBufferDesc.AccessFlag == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_WRITE || structuredBufferDesc.AccessFlag == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ_WRITE;
		if (structuredBufferDesc.GPUWritable && output.AccessMapWrite)
		{
			//TODO Error access ability log.
			return FALSE;
		}
		D3D11_BUFFER_DESC bd;
		::ZeroMemory(&bd, sizeof(bd));
		bd.ByteWidth = structuredBufferDesc.StructureSize * structuredBufferDesc.NumElements;
		bd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		if (!(structuredBufferDesc.GPUWritable) && !(output.AccessMapWrite))
		{
			if (structuredBufferDesc.AccessFlag == CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_NONE)
			{
				bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
			}
			else
			{
				bd.Usage = D3D11_USAGE::D3D11_USAGE_IMMUTABLE;
			}
		}
		else if (!(structuredBufferDesc.GPUWritable) && output.AccessMapWrite)
		{
			bd.Usage = D3D11_USAGE::D3D11_USAGE_DYNAMIC;
		}
		else if (structuredBufferDesc.GPUWritable && !(output.AccessMapWrite))
		{
			bd.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
			bd.BindFlags = bd.BindFlags | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS;
		}
		TranslateCPUAccessFlag(bd.CPUAccessFlags, structuredBufferDesc.AccessFlag);
		bd.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
		bd.StructureByteStride = structuredBufferDesc.StructureSize;
		HRESULT hr = S_FALSE;
		if (subData)
		{
			D3D11_SUBRESOURCE_DATA sd;
			::ZeroMemory(&sd, sizeof(sd));
			sd.pSysMem = subData->pSysMem;
			sd.SysMemPitch = subData->SysMemPitch;
			sd.SysMemSlicePitch = subData->SysMemSlicePitch;
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, &sd, output.Buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateBuffer(&bd, NULL, output.Buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			//TODO Create structured buffer failed log.
			return FALSE;
		}
	}

	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		::ZeroMemory(&srvd, sizeof(srvd));
		srvd.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_BUFFER;
		srvd.Buffer.FirstElement = structuredBufferDesc.FirstElement;
		srvd.Buffer.NumElements = structuredBufferDesc.NumElements;
		HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			//TODO Create structured buffer SRV failed log.
			return FALSE;
		}
	}

	{
		if (structuredBufferDesc.GPUWritable)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
			::ZeroMemory(&uavd, sizeof(uavd));
			uavd.Format = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
			uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_BUFFER;
			uavd.Buffer.FirstElement = structuredBufferDesc.FirstElement;
			uavd.Buffer.NumElements = structuredBufferDesc.NumElements;
			CRenderDevice::TranslateBufferUAVFlag(uavd.Buffer.Flags, structuredBufferDesc.BufferFlag);
			HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO Create structured buffer UAV failed log.
				return FALSE;
			}
		}
	}

	return TRUE;
}
BOOL CRenderDevice::CreateRenderTexture2D(RenderTexture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc)
{
	BOOL isDepthFormat = textureDesc.Depth == 16u || textureDesc.Depth == 24u || textureDesc.Depth == 32u;
	if (!isDepthFormat && textureDesc.Depth != 0u)
	{
		//TODO Create depth texture failed log.
		return FALSE;
	}
	D3D11_TEXTURE2D_DESC td;
	{
		UINT tBindFlags = 0u;
		DXGI_FORMAT tFormat = DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		if (isDepthFormat)
		{
			if (textureDesc.Depth == 16u)
			{
				tFormat = DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS;
			}
			else if (textureDesc.Depth == 24u)
			{
				tFormat = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS;
			}
			else if (textureDesc.Depth == 32u)
			{
				tFormat = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS;
			}
			tBindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		}
		else
		{
			CRenderDevice::TranslateResourceFormat(tFormat, textureDesc.TextureFormat);
			CRenderDevice::TranslateBindFlag(tBindFlags, textureDesc.BindFlags);
		}

		::ZeroMemory(&td, sizeof(td));
		td.Width = textureDesc.Width;
		td.Height = textureDesc.Height;
		td.MipLevels = textureDesc.MipLevels;
		td.ArraySize = textureDesc.ArraySize;
		td.Format = tFormat;
		td.SampleDesc.Count = textureDesc.SampleDesc.Count;
		td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
		td.BindFlags = tBindFlags;
		CRenderDevice::TranslateUsage(td.Usage, textureDesc.Usage);
		CRenderDevice::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
		CRenderDevice::TranslateResourceMiscFlag(td.MiscFlags, textureDesc.MiscFlags);
	}
	output.Release();
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateTexture2D(&td, NULL, output.Texture2D.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create texture object failed log.
		return FALSE;
	}

	{
		BOOL needSRV = textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_SHADER_RESOURCE
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RTV_SRV
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RTV_SRV_UAV
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_SRV_UAV;
		if (needSRV || isDepthFormat)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			{
				::ZeroMemory(&srvd, sizeof(srvd));
				if (isDepthFormat)
				{
					if (textureDesc.Depth == 16u)
					{
						srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
					}
					else if (textureDesc.Depth == 24u)
					{
						srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
					}
					else if (textureDesc.Depth == 32u)
					{
						srvd.Format = DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT_X8X24_TYPELESS;
					}
				}
				else
				{
					CRenderDevice::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
				}
				srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
				srvd.Texture2D.MostDetailedMip = 0u;
				srvd.Texture2D.MipLevels = td.MipLevels;
			}
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateShaderResourceView(output.Texture2D.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO Create SRV object failed log.
				return FALSE;
			}
		}
	}

	{
		BOOL needUAV = textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_UNORDERED_ACCESS
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_SRV_UAV
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RTV_SRV_UAV;
		if (needUAV && !isDepthFormat)
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
			::ZeroMemory(&uavd, sizeof(uavd));
			CRenderDevice::TranslateResourceFormat(uavd.Format, textureDesc.UAVFormat);
			uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D;
			uavd.Texture2D.MipSlice = 0u;
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateUnorderedAccessView(output.Texture2D.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO Create UAV object failed log.
				return FALSE;
			}
		}
	}

	{
		BOOL needRTV = textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RENDER_TARGET
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RTV_SRV
			|| textureDesc.BindFlags == CustomStruct::CRenderBindFlag::BIND_RTV_SRV_UAV;
		if (needRTV && !isDepthFormat)
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtvd;
			{
				::ZeroMemory(&rtvd, sizeof(rtvd));
				CRenderDevice::TranslateResourceFormat(rtvd.Format, textureDesc.RTVFormat);
				rtvd.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvd.Texture2D.MipSlice = 0u;
			}
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateRenderTargetView(output.Texture2D.Get(), &rtvd, output.RenderTargetView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO Create RTV object failed log.
				return FALSE;
			}
		}
	}

	{
		if (isDepthFormat)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
			{
				::ZeroMemory(&dsvd, sizeof(dsvd));
				if (textureDesc.Depth == 16u)
				{
					dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D16_UNORM;
				}
				else if (textureDesc.Depth == 24u)
				{
					dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT;
				}
				else if (textureDesc.Depth == 32u)
				{
					dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				}
				dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Flags = 0u;	//D3D11_DSV_FLAG
				dsvd.Texture2D.MipSlice = 0u;
			}
			hr = CRenderDevice::m_RenderDevice->m_Device->CreateDepthStencilView(output.Texture2D.Get(), &dsvd, output.DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				//TODO Create DSV object failed log.
				return FALSE;
			}
		}
	}

	return TRUE;
}
BOOL CRenderDevice::CreateTexture2D(Texture2DViewInfo& output, const CustomStruct::CRenderTextureDesc& textureDesc, const CustomStruct::CRenderSubresourceData* subData)
{
	D3D11_TEXTURE2D_DESC td;
	{
		::ZeroMemory(&td, sizeof(td));
		td.Width = textureDesc.Width;
		td.Height = textureDesc.Height;
		td.MipLevels = textureDesc.MipLevels;
		td.ArraySize = textureDesc.ArraySize;
		CRenderDevice::TranslateResourceFormat(td.Format, textureDesc.TextureFormat);
		td.SampleDesc.Count = textureDesc.SampleDesc.Count;
		td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
		td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
		CRenderDevice::TranslateUsage(td.Usage, textureDesc.Usage);
		CRenderDevice::TranslateCPUAccessFlag(td.CPUAccessFlags, textureDesc.CPUAccessFlags);
		CRenderDevice::TranslateResourceMiscFlag(td.MiscFlags, textureDesc.MiscFlags);
	}
	HRESULT hr = S_FALSE;
	if (subData)
	{
		D3D11_SUBRESOURCE_DATA subresourceData;
		subresourceData.pSysMem = subData->pSysMem;
		subresourceData.SysMemPitch = subData->SysMemPitch;
		subresourceData.SysMemSlicePitch = subData->SysMemSlicePitch;

		hr = CRenderDevice::m_RenderDevice->m_Device->CreateTexture2D(&td, &subresourceData, output.Texture2D.ReleaseAndGetAddressOf());
	}
	else
	{
		hr = CRenderDevice::m_RenderDevice->m_Device->CreateTexture2D(&td, NULL, output.Texture2D.ReleaseAndGetAddressOf());
	}
	if (FAILED(hr))
	{
		//TODO Create texture object failed log.
		return FALSE;
	}
	D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
	{
		::ZeroMemory(&srvd, sizeof(srvd));
		CRenderDevice::TranslateResourceFormat(srvd.Format, textureDesc.SRVFormat);
		srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
		srvd.Texture2D.MostDetailedMip = 0u;
		srvd.Texture2D.MipLevels = td.MipLevels;
	}
	hr = CRenderDevice::m_RenderDevice->m_Device->CreateShaderResourceView(output.Texture2D.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create SRV object failed log.
		return FALSE;
	}
	return TRUE;
}
void CRenderDevice::Present(const UINT& syncInterval)
{
	HRESULT hr = CRenderDevice::m_RenderDevice->m_SwapChain->Present(syncInterval, 0u);	//DXGI_PRESENT
}
void CRenderDevice::SetDefaultDepthStencilState()
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetDepthStencilState(NULL, 0x0u);
}
void CRenderDevice::SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT& stencilRef)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetDepthStencilState(dss.Get(), stencilRef);
}
void CRenderDevice::SetDefaultBlendState()
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetBlendState(NULL, NULL, 0xffffffff);
}
void CRenderDevice::SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const CustomStruct::CColor& blendFactor, const UINT& sampleMask)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetBlendState(bs.Get(), blendFactor.rgba, sampleMask);
}
void CRenderDevice::SetNoRenderTarget()
{
	ID3D11RenderTargetView* rtvs[] = { NULL };
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(1u, rtvs, NULL);
}
void CRenderDevice::SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(1u, rtv.GetAddressOf(), NULL);
}
void CRenderDevice::SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv)
{
	ID3D11RenderTargetView* rtvs[] = { NULL };
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(1u, rtvs, dsv.Get());
}
void CRenderDevice::SetRenderTarget(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(1u, rtv.GetAddressOf(), dsv.Get());
}
void CRenderDevice::SetRenderTargets(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* rtv, const UINT& rtvNum)
{
	std::vector<ID3D11RenderTargetView*> rtvs(rtvNum);
	for (UINT i = 0u; i < rtvNum; i++)
		rtvs[i] = rtv[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(rtvNum, rtvs.data(), NULL);
}
void CRenderDevice::SetRenderTargets(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>* rtv, const UINT& rtvNum, const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv)
{
	std::vector<ID3D11RenderTargetView*> rtvs(rtvNum);
	for (UINT i = 0u; i < rtvNum; i++)
		rtvs[i] = rtv[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->OMSetRenderTargets(rtvNum, rtvs.data(), dsv.Get());
}
void CRenderDevice::SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->RSSetState(rs.Get());
}
void CRenderDevice::SetViewport(const D3D11_VIEWPORT& viewport)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->RSSetViewports(1u, &viewport);
}
void CRenderDevice::SetViewports(std::vector<D3D11_VIEWPORT> viewports)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->RSSetViewports(static_cast<UINT>(viewports.size()), viewports.data());
}
void CRenderDevice::SetNoVSShader()
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetShader(NULL, NULL, 0u);
}
void CRenderDevice::SetNoPSShader()
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetShader(NULL, NULL, 0u);
}
void CRenderDevice::SetNoCSShader()
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetShader(NULL, NULL, 0u);
}
void CRenderDevice::SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetShader(vs.Get(), NULL, 0u);
}
void CRenderDevice::SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetShader(ps.Get(), NULL, 0u);
}
void CRenderDevice::SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetShader(cs.Get(), NULL, 0u);
}
void CRenderDevice::BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
void CRenderDevice::BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum)
{
	std::vector<ID3D11SamplerState*> sampler(ssNum);
	for (UINT i = 0u; i < ssNum; i++)
		sampler[i] = ss[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetSamplers(startSlot, ssNum, sampler.data());
}
void CRenderDevice::BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
void CRenderDevice::BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum)
{
	std::vector<ID3D11SamplerState*> sampler(ssNum);
	for (UINT i = 0u; i < ssNum; i++)
		sampler[i] = ss[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetSamplers(startSlot, ssNum, sampler.data());
}
void CRenderDevice::BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetSamplers(startSlot, 1u, ss.GetAddressOf());
}
void CRenderDevice::BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT& startSlot, const UINT& ssNum)
{
	std::vector<ID3D11SamplerState*> sampler(ssNum);
	for (UINT i = 0u; i < ssNum; i++)
		sampler[i] = ss[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetSamplers(startSlot, ssNum, sampler.data());
}
void CRenderDevice::BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
}
void CRenderDevice::BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum)
{
	std::vector<ID3D11Buffer*> buffers(bufferNum);
	for (UINT i = 0u; i < bufferNum; i++)
		buffers[i] = buffer[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetConstantBuffers(startSlot, bufferNum, buffers.data());
}
void CRenderDevice::BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
}
void CRenderDevice::BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum)
{
	std::vector<ID3D11Buffer*> buffers(bufferNum);
	for (UINT i = 0u; i < bufferNum; i++)
		buffers[i] = buffer[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetConstantBuffers(startSlot, bufferNum, buffers.data());
}
void CRenderDevice::BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
}
void CRenderDevice::BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT& startSlot, const UINT& bufferNum)
{
	std::vector<ID3D11Buffer*> buffers(bufferNum);
	for (UINT i = 0u; i < bufferNum; i++)
		buffers[i] = buffer[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetConstantBuffers(startSlot, bufferNum, buffers.data());
}
void CRenderDevice::BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
}
void CRenderDevice::BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum)
{
	std::vector<ID3D11ShaderResourceView*> srvs(srvNum);
	for (UINT i = 0u; i < srvNum; i++)
		srvs[i] = srv[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->VSSetShaderResources(startSlot, srvNum, srvs.data());
}
void CRenderDevice::BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
}
void CRenderDevice::BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum)
{
	std::vector<ID3D11ShaderResourceView*> srvs(srvNum);
	for (UINT i = 0u; i < srvNum; i++)
		srvs[i] = srv[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->PSSetShaderResources(startSlot, srvNum, srvs.data());
}
void CRenderDevice::BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
}
void CRenderDevice::BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT& startSlot, const UINT& srvNum)
{
	std::vector<ID3D11ShaderResourceView*> srvs(srvNum);
	for (UINT i = 0u; i < srvNum; i++)
		srvs[i] = srv[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetShaderResources(startSlot, srvNum, srvs.data());
}
void CRenderDevice::BindNoCSUnorderedAccessView(const UINT& startSlot)
{
	ID3D11UnorderedAccessView* uavs[] = { NULL };
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uavs, NULL);
}
void CRenderDevice::BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT& startSlot)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uav.GetAddressOf(), NULL);
}
void CRenderDevice::BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT& startSlot, const UINT& uavNum)
{
	std::vector<ID3D11UnorderedAccessView*> uavs(uavNum);
	for (UINT i = 0u; i < uavNum; i++)
		uavs[i] = uav[i].Get();

	CRenderDevice::m_RenderDevice->m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, uavNum, uavs.data(), NULL);
}
void CRenderDevice::ClearRenderTargetView(const Microsoft::WRL::ComPtr<ID3D11RenderTargetView>& rtv, const CustomStruct::CColor& clearColor)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->ClearRenderTargetView(rtv.Get(), clearColor.rgba);
}
void CRenderDevice::ClearDepthStencilView(const Microsoft::WRL::ComPtr<ID3D11DepthStencilView>& dsv, CustomStruct::CRenderClearDepthStencilFlag flag, const FLOAT& depth, const UINT& stencil)
{
	UINT clearFlag = 0u;
	CRenderDevice::TranslateClearDepthStencilFlag(clearFlag, flag);
	UINT8 clearStencil = static_cast<UINT8>(CustomType::CMath::Min(stencil, 0xffu));
	FLOAT clearDepth = CustomType::CMath::Clamp(depth, 0.f, 1.f);
	CRenderDevice::m_RenderDevice->m_ImmediateContext->ClearDepthStencilView(dsv.Get(), clearFlag, clearDepth, clearStencil);
}
void CRenderDevice::ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const CustomStruct::CColor& clearValue)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->ClearUnorderedAccessViewFloat(uav.Get(), clearValue.rgba);
}
void CRenderDevice::ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const CustomType::Vector4Int& clearValue)
{
	UINT values[4] = { static_cast<UINT>(clearValue.X()), static_cast<UINT>(clearValue.Y()), static_cast<UINT>(clearValue.Z()), static_cast<UINT>(clearValue.W()) };
	CRenderDevice::m_RenderDevice->m_ImmediateContext->ClearUnorderedAccessViewUint(uav.Get(), values);
}
void CRenderDevice::SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride, const UINT& offset)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetInputLayout(layout.Get());
	//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetVertexBuffers(0u, 1u, vb.GetAddressOf(), &stride, &offset);
}
void CRenderDevice::SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetInputLayout(layout.Get());
}
void CRenderDevice::SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT& stride, const UINT& offset)
{
	//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetVertexBuffers(0u, 1u, vb.GetAddressOf(), &stride, &offset);
}
void CRenderDevice::SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& ib, const UINT& offset, CustomStruct::CRenderFormat format)
{
	DXGI_FORMAT indexFormat = DXGI_FORMAT_R32_UINT;
	CRenderDevice::TranslateResourceFormat(indexFormat, format);
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetIndexBuffer(ib.Get(), indexFormat, offset);
}
void CRenderDevice::SetPrimitiveTopology(CustomStruct::CRenderPrimitiveTopology topology)
{
	D3D_PRIMITIVE_TOPOLOGY output = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	TranslatePrimitiveTopology(output, topology);
	CRenderDevice::m_RenderDevice->m_ImmediateContext->IASetPrimitiveTopology(output);
}
void CRenderDevice::Draw(const UINT& vertexCount, const UINT& startVertexLocation)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->Draw(vertexCount, startVertexLocation);
}
void CRenderDevice::DrawIndexed(const UINT& indexCount, const UINT& startIndexLocation, const INT& baseVertexLocation)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
}
void CRenderDevice::Dispatch(const UINT& x, const UINT& y, const UINT& z)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->Dispatch(x, y, z);
}
void CRenderDevice::DispatchIndirect(const Microsoft::WRL::ComPtr<ID3D11Buffer>& arg, const UINT& alignedByteOffsetForArgs)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->DispatchIndirect(arg.Get(), alignedByteOffsetForArgs);
}
BOOL CRenderDevice::CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const CustomStruct::CRenderDepthState& depthState, const CustomStruct::CRenderStencilState* stencilState)
{
	D3D11_DEPTH_STENCIL_DESC dsd;
	{
		::ZeroMemory(&dsd, sizeof(dsd));
		CRenderDevice::TranslateDepthStencilState(dsd, depthState, stencilState);
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateDepthStencilState(&dsd, dss.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create depth stencil state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const std::vector<CustomStruct::CRenderBlendState>& blendStates)
{
	if (blendStates.size() < 1)
	{
		//TODO Blend state info error log.
		return FALSE;
	}
	D3D11_BLEND_DESC bd;
	{
		::ZeroMemory(&bd, sizeof(bd));
		bd.AlphaToCoverageEnable = FALSE;
		bd.IndependentBlendEnable = FALSE;
		if (blendStates.size() > 1)
		{
			bd.IndependentBlendEnable = TRUE;
		}
		for (INT i = 0; i < (blendStates.size()) && i < 8; i++)
		{
			CRenderDevice::TranslateBlendState(bd.RenderTarget[i], blendStates[i]);
		}
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateBlendState(&bd, bs.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create blend state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const CustomStruct::CRenderRasterizerState& rasterizerState)
{
	D3D11_RASTERIZER_DESC rd;
	{
		::ZeroMemory(&rd, sizeof(rd));
		CRenderDevice::TranslateRasterizerState(rd.CullMode, rd.FillMode, rasterizerState);
		rd.FrontCounterClockwise = FALSE;
		rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
		rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
		rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		rd.DepthClipEnable = TRUE;
		rd.ScissorEnable = FALSE;
		rd.MultisampleEnable = FALSE;
		rd.AntialiasedLineEnable = FALSE;
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateRasterizerState(&rd, rs.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create rasterizer state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const CustomStruct::CRenderSamplerState& samplerState)
{
	D3D11_SAMPLER_DESC sd;
	{
		::ZeroMemory(&sd, sizeof(sd));
		CRenderDevice::TranslateSamplerState(sd, samplerState);
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateSamplerState(&sd, ss.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create sampler state object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::CreateQuery(Microsoft::WRL::ComPtr<ID3D11Query>& q, const CustomStruct::CRenderQueryDesc& queryDesc)
{
	D3D11_QUERY_DESC qd;
	{
		::ZeroMemory(&qd, sizeof(qd));
		CRenderDevice::TranslateQueryDesc(qd, queryDesc);
	}
	HRESULT hr = CRenderDevice::m_RenderDevice->m_Device->CreateQuery(&qd, q.ReleaseAndGetAddressOf());
	if (FAILED(hr))
	{
		//TODO Create query object failed log.
		return FALSE;
	}
	return TRUE;
}
BOOL CRenderDevice::GetData(ID3D11Asynchronous* pAsync, void* output, const UINT& size, CustomStruct::CRenderAsyncGetDataFlag flag)
{
	UINT getDataFlag = 0u;
	CRenderDevice::TranslateGetDataFlag(getDataFlag, flag);
	HRESULT hr = CRenderDevice::m_RenderDevice->m_ImmediateContext->GetData(pAsync, output, size, getDataFlag);
	return (!(FAILED(hr)));
}
void CRenderDevice::Begin(ID3D11Asynchronous* pAsync)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->Begin(pAsync);
}
void CRenderDevice::End(ID3D11Asynchronous* pAsync)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->End(pAsync);
}
BOOL CRenderDevice::Map(const StructuredBufferInfo& input, const UINT& indexSubResource, CustomStruct::CRenderMapType mapType, CustomStruct::CRenderMapFlag mapFlag, CustomStruct::CRenderMappedResource& output)
{
	if (!(input.AccessMapRead || input.AccessMapWrite))
	{
		//TODO Error resource type log.
		return FALSE;
	}
	D3D11_MAPPED_SUBRESOURCE ms;
	{
		D3D11_MAP d3dMapType;
		{
			BOOL needRead = mapType == CustomStruct::CRenderMapType::MAP_READ || mapType == CustomStruct::CRenderMapType::MAP_READ_WRITE;
			BOOL needWrite = mapType == CustomStruct::CRenderMapType::MAP_WRITE || mapType == CustomStruct::CRenderMapType::MAP_READ_WRITE || mapType == CustomStruct::CRenderMapType::MAP_WRITE_DISCARD || mapType == CustomStruct::CRenderMapType::MAP_WRITE_NO_OVERWRITE;
			if (input.AccessMapRead != needRead || input.AccessMapWrite != needWrite)
			{
				//TODO Error map type log.
				return FALSE;
			}
		}
		UINT d3dMapFlag;
		::ZeroMemory(&ms, sizeof(ms));
		CRenderDevice::TranslateMapType(d3dMapType, mapType);
		CRenderDevice::TranslateMapFlag(d3dMapFlag, mapFlag);
		HRESULT hr = CRenderDevice::m_RenderDevice->m_ImmediateContext->Map(
			input.Buffer.Get(),
			indexSubResource,
			d3dMapType,
			d3dMapFlag,
			&ms);
		if (hr != S_OK)
		{
			//TODO Access failed log.
			return FALSE;
		}
	}
	output.pData = ms.pData;
	output.RowPitch = ms.RowPitch;
	output.DepthPitch = ms.DepthPitch;
	return TRUE;
}
void CRenderDevice::Unmap(const StructuredBufferInfo& input, const UINT& indexSubResource)
{
	CRenderDevice::m_RenderDevice->m_ImmediateContext->Unmap(input.Buffer.Get(), indexSubResource);
}
void CRenderDevice::TranslateBindFlag(UINT& output, CustomStruct::CRenderBindFlag input)
{
	static std::map<CustomStruct::CRenderBindFlag, UINT> bindFlagMap = {
		{ CustomStruct::CRenderBindFlag::BIND_NONE, 0u },
		{ CustomStruct::CRenderBindFlag::BIND_VERTEX_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_INDEX_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_CONSTANT_BUFFER, D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER },
		{ CustomStruct::CRenderBindFlag::BIND_SHADER_RESOURCE, D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE },
		{ CustomStruct::CRenderBindFlag::BIND_RENDER_TARGET, D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET },
		{ CustomStruct::CRenderBindFlag::BIND_DEPTH_STENCIL, D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL },
		{ CustomStruct::CRenderBindFlag::BIND_UNORDERED_ACCESS, D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS },
		{ CustomStruct::CRenderBindFlag::BIND_SRV_UAV, (D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS) },
		{ CustomStruct::CRenderBindFlag::BIND_RTV_SRV, (D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE) },
		{ CustomStruct::CRenderBindFlag::BIND_RTV_SRV_UAV, (D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET | D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS) } };

	output = bindFlagMap[input];
}
void CRenderDevice::TranslateUsage(D3D11_USAGE& output, CustomStruct::CRenderUsage input)
{
	static std::map<CustomStruct::CRenderUsage, D3D11_USAGE> usageMap = {
		{ CustomStruct::CRenderUsage::USAGE_DEFAULT, D3D11_USAGE::D3D11_USAGE_DEFAULT },
		{ CustomStruct::CRenderUsage::USAGE_IMMUTABLE, D3D11_USAGE::D3D11_USAGE_IMMUTABLE },
		{ CustomStruct::CRenderUsage::USAGE_DYNAMIC, D3D11_USAGE::D3D11_USAGE_DYNAMIC },
		{ CustomStruct::CRenderUsage::USAGE_STAGING, D3D11_USAGE::D3D11_USAGE_STAGING } };

	output = usageMap[input];
}
void CRenderDevice::TranslateCPUAccessFlag(UINT& output, CustomStruct::CRenderCPUAccessFlag input)
{
	static std::map<CustomStruct::CRenderCPUAccessFlag, UINT> cpuAccessFlagMap = {
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_NONE, 0u },
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_WRITE, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE },
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ, D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ },
		{ CustomStruct::CRenderCPUAccessFlag::CPU_ACCESS_READ_WRITE, (D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ | D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE) } };

	output = cpuAccessFlagMap[input];
}
void CRenderDevice::TranslateResourceMiscFlag(UINT& output, CustomStruct::CRenderResourceMiscFlag input)
{
	static std::map<CustomStruct::CRenderResourceMiscFlag, UINT> resourceMiscFlagMap = {
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_NONE, 0u },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_GENERATE_MIPS, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_TEXTURECUBE, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_DRAWINDIRECT_ARGS, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_BUFFER_STRUCTURED, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED },
		{ CustomStruct::CRenderResourceMiscFlag::RESOURCE_MISC_RESOURCE_CLAMP, D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_RESOURCE_CLAMP } };

	output = resourceMiscFlagMap[input];
}
void CRenderDevice::TranslateResourceFormat(DXGI_FORMAT& output, CustomStruct::CRenderFormat input)
{
	static std::map<CustomStruct::CRenderFormat, DXGI_FORMAT> formatMap = {
		{ CustomStruct::CRenderFormat::FORMAT_UNKNOWN						, DXGI_FORMAT::DXGI_FORMAT_UNKNOWN						},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_TYPELESS		},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_UINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32A32_SINT 			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_TYPELESS			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_UINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32B32_SINT				, DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT				},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_TYPELESS		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_FLOAT			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_UNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_UINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_SNORM			, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SNORM			},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16B16A16_SINT				, DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT			},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R32G32_TYPELESS				},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT					},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_UINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT					},
		{ CustomStruct::CRenderFormat::FORMAT_R32G32_SINT					, DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT					},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_TYPELESS			, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_TYPELESS			},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UNORM     		},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10A2_UINT				, DXGI_FORMAT::DXGI_FORMAT_R10G10B10A2_UINT      		},
		{ CustomStruct::CRenderFormat::FORMAT_R11G11B10_FLOAT				, DXGI_FORMAT::DXGI_FORMAT_R11G11B10_FLOAT       		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_TYPELESS     		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM        		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UNORM_SRGB			, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB   		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_UINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT         		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_SNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SNORM        		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8B8A8_SINT					, DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT         		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_R16G16_TYPELESS       		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_FLOAT					, DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT           		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SNORM          		},
		{ CustomStruct::CRenderFormat::FORMAT_R16G16_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT           		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R32_TYPELESS          		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT             		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_UINT              		},
		{ CustomStruct::CRenderFormat::FORMAT_R32_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R32_SINT              		},
		{ CustomStruct::CRenderFormat::FORMAT_R24G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS        		},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_R8G8_TYPELESS     			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_UNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM        			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_UINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT         			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_SNORM					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM        			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_SINT 					, DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R16_TYPELESS      			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_FLOAT						, DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_UNORM         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_UINT          			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R16_SNORM         			},
		{ CustomStruct::CRenderFormat::FORMAT_R16_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R16_SINT          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_R8_TYPELESS       			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_UINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_UINT           			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_SNORM						, DXGI_FORMAT::DXGI_FORMAT_R8_SNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R8_SINT 						, DXGI_FORMAT::DXGI_FORMAT_R8_SINT           			},
		{ CustomStruct::CRenderFormat::FORMAT_A8_UNORM						, DXGI_FORMAT::DXGI_FORMAT_A8_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_R1_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_R9G9B9E5_SHAREDEXP			, DXGI_FORMAT::DXGI_FORMAT_R9G9B9E5_SHAREDEXP			},
		{ CustomStruct::CRenderFormat::FORMAT_R8G8_B8G8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_R8G8_B8G8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_G8R8_G8B8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_G8R8_G8B8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC1_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_UNORM						, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC1_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC1_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC2_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC2_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC2_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_TYPELESS					, DXGI_FORMAT::DXGI_FORMAT_BC3_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC3_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC3_UNORM_SRGB				},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC4_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC4_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC4_SNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC5_TYPELESS					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_BC5_SNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC5_SNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_B5G6R5_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_B5G6R5_UNORM					},
		{ CustomStruct::CRenderFormat::FORMAT_B5G5R5A1_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B5G5R5A1_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM				},
		{ CustomStruct::CRenderFormat::FORMAT_R10G10B10_XR_BIAS_A2_UNORM	, DXGI_FORMAT::DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM	},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_TYPELESS				, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_TYPELESS  			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8A8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8A8_UNORM_SRGB			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_TYPELESS 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_TYPELESS  			},
		{ CustomStruct::CRenderFormat::FORMAT_B8G8R8X8_UNORM_SRGB 			, DXGI_FORMAT::DXGI_FORMAT_B8G8R8X8_UNORM_SRGB			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_TYPELESS 				, DXGI_FORMAT::DXGI_FORMAT_BC6H_TYPELESS      			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_UF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_UF16          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC6H_SF16						, DXGI_FORMAT::DXGI_FORMAT_BC6H_SF16          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_TYPELESS 					, DXGI_FORMAT::DXGI_FORMAT_BC7_TYPELESS       			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_UNORM 					, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM          			},
		{ CustomStruct::CRenderFormat::FORMAT_BC7_UNORM_SRGB 				, DXGI_FORMAT::DXGI_FORMAT_BC7_UNORM_SRGB     			},
		{ CustomStruct::CRenderFormat::FORMAT_AYUV 							, DXGI_FORMAT::DXGI_FORMAT_AYUV               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y410 							, DXGI_FORMAT::DXGI_FORMAT_Y410               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y416 							, DXGI_FORMAT::DXGI_FORMAT_Y416               			},
		{ CustomStruct::CRenderFormat::FORMAT_NV12 							, DXGI_FORMAT::DXGI_FORMAT_NV12               			},
		{ CustomStruct::CRenderFormat::FORMAT_P010 							, DXGI_FORMAT::DXGI_FORMAT_P010               			},
		{ CustomStruct::CRenderFormat::FORMAT_P016 							, DXGI_FORMAT::DXGI_FORMAT_P016               			},
		{ CustomStruct::CRenderFormat::FORMAT_420_OPAQUE 					, DXGI_FORMAT::DXGI_FORMAT_420_OPAQUE         			},
		{ CustomStruct::CRenderFormat::FORMAT_YUY2 							, DXGI_FORMAT::DXGI_FORMAT_YUY2               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y210 							, DXGI_FORMAT::DXGI_FORMAT_Y210               			},
		{ CustomStruct::CRenderFormat::FORMAT_Y216 							, DXGI_FORMAT::DXGI_FORMAT_Y216               			},
		{ CustomStruct::CRenderFormat::FORMAT_NV11 							, DXGI_FORMAT::DXGI_FORMAT_NV11               			},
		{ CustomStruct::CRenderFormat::FORMAT_AI44 							, DXGI_FORMAT::DXGI_FORMAT_AI44               			},
		{ CustomStruct::CRenderFormat::FORMAT_IA44 							, DXGI_FORMAT::DXGI_FORMAT_IA44               			},
		{ CustomStruct::CRenderFormat::FORMAT_P8							, DXGI_FORMAT::DXGI_FORMAT_P8                 			},
		{ CustomStruct::CRenderFormat::FORMAT_A8P8 							, DXGI_FORMAT::DXGI_FORMAT_A8P8               			},
		{ CustomStruct::CRenderFormat::FORMAT_B4G4R4A4_UNORM				, DXGI_FORMAT::DXGI_FORMAT_B4G4R4A4_UNORM     			},
		{ CustomStruct::CRenderFormat::FORMAT_P208							, DXGI_FORMAT::DXGI_FORMAT_P208							},
		{ CustomStruct::CRenderFormat::FORMAT_V208							, DXGI_FORMAT::DXGI_FORMAT_V208							},
		{ CustomStruct::CRenderFormat::FORMAT_V408							, DXGI_FORMAT::DXGI_FORMAT_V408							},
		{ CustomStruct::CRenderFormat::FORMAT_FORCE_UINT					, DXGI_FORMAT::DXGI_FORMAT_FORCE_UINT }};

		output = formatMap[input];
}
void CRenderDevice::TranslateBufferUAVFlag(UINT& output, CustomStruct::CRenderBufferUAVFlag input)
{
	static std::map<CustomStruct::CRenderBufferUAVFlag, UINT> bufferUAVFlagMap = {
	{ CustomStruct::CRenderBufferUAVFlag::BUFFER_UAV_FLAG_NONE, 0u },
	{ CustomStruct::CRenderBufferUAVFlag::BUFFER_UAV_FLAG_RAW, D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_RAW },
	{ CustomStruct::CRenderBufferUAVFlag::BUFFER_UAV_FLAG_APPEND, D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_APPEND },
	{ CustomStruct::CRenderBufferUAVFlag::BUFFER_UAV_FLAG_COUNTER, D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_COUNTER } };

	output = bufferUAVFlagMap[input];
}
void CRenderDevice::TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, CustomStruct::CRenderComparisonFunction input)
{
	static std::map<CustomStruct::CRenderComparisonFunction, D3D11_COMPARISON_FUNC> comparisonFuncMap = {
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_NEVER, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_LESS, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_LESS_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_GREATER, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_NOT_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_GREATER_EQUAL, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER_EQUAL },
		{ CustomStruct::CRenderComparisonFunction::COMPARISON_ALWAYS, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS } };

	output = comparisonFuncMap[input];
}
void CRenderDevice::TranslateStencilOperation(D3D11_DEPTH_STENCILOP_DESC& output, const CustomStruct::CRenderStencilOp& input)
{
	static std::map<CustomStruct::CRenderStencilOperation, D3D11_STENCIL_OP> stencilOpMap = {
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_KEEP, D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_ZERO, D3D11_STENCIL_OP::D3D11_STENCIL_OP_ZERO },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_REPLACE, D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INCR_SAT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR_SAT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_DECR_SAT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR_SAT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INVERT, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INVERT },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_INCR, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR },
		{ CustomStruct::CRenderStencilOperation::STENCIL_OP_DECR, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR } };

	output.StencilFailOp = stencilOpMap[input.StencilFailOp];
	output.StencilDepthFailOp = stencilOpMap[input.StencilDepthFailOp];
	output.StencilPassOp = stencilOpMap[input.StencilPassOp];
	TranslateComparisonFunction(output.StencilFunc, input.StencilFunc);
}
void CRenderDevice::TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const CustomStruct::CRenderDepthState& depthInput, const CustomStruct::CRenderStencilState* stencilInput)
{
	static std::map<CustomStruct::CRenderDepthWriteMask, D3D11_DEPTH_WRITE_MASK> depthWriteMap = {
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ZERO, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO },
		{ CustomStruct::CRenderDepthWriteMask::DEPTH_WRITE_MASK_ALL, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL } };

	output.DepthEnable = depthInput.DepthEnable;
	output.DepthWriteMask = depthWriteMap[depthInput.DepthWriteMask];
	TranslateComparisonFunction(output.DepthFunc, depthInput.DepthFunc);

	output.StencilEnable = FALSE;
	if (stencilInput)
	{
		output.StencilEnable = TRUE;
		output.StencilReadMask = static_cast<UINT8>(stencilInput->StencilReadMask);
		output.StencilWriteMask = static_cast<UINT8>(stencilInput->StencilWriteMask);
		CRenderDevice::TranslateStencilOperation(output.FrontFace, stencilInput->FrontFace);
		CRenderDevice::TranslateStencilOperation(output.BackFace, stencilInput->BackFace);
	}
}
void CRenderDevice::TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const CustomStruct::CRenderBlendState& input)
{
	static std::map<CustomStruct::CRenderBlendOption, D3D11_BLEND> blendMap = {
		{ CustomStruct::CRenderBlendOption::BLEND_ZERO, D3D11_BLEND::D3D11_BLEND_ZERO },
		{ CustomStruct::CRenderBlendOption::BLEND_ONE, D3D11_BLEND::D3D11_BLEND_ONE },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_COLOR, D3D11_BLEND::D3D11_BLEND_SRC_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_SRC_COLOR, D3D11_BLEND::D3D11_BLEND_INV_SRC_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_DEST_ALPHA, D3D11_BLEND::D3D11_BLEND_DEST_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_DEST_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA },
		{ CustomStruct::CRenderBlendOption::BLEND_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_DEST_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_DEST_COLOR, D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR },
		{ CustomStruct::CRenderBlendOption::BLEND_SRC_ALPHA_SAT, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA_SAT },
		{ CustomStruct::CRenderBlendOption::BLEND_BLEND_FACTOR, D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR },
		{ CustomStruct::CRenderBlendOption::BLEND_INV_BLEND_FACTOR, D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR } };
	static std::map<CustomStruct::CRenderBlendOperation, D3D11_BLEND_OP> blendOpMap = {
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_ADD, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_SUBTRACT, D3D11_BLEND_OP::D3D11_BLEND_OP_SUBTRACT },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_REV_SUBTRACT, D3D11_BLEND_OP::D3D11_BLEND_OP_REV_SUBTRACT },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_MIN, D3D11_BLEND_OP::D3D11_BLEND_OP_MIN },
		{ CustomStruct::CRenderBlendOperation::BLEND_OP_MAX, D3D11_BLEND_OP::D3D11_BLEND_OP_MAX } };
	static std::map<CustomStruct::CRenderBlendWriteEnable, D3D11_COLOR_WRITE_ENABLE> writeEnableMap = {
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_RED, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_RED },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_GREEN, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_GREEN },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_BLUE, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_BLUE },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALPHA, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALPHA },
		{ CustomStruct::CRenderBlendWriteEnable::COLOR_WRITE_ENABLE_ALL, D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL } };

	output.BlendEnable = input.BlendEnable;
	output.SrcBlend = blendMap[input.SrcBlend];
	output.DestBlend = blendMap[input.DstBlend];
	output.BlendOp = blendOpMap[input.BlendOp];
	output.SrcBlendAlpha = blendMap[input.SrcBlendAlpha];
	output.DestBlendAlpha = blendMap[input.DstBlendAlpha];
	output.BlendOpAlpha = blendOpMap[input.BlendOpAlpha];
	output.RenderTargetWriteMask = writeEnableMap[input.RenderTargetWriteMask];
}
void CRenderDevice::TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const CustomStruct::CRenderRasterizerState& input)
{
	static std::map<CustomStruct::CRenderCullMode, D3D11_CULL_MODE> cullMap = {
		{ CustomStruct::CRenderCullMode::CULL_NONE, D3D11_CULL_MODE::D3D11_CULL_NONE },
		{ CustomStruct::CRenderCullMode::CULL_FRONT, D3D11_CULL_MODE::D3D11_CULL_FRONT },
		{ CustomStruct::CRenderCullMode::CULL_BACK, D3D11_CULL_MODE::D3D11_CULL_BACK } };
	static std::map<CustomStruct::CRenderFillMode, D3D11_FILL_MODE> fillMap = {
		{ CustomStruct::CRenderFillMode::FILL_WIREFRAME, D3D11_FILL_MODE::D3D11_FILL_WIREFRAME },
		{ CustomStruct::CRenderFillMode::FILL_SOLID, D3D11_FILL_MODE::D3D11_FILL_SOLID } };

	outputCull = cullMap[input.CullMode];
	outputFill = fillMap[input.FillMode];
}
void CRenderDevice::TranslateSamplerState(D3D11_SAMPLER_DESC& output, const CustomStruct::CRenderSamplerState& input)
{
	static std::map<CustomStruct::CRenderFilter, D3D11_FILTER> filterMap = {
		{ CustomStruct::CRenderFilter::FILTER_POINT, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT },
		{ CustomStruct::CRenderFilter::FILTER_LINEAR, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR },
		{ CustomStruct::CRenderFilter::FILTER_ANISOTROPIC, D3D11_FILTER::D3D11_FILTER_ANISOTROPIC } };
	static std::map<CustomStruct::CRenderTextureAddressMode, D3D11_TEXTURE_ADDRESS_MODE> textureAddressModeMap = {
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_MIRROR, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER },
		{ CustomStruct::CRenderTextureAddressMode::TEXTURE_ADDRESS_MIRROR_ONCE, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR_ONCE } };

	output.Filter = filterMap[input.Filter];
	output.AddressU = textureAddressModeMap[input.AddressU];
	output.AddressV = textureAddressModeMap[input.AddressV];
	output.AddressW = textureAddressModeMap[input.AddressW];
	output.MipLODBias = input.MipLODBias;
	output.MaxAnisotropy = input.MaxAnisotropy;
	TranslateComparisonFunction(output.ComparisonFunc, input.ComparisonFunc);
	output.BorderColor[0] = input.BorderColor.r;
	output.BorderColor[1] = input.BorderColor.g;
	output.BorderColor[2] = input.BorderColor.b;
	output.BorderColor[3] = input.BorderColor.a;
	output.MinLOD = input.MinLOD;
	output.MaxLOD = input.MaxLOD;
}
void CRenderDevice::TranslateClearDepthStencilFlag(UINT& output, CustomStruct::CRenderClearDepthStencilFlag input)
{
	static std::map<CustomStruct::CRenderClearDepthStencilFlag, UINT> clearDepthStencilMap = {
		{ CustomStruct::CRenderClearDepthStencilFlag::CLEAR_DEPTH, D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH },
		{ CustomStruct::CRenderClearDepthStencilFlag::CLEAR_STENCIL, D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL },
		{ CustomStruct::CRenderClearDepthStencilFlag::CLEAR_DEPTH_STENCIL, (D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL) } };

	output = clearDepthStencilMap[input];
}
void CRenderDevice::TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, CustomStruct::CRenderPrimitiveTopology input)
{
	static std::map<CustomStruct::CRenderPrimitiveTopology, D3D_PRIMITIVE_TOPOLOGY> primitiveTopologyMap = {
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_UNDEFINED, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED },
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_POINTLIST, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST },
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINELIST, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST },
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_LINESTRIP, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP },
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLELIST, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
		{ CustomStruct::CRenderPrimitiveTopology::PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP } };

	output = primitiveTopologyMap[input];
}
void CRenderDevice::TranslateQueryDesc(D3D11_QUERY_DESC& output, const CustomStruct::CRenderQueryDesc& input)
{
	static std::map<CustomStruct::CRenderQueryType, D3D11_QUERY> queryTypeMap = {
		{ CustomStruct::CRenderQueryType::QUERY_EVENT, D3D11_QUERY::D3D11_QUERY_EVENT },
		{ CustomStruct::CRenderQueryType::QUERY_OCCLUSION, D3D11_QUERY::D3D11_QUERY_OCCLUSION },
		{ CustomStruct::CRenderQueryType::QUERY_TIMESTAMP, D3D11_QUERY::D3D11_QUERY_TIMESTAMP },
		{ CustomStruct::CRenderQueryType::QUERY_TIMESTAMP_DISJOINT, D3D11_QUERY::D3D11_QUERY_TIMESTAMP_DISJOINT },
		{ CustomStruct::CRenderQueryType::QUERY_PIPELINE_STATISTICS, D3D11_QUERY::D3D11_QUERY_PIPELINE_STATISTICS },
		{ CustomStruct::CRenderQueryType::QUERY_OCCLUSION_PREDICATE, D3D11_QUERY::D3D11_QUERY_OCCLUSION_PREDICATE },
		{ CustomStruct::CRenderQueryType::QUERY_SO_STATISTICS, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS },
		{ CustomStruct::CRenderQueryType::QUERY_SO_OVERFLOW_PREDICATE, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE },
		{ CustomStruct::CRenderQueryType::QUERY_SO_STATISTICS_STREAM0, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM0 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM0, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_STATISTICS_STREAM1, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM1 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM1, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_STATISTICS_STREAM2, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM2 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM2, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_STATISTICS_STREAM3, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM3 },
		{ CustomStruct::CRenderQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM3, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 } };
	static std::map<CustomStruct::CRenderQueryMiscFlag, UINT> queryMiscFlagMap = {
		{ CustomStruct::CRenderQueryMiscFlag::QUERY_MISC_DEFAULT, 0u },
		{ CustomStruct::CRenderQueryMiscFlag::QUERY_MISC_PREDICATEHINT, D3D11_QUERY_MISC_FLAG::D3D11_QUERY_MISC_PREDICATEHINT } };

	output.Query = queryTypeMap[input.Query];
	output.MiscFlags = queryMiscFlagMap[input.MiscFlags];
}
void CRenderDevice::TranslateGetDataFlag(UINT& output, CustomStruct::CRenderAsyncGetDataFlag input)
{
	static std::map<CustomStruct::CRenderAsyncGetDataFlag, UINT> getDataFlagMap = {
		{ CustomStruct::CRenderAsyncGetDataFlag::D3D11_ASYNC_GETDATA_DEFAULT, 0u },
		{ CustomStruct::CRenderAsyncGetDataFlag::D3D11_ASYNC_GETDATA_DONOTFLUSH, D3D11_ASYNC_GETDATA_FLAG::D3D11_ASYNC_GETDATA_DONOTFLUSH } };

	output = getDataFlagMap[input];
}
void CRenderDevice::TranslateMapType(D3D11_MAP& output, CustomStruct::CRenderMapType input)
{
	static std::map<CustomStruct::CRenderMapType, D3D11_MAP> mapTypeMap = {
		{ CustomStruct::CRenderMapType::MAP_READ, D3D11_MAP::D3D11_MAP_READ },
		{ CustomStruct::CRenderMapType::MAP_WRITE, D3D11_MAP::D3D11_MAP_WRITE },
		{ CustomStruct::CRenderMapType::MAP_READ_WRITE, D3D11_MAP::D3D11_MAP_READ_WRITE },
		{ CustomStruct::CRenderMapType::MAP_WRITE_DISCARD, D3D11_MAP::D3D11_MAP_WRITE_DISCARD },
		{ CustomStruct::CRenderMapType::MAP_WRITE_NO_OVERWRITE, D3D11_MAP::D3D11_MAP_WRITE_NO_OVERWRITE } };

	output = mapTypeMap[input];
}
void CRenderDevice::TranslateMapFlag(UINT& output, CustomStruct::CRenderMapFlag input)
{
	static std::map<CustomStruct::CRenderMapFlag, UINT> mapFlagMap = {
		{ CustomStruct::CRenderMapFlag::MAP_FLAG_NONE, 0u },
		{ CustomStruct::CRenderMapFlag::MAP_FLAG_DO_NOT_WAIT, D3D11_MAP_FLAG::D3D11_MAP_FLAG_DO_NOT_WAIT } };

	output = mapFlagMap[input];
}
void CRenderDevice::ClearFinalOutput()
{
	CRenderDevice::ClearRenderTargetView(CRenderDevice::m_RenderDevice->m_RenderTargetView);
	CRenderDevice::ClearDepthStencilView(CRenderDevice::m_RenderDevice->m_DepthStencilView);
}
void CRenderDevice::SetFinalOutput()
{
	CRenderDevice::SetRenderTarget(CRenderDevice::m_RenderDevice->m_RenderTargetView, CRenderDevice::m_RenderDevice->m_DepthStencilView);
	CRenderDevice::SetViewport(CRenderDevice::m_RenderDevice->m_Viewport);
}
D3D11_VIEWPORT CRenderDevice::GetViewport()
{
	return (CRenderDevice::m_RenderDevice->m_Viewport);
}
D3D_FEATURE_LEVEL CRenderDevice::GetFeatureLevel()
{
	return (CRenderDevice::m_RenderDevice->m_FeatureLevel);
}
Microsoft::WRL::ComPtr<ID3D11DeviceContext> CRenderDevice::GetRenderDeviceContext()
{
	return (CRenderDevice::m_RenderDevice->m_ImmediateContext);
}
Microsoft::WRL::ComPtr<ID3D11Device> CRenderDevice::GetRenderDevice()
{
	return (CRenderDevice::m_RenderDevice->m_Device);
}