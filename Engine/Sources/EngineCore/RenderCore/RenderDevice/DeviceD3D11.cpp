#include "DeviceD3D11.h"
#include "PipelineStateD3D11.h"
#include "DXGIFormatTranslate.h"

namespace PigeonEngine
{
	void RDeviceD3D11::SetInitializeData(HWND hWnd, const Vector2Int& bufferSize, UINT32 bufferDepth, UINT32 frameNum, BOOL32 windowed)
	{
		if (bufferSize.x < 1 || bufferSize.y < 1 || !(bufferDepth == 24u || bufferDepth == 32u) || frameNum < 2u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Render device init failed. Invalid arguments"));
			return;
		}
		UINT32 bufferWidth = static_cast<UINT32>(bufferSize.x);
		UINT32 bufferHeight = static_cast<UINT32>(bufferSize.y);
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
			nullptr,
			D3D_DRIVER_TYPE_HARDWARE,
			nullptr,
			0u,
			featureLevels,
			7u,
			D3D11_SDK_VERSION,
			&sd,
			m_SwapChain.ReleaseAndGetAddressOf(),
			m_Device.ReleaseAndGetAddressOf(),
			&(m_FeatureLevel),
			m_ImmediateContext.ReleaseAndGetAddressOf());	//D3D11_CREATE_DEVICE_FLAG
		if (FAILED(hr) || !(m_FeatureLevel == D3D_FEATURE_LEVEL_11_1 || m_FeatureLevel == D3D_FEATURE_LEVEL_11_0))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create D3D11 device failed."));
			return;
		}
		m_Viewport.Width = static_cast<FLOAT>(bufferWidth);
		m_Viewport.Height = static_cast<FLOAT>(bufferHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		ID3D11Texture2D* pBackBuffer = nullptr;
		hr = m_SwapChain->GetBuffer(0u, __uuidof(ID3D11Texture2D), (LPVOID*)(&pBackBuffer));
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Failed to gather swap chain back buffer."));
			return;
		}
		hr = m_Device->CreateRenderTargetView(pBackBuffer, nullptr, m_RenderTargetView.ReleaseAndGetAddressOf());
		pBackBuffer->Release();
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create swap chain back buffer RTV failed."));
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
				if (bufferDepth == 24u) { td.Format = DXGI_FORMAT::DXGI_FORMAT_R24G8_TYPELESS; }
				else if (bufferDepth == 32u) { td.Format = DXGI_FORMAT::DXGI_FORMAT_R32G8X24_TYPELESS; }
				td.SampleDesc = sd.SampleDesc;
				td.Usage = D3D11_USAGE::D3D11_USAGE_DEFAULT;
				td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL;
				td.CPUAccessFlags = 0u;
				td.MiscFlags = 0u;
			}
			hr = m_Device->CreateTexture2D(&td, nullptr, m_DepthTexture.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create main depth buffer failed."));
				return;
			}
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
			{
				::ZeroMemory(&dsvd, sizeof(dsvd));
				if (bufferDepth == 24u) { dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D24_UNORM_S8_UINT; }
				else if (bufferDepth == 32u) { dsvd.Format = DXGI_FORMAT::DXGI_FORMAT_D32_FLOAT_S8X24_UINT; }
				dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Flags = 0u;	//D3D11_DSV_FLAG
			}
			hr = m_Device->CreateDepthStencilView(m_DepthTexture.Get(), &dsvd, m_DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create main depth buffer DSV failed."));
				return;
			}
		}

		// Prevent DXGI from intercepting Alt+Enter and monitoring window size/style changes.
		// Without this, DXGI may react to programmatic SetWindowPos/SetWindowLongPtr calls
		// (e.g., our borderless fullscreen toggle) by sending unexpected window messages.
		{
			Microsoft::WRL::ComPtr<IDXGIFactory> DxgiFactory;
			if (SUCCEEDED(m_SwapChain->GetParent(IID_PPV_ARGS(DxgiFactory.GetAddressOf()))) && DxgiFactory)
			{
				DxgiFactory->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);
			}
		}
	}
	void RDeviceD3D11::Initialize()
	{
	}
	void RDeviceD3D11::ShutDown()
	{
		//if (m_RenderTargetView)
		//{
		//	m_RenderTargetView->Release();
		//	m_RenderTargetView = nullptr;
		//}
		//if (m_DepthStencilView)
		//{
		//	m_DepthStencilView->Release();
		//	m_DepthStencilView = nullptr;
		//}
		//if (m_DepthTexture)
		//{
		//	m_DepthTexture->Release();
		//	m_DepthTexture = nullptr;
		//}
		//if (m_SwapChain)
		//{
		//	m_SwapChain->Release();
		//	m_SwapChain = nullptr;
		//}
		//if (m_ImmediateContext)
		//{
		//	m_ImmediateContext->Release();
		//	m_ImmediateContext = nullptr;
		//}
		//if (m_Device)
		//{
		//	m_Device->Release();
		//	m_Device = nullptr;
		//}
	}
	BOOL32 RDeviceD3D11::CreateShaderObject(const RRHIShaderDesc& InDesc, RD3D11Shader& OutShader)
	{
		if ((!InDesc.ByteCode) || (InDesc.ByteCodeSize == 0u))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create shader object failed (empty byte code)."));
			return FALSE;
		}
		const void* ByteCode = InDesc.ByteCode.get();
		const ULONG ByteCodeSize = static_cast<ULONG>(InDesc.ByteCodeSize);
		// Keep a copy: a pipeline state that declares its own vertex data layout needs the vertex
		// shader's signature to bake one against (see CreateGraphicsPipelineState).
		OutShader.ByteCode.Empty();
		OutShader.ByteCode.SetNum(static_cast<UINT32>(InDesc.ByteCodeSize));
		{
			const UINT8* SourceBytes = static_cast<const UINT8*>(ByteCode);
			UINT8* DestBytes = OutShader.ByteCode.GetData();
			for (UINT32 ByteIndex = 0u; (DestBytes != nullptr) && (ByteIndex < static_cast<UINT32>(InDesc.ByteCodeSize)); ByteIndex++)
			{
				DestBytes[ByteIndex] = SourceBytes[ByteIndex];
			}
		}
		switch (InDesc.Stage)
		{
		case ERHIShaderStage::RHI_SHADER_STAGE_VERTEX:
		{
			HRESULT hr = m_Device->CreateVertexShader(ByteCode, ByteCodeSize, nullptr, OutShader.VertexShader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader resource failed."));
				return FALSE;
			}
			if ((InDesc.InputLayouts.Num<UINT32>() == 0u) || (!InDesc.InputLayouts.GetData()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader which needs fitted input layouts."));
				return FALSE;
			}
			const UINT32 LayoutNum = InDesc.InputLayouts.Num<UINT32>();
			TArray<D3D11_INPUT_ELEMENT_DESC> TempLayouts;
			TempLayouts.SetNum(LayoutNum);
			for (UINT32 i = 0u; i < LayoutNum; i++)
			{
				RDeviceD3D11::TranslateInputLayoutDesc(TempLayouts[i], InDesc.InputLayouts[i]);
			}
			hr = m_Device->CreateInputLayout(TempLayouts.GetData(), LayoutNum, ByteCode, ByteCodeSize, OutShader.InputLayout.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create vertex shader input layout failed."));
				return FALSE;
			}
			break;
		}
		case ERHIShaderStage::RHI_SHADER_STAGE_PIXEL:
		{
			const HRESULT hr = m_Device->CreatePixelShader(ByteCode, ByteCodeSize, nullptr, OutShader.PixelShader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create pixel shader resource failed."));
				return FALSE;
			}
			break;
		}
		case ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE:
		{
			const HRESULT hr = m_Device->CreateComputeShader(ByteCode, ByteCodeSize, nullptr, OutShader.ComputeShader.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create compute shader resource failed."));
				return FALSE;
			}
			break;
		}
		default:
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create shader object failed (unsupported stage)."));
			return FALSE;
		}
		}
		OutShader.Stage = InDesc.Stage;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBuffer(RBufferResource& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData)
	{
		const BOOL32 Result = CreateBuffer(buffer.Buffer, bufferDesc, subData);
		if (Result)
		{
			buffer.ByteSize				= bufferDesc.ByteWidth;
			buffer.StructureByteStride	= bufferDesc.StructureByteStride;
			buffer.BindFlags			= bufferDesc.BindFlags;
		}
		return Result;
	}
	BOOL32 RDeviceD3D11::CreateStructuredBuffer(RStructuredBuffer& output, const RStructuredBufferDesc& structuredBufferDesc, const RSubresourceDataDesc* subData)
	{
		if (structuredBufferDesc.NumElements < 1u || structuredBufferDesc.FirstElement >= structuredBufferDesc.NumElements || structuredBufferDesc.StructureSize < 4u)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource check failed (error description)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		{
			output.AccessMapRead = (structuredBufferDesc.AccessFlags & RCPUAccessFlagType::CPU_ACCESS_READ) != 0;
			output.AccessMapWrite = (structuredBufferDesc.AccessFlags & RCPUAccessFlagType::CPU_ACCESS_WRITE) != 0;
			if (structuredBufferDesc.GPUWritable && output.AccessMapWrite)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource check failed (error access ability)."));
				return FALSE;
			}
			D3D11_BUFFER_DESC bd;
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = structuredBufferDesc.StructureSize * structuredBufferDesc.NumElements;
			bd.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			if (!(structuredBufferDesc.GPUWritable) && !(output.AccessMapWrite))
			{
				if (structuredBufferDesc.AccessFlags == RCPUAccessFlagType::CPU_ACCESS_DEFAULT)
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
			TranslateCPUAccessFlag(bd.CPUAccessFlags, structuredBufferDesc.AccessFlags);
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
				hr = m_Device->CreateBuffer(&bd, &sd, output.Buffer.ReleaseAndGetAddressOf());
			}
			else
			{
				hr = m_Device->CreateBuffer(&bd, nullptr, output.Buffer.ReleaseAndGetAddressOf());
			}
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource failed."));
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
			HRESULT hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource SRV failed."));
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
				RDeviceD3D11::TranslateUAVFlag(uavd.Buffer.Flags, structuredBufferDesc.UAVFlags);
				HRESULT hr = m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
				if (FAILED(hr))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create structured buffer resource UAV failed."));
					return FALSE;
				}
			}
		}
		output.ByteSize				= static_cast<UINT64>(structuredBufferDesc.StructureSize) * structuredBufferDesc.NumElements;
		output.StructureByteStride	= structuredBufferDesc.StructureSize;
		output.BindFlags			= static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE) |
			(structuredBufferDesc.GPUWritable ? static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS) : 0u);
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateRenderTexture2D(RRenderTexture2D& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData)
	{
		// This factory builds a 2D texture. A depth texture states its bit count in Depth - a
		// reading the resolver below recognises - while any other non-zero Depth would be a
		// volume, which a 2D texture has no meaning for.
		if ((RFormatTraits::IsDepthBitCountDesc(textureDesc) == FALSE) && (textureDesc.Depth != 0u))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D depth check failed (wrong depth format used)."));
			return FALSE;
		}
		output.ReleaseRenderResource();

		// The same resolution the D3D12 back-end uses: the resource format and one format per
		// view, so an sRGB texture means the same thing on either API. In particular the
		// resource itself is created linear or typeless - D3D11 only accepts a differently
		// formatted view over a typeless resource, and no API has an sRGB unordered access
		// view - while the sRGB reading lives on the SRV.
		RTextureFormatSet FormatSet;
		if (RFormatTraits::ResolveTextureFormats(textureDesc, FormatSet) == FALSE)
		{
			return FALSE;
		}
		output.ViewFormats = FormatSet;
		const BOOL32 isDepthFormat = (FormatSet.DSV != RFormatType::FORMAT_UNKNOWN) ? TRUE : FALSE;

		D3D11_TEXTURE2D_DESC td;
		{
			UINT32 tBindFlags = 0u;
			DXGI_FORMAT tFormat = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.Resource);
			RDeviceD3D11::TranslateBindFlag(tBindFlags, textureDesc.BindFlags);
			::ZeroMemory(&td, sizeof(td));
			td.Width = textureDesc.Width;
			td.Height = textureDesc.Height;
			td.MipLevels = textureDesc.MipLevels;
			td.ArraySize = textureDesc.ArraySize;
			td.Format = tFormat;
			td.SampleDesc.Count = textureDesc.SampleDesc.Count;
			td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
			td.BindFlags = tBindFlags;
			RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
			RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, static_cast<RCPUAccessFlagType>(textureDesc.CPUAccessFlags));
			RDeviceD3D11::TranslateResourceMiscFlag(td.MiscFlags, static_cast<RResourceMiscFlagType>(textureDesc.MiscFlags));
		}
		HRESULT hr = S_FALSE;
		// Same single-subresource contract as CreateTexture2D: only a single-mip texture
		// can be seeded here.
		if ((subData != nullptr) && (textureDesc.MipLevels == 1u))
		{
			D3D11_SUBRESOURCE_DATA subresourceData;
			subresourceData.pSysMem = subData->pSysMem;
			subresourceData.SysMemPitch = subData->SysMemPitch;
			subresourceData.SysMemSlicePitch = subData->SysMemSlicePitch;
			hr = m_Device->CreateTexture2D(&td, &subresourceData, output.Buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource failed."));
			return FALSE;
		}
		if (FormatSet.SRV != RFormatType::FORMAT_UNKNOWN)
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			{
				::ZeroMemory(&srvd, sizeof(srvd));
				srvd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.SRV);
				srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
				srvd.Texture2D.MostDetailedMip = 0u;
				srvd.Texture2D.MipLevels = td.MipLevels;
			}
			hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource SRV failed."));
				return FALSE;
			}
		}
		if ((FormatSet.UAV != RFormatType::FORMAT_UNKNOWN) && (!isDepthFormat))
		{
			D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
			::ZeroMemory(&uavd, sizeof(uavd));
			uavd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.UAV);
			uavd.ViewDimension = D3D11_UAV_DIMENSION::D3D11_UAV_DIMENSION_TEXTURE2D;
			uavd.Texture2D.MipSlice = 0u;
			hr = m_Device->CreateUnorderedAccessView(output.Buffer.Get(), &uavd, output.UnorderedAccessView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource UAV failed."));
				return FALSE;
			}
		}
		if ((FormatSet.RTV != RFormatType::FORMAT_UNKNOWN) && (!isDepthFormat))
		{
			D3D11_RENDER_TARGET_VIEW_DESC rtvd;
			{
				::ZeroMemory(&rtvd, sizeof(rtvd));
				rtvd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.RTV);
				rtvd.ViewDimension = D3D11_RTV_DIMENSION::D3D11_RTV_DIMENSION_TEXTURE2D;
				rtvd.Texture2D.MipSlice = 0u;
			}
			hr = m_Device->CreateRenderTargetView(output.Buffer.Get(), &rtvd, output.RenderTargetView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource RTV failed."));
				return FALSE;
			}
		}
		if (isDepthFormat)
		{
			D3D11_DEPTH_STENCIL_VIEW_DESC dsvd;
			{
				::ZeroMemory(&dsvd, sizeof(dsvd));
				dsvd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.DSV);
				dsvd.ViewDimension = D3D11_DSV_DIMENSION::D3D11_DSV_DIMENSION_TEXTURE2D;
				dsvd.Flags = 0u;	//D3D11_DSV_FLAG
				dsvd.Texture2D.MipSlice = 0u;
			}
			hr = m_Device->CreateDepthStencilView(output.Buffer.Get(), &dsvd, output.DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create render texture2D resource DSV failed."));
				return FALSE;
			}
		}
		output.Width		= textureDesc.Width;
		output.Height		= textureDesc.Height;
		output.MipLevels	= textureDesc.MipLevels;
		output.ArraySize	= textureDesc.ArraySize;
		output.Format		= textureDesc.BufferFormat;
		output.BindFlags	= textureDesc.BindFlags;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateTexture2D(RTexture2DResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData)
	{
		output.ReleaseRenderResource();
		// The resource format and the SRV format come from the same resolution the D3D12
		// back-end uses, so an sRGB texture is stored linear/typeless here too and read
		// through an sRGB view.
		RTextureFormatSet FormatSet;
		if (RFormatTraits::ResolveTextureFormats(textureDesc, FormatSet) == FALSE)
		{
			return FALSE;
		}
		if (FormatSet.SRV == RFormatType::FORMAT_UNKNOWN)
		{
			// This factory exists to produce a sampled texture: a desc that binds no SRV has
			// no view for it to create.
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture2D called for a desc that binds no shader resource view."));
			return FALSE;
		}
		D3D11_TEXTURE2D_DESC td;
		{
			::ZeroMemory(&td, sizeof(td));
			td.Width = textureDesc.Width;
			td.Height = textureDesc.Height;
			td.MipLevels = textureDesc.MipLevels;
			td.ArraySize = textureDesc.ArraySize;
			td.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.Resource);
			td.SampleDesc.Count = textureDesc.SampleDesc.Count;
			td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
			td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
			RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
			RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, static_cast<RCPUAccessFlagType>(textureDesc.CPUAccessFlags));
			RDeviceD3D11::TranslateResourceMiscFlag(td.MiscFlags, static_cast<RResourceMiscFlagType>(textureDesc.MiscFlags));
		}
		HRESULT hr = S_FALSE;
		// D3D11 reads MipLevels * ArraySize entries out of pInitialData, while the RHI hands
		// over one subresource: the payload only covers a single-mip texture, deeper chains
		// have to come through IRRHIDevice::UploadTexture.
		if (subData && textureDesc.MipLevels == 1u)
		{
			D3D11_SUBRESOURCE_DATA subresourceData;
			subresourceData.pSysMem = subData->pSysMem;
			subresourceData.SysMemPitch = subData->SysMemPitch;
			subresourceData.SysMemSlicePitch = subData->SysMemSlicePitch;

			hr = m_Device->CreateTexture2D(&td, &subresourceData, output.Buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture2D resource failed."));
			return FALSE;
		}
		D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
		{
			::ZeroMemory(&srvd, sizeof(srvd));
			srvd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.SRV);
			srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURE2D;
			srvd.Texture2D.MostDetailedMip = 0u;
			srvd.Texture2D.MipLevels = td.MipLevels;
		}
		hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture2D resource SRV failed."));
			return FALSE;
		}
		output.Width		= textureDesc.Width;
		output.Height		= textureDesc.Height;
		output.MipLevels	= textureDesc.MipLevels;
		output.ArraySize	= textureDesc.ArraySize;
		output.Format		= textureDesc.BufferFormat;
		output.BindFlags	= textureDesc.BindFlags;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateTextureCube(RTextureCubeResource& output, const RTextureDesc& textureDesc, const RSubresourceDataDesc* subData)
	{
		if (textureDesc.Width != textureDesc.Height)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube check failed (size of texture cube must be a square)."));
			return FALSE;
		}
		output.ReleaseRenderResource();
		// Same resolution as every other creation path; a cube's sRGB reading is an SRV
		// property like any other texture's.
		RTextureFormatSet FormatSet;
		if (RFormatTraits::ResolveTextureFormats(textureDesc, FormatSet) == FALSE)
		{
			return FALSE;
		}
		if (FormatSet.SRV == RFormatType::FORMAT_UNKNOWN)
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube called for a desc that binds no shader resource view."));
			return FALSE;
		}
		{
			D3D11_TEXTURE2D_DESC td;
			{
				::ZeroMemory(&td, sizeof(td));
				td.Width = textureDesc.Width;
				td.Height = textureDesc.Height;
				td.MipLevels = textureDesc.MipLevels;
				td.ArraySize = 6u;	//Texture cube has fixed number(6) of textures.
				td.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.Resource);
				td.SampleDesc.Count = textureDesc.SampleDesc.Count;
				td.SampleDesc.Quality = textureDesc.SampleDesc.Quality;
				RDeviceD3D11::TranslateUsageFlag(td.Usage, textureDesc.UsageFlag);
				td.BindFlags = D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE;
				RDeviceD3D11::TranslateCPUAccessFlag(td.CPUAccessFlags, static_cast<RCPUAccessFlagType>(textureDesc.CPUAccessFlags));
				td.MiscFlags = D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE;
				if (textureDesc.MipLevels == 0u || textureDesc.MipLevels > 1u)
				{
					td.MiscFlags |= D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS;
				}
			}
			HRESULT hr = S_FALSE;
			// One entry per face covers a single-mip cube; a full chain would need
			// 6 * MipLevels entries, which the single-subresource RHI path cannot supply.
			//
			// Only a complete set is handed over, and that is a rule of the runtime rather than a
			// convenience: every entry of an initial data array has to carry a payload, so a partial
			// one (the RHI's single subresource reaching only face 0) makes CreateTexture2D fail
			// outright. A caller with faces to seed uploads them through UploadTexture instead.
			BOOL8 bAllFacesSeeded = ((subData != nullptr) && (textureDesc.MipLevels == 1u)) ? TRUE : FALSE;
			for (UINT32 i = 0u; (i < 6u) && (bAllFacesSeeded != FALSE); i++)
			{
				if (subData[i].pSysMem == nullptr)
				{
					bAllFacesSeeded = FALSE;
				}
			}
			if (bAllFacesSeeded != FALSE)
			{
				D3D11_SUBRESOURCE_DATA sd[6u];
				for (UINT32 i = 0u; i < 6u; i++)
				{
					::ZeroMemory(&(sd[i]), sizeof(sd[i]));
					sd[i].pSysMem = subData[i].pSysMem;
					sd[i].SysMemPitch = subData[i].SysMemPitch;
					sd[i].SysMemSlicePitch = subData[i].SysMemSlicePitch;
				}
				hr = m_Device->CreateTexture2D(&td, sd, output.Buffer.ReleaseAndGetAddressOf());
			}
			else
			{
				hr = m_Device->CreateTexture2D(&td, nullptr, output.Buffer.ReleaseAndGetAddressOf());
			}
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube resource failed."));
				return FALSE;
			}
		}
		{
			D3D11_SHADER_RESOURCE_VIEW_DESC srvd;
			{
				::ZeroMemory(&srvd, sizeof(srvd));
				srvd.Format = RDXGIFormatTranslate::ToDXGIOrUnknown(FormatSet.SRV);
				srvd.ViewDimension = D3D_SRV_DIMENSION::D3D11_SRV_DIMENSION_TEXTURECUBE;
				srvd.Texture2D.MostDetailedMip = 0u;
				srvd.Texture2D.MipLevels = textureDesc.MipLevels;
			}
			HRESULT hr = m_Device->CreateShaderResourceView(output.Buffer.Get(), &srvd, output.ShaderResourceView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create texture cube resource SRV failed."));
				return FALSE;
			}
		}
		output.Width		= textureDesc.Width;
		output.Height		= textureDesc.Height;
		output.MipLevels	= textureDesc.MipLevels;
		output.Format		= textureDesc.BufferFormat;
		output.BindFlags	= textureDesc.BindFlags;
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBuffer(Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const RBufferDesc& bufferDesc, const RSubresourceDataDesc* subData)
	{
		D3D11_BUFFER_DESC bd;
		{
			::ZeroMemory(&bd, sizeof(bd));
			bd.ByteWidth = static_cast<UINT>(bufferDesc.ByteWidth);
			RDeviceD3D11::TranslateUsageFlag(bd.Usage, bufferDesc.Usage);
			RDeviceD3D11::TranslateBindFlag(bd.BindFlags, bufferDesc.BindFlags);
			RDeviceD3D11::TranslateCPUAccessFlag(bd.CPUAccessFlags, bufferDesc.CPUAccessFlags);
			RDeviceD3D11::TranslateResourceMiscFlag(bd.MiscFlags, bufferDesc.MiscFlags);
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
			hr = m_Device->CreateBuffer(&bd, &sd, buffer.ReleaseAndGetAddressOf());
		}
		else
		{
			hr = m_Device->CreateBuffer(&bd, nullptr, buffer.ReleaseAndGetAddressOf());
		}
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create buffer resource failed."));
			return FALSE;
		}
		return TRUE;
	}
	void RDeviceD3D11::UploadBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource, const D3D11_BOX* dstBox)
	{
		m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
	}
	void RDeviceD3D11::UploadResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dstResource, const void* srcData, UINT32 srcRowPitch, UINT32 srcDepthPitch, UINT32 dstSubresource, const D3D11_BOX* dstBox)
	{
		m_ImmediateContext->UpdateSubresource(dstResource.Get(), dstSubresource, dstBox, srcData, srcRowPitch, srcDepthPitch);
	}
	void RDeviceD3D11::Present(UINT32 InSyncInterval) noexcept
	{
		m_SwapChain->Present(InSyncInterval, 0u);
	}
	void RDeviceD3D11::SetDefaultDepthStencilState()
	{
		m_ImmediateContext->OMSetDepthStencilState(nullptr, 0x0u);
	}
	void RDeviceD3D11::SetDepthStencilState(const Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const UINT32& stencilRef)
	{
		m_ImmediateContext->OMSetDepthStencilState(dss.Get(), stencilRef);
	}
	void RDeviceD3D11::SetDefaultBlendState()
	{
		m_ImmediateContext->OMSetBlendState(nullptr, nullptr, 0xffffffffu);
	}
	void RDeviceD3D11::SetBlendState(const Microsoft::WRL::ComPtr<ID3D11BlendState>& bs, const Color4& blendFactor, const UINT32& sampleMask)
	{
		m_ImmediateContext->OMSetBlendState(bs.Get(), blendFactor.rgba, sampleMask);
	}
	void RDeviceD3D11::SetNoRenderTarget()
	{
		ID3D11RenderTargetView* rtvs[] = { nullptr };
		m_ImmediateContext->OMSetRenderTargets(1u, rtvs, nullptr);
	}
	void RDeviceD3D11::SetRenderTargetOnly(const RRenderTexture2D& InRTV)
	{
		m_ImmediateContext->OMSetRenderTargets(1u, InRTV.RenderTargetView.GetAddressOf(), nullptr);
	}
	void RDeviceD3D11::SetDepthStencilOnly(const RRenderTexture2D& InDSV)
	{
		ID3D11RenderTargetView* rtvs[] = { nullptr };
		m_ImmediateContext->OMSetRenderTargets(1u, rtvs, InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRenderTarget(const RRenderTexture2D& InRTV, const RRenderTexture2D& InDSV)
	{
		m_ImmediateContext->OMSetRenderTargets(1u, InRTV.RenderTargetView.GetAddressOf(), InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum)
	{
		TArray<ID3D11RenderTargetView*> UsedRTVs;
		UsedRTVs.SetNum(InRTVNum);
		for (UINT32 i = 0u; i < InRTVNum; i++)
		{
			UsedRTVs[i] = (InRTVs[i]).RenderTargetView.Get();
		}
		m_ImmediateContext->OMSetRenderTargets(InRTVNum, UsedRTVs.GetData(), nullptr);
	}
	void RDeviceD3D11::SetRenderTargets(const RRenderTexture2D* InRTVs, const UINT32& InRTVNum, const RRenderTexture2D& InDSV)
	{
		TArray<ID3D11RenderTargetView*> UsedRTVs;
		UsedRTVs.SetNum(InRTVNum);
		for (UINT32 i = 0u; i < InRTVNum; i++)
		{
			UsedRTVs[i] = (InRTVs[i]).RenderTargetView.Get();
		}
		m_ImmediateContext->OMSetRenderTargets(InRTVNum, UsedRTVs.GetData(), InDSV.DepthStencilView.Get());
	}
	void RDeviceD3D11::SetRasterizerState(const Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs)
	{
		m_ImmediateContext->RSSetState(rs.Get());
	}
	void RDeviceD3D11::SetViewport(const EViewport& viewport)
	{
		D3D11_VIEWPORT vp = {
			viewport.TopLeftX, viewport.TopLeftY,
			viewport.Width, viewport.Height,
			viewport.MinDepth, viewport.MaxDepth };
		m_ImmediateContext->RSSetViewports(1u, &vp);
	}
	void RDeviceD3D11::SetViewports(const EViewport* viewports, const UINT32& viewportNum)
	{
		TArray<D3D11_VIEWPORT> vps;
		vps.SetNum(viewportNum);
		for (UINT32 i = 0u; i < viewportNum; i++)
		{
			vps[i].TopLeftX = viewports[i].TopLeftX;
			vps[i].TopLeftY = viewports[i].TopLeftY;
			vps[i].Width = viewports[i].Width;
			vps[i].Height = viewports[i].Height;
			vps[i].MinDepth = viewports[i].MinDepth;
			vps[i].MaxDepth = viewports[i].MaxDepth;
		}
		m_ImmediateContext->RSSetViewports(viewportNum, vps.GetData());
	}
	void RDeviceD3D11::SetNoVSShader()
	{
		m_ImmediateContext->VSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetNoPSShader()
	{
		m_ImmediateContext->PSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetNoCSShader()
	{
		m_ImmediateContext->CSSetShader(nullptr, nullptr, 0u);
	}
	void RDeviceD3D11::SetVSShader(const Microsoft::WRL::ComPtr<ID3D11VertexShader>& vs)
	{
		m_ImmediateContext->VSSetShader(vs.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::SetPSShader(const Microsoft::WRL::ComPtr<ID3D11PixelShader>& ps)
	{
		m_ImmediateContext->PSSetShader(ps.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::SetCSShader(const Microsoft::WRL::ComPtr<ID3D11ComputeShader>& cs)
	{
		m_ImmediateContext->CSSetShader(cs.Get(), nullptr, 0u);
	}
	void RDeviceD3D11::BindVSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindVSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.SetNum(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->VSSetSamplers(startSlot, ssNum, sampler.GetData());
	}
	void RDeviceD3D11::BindPSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindPSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.SetNum(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->PSSetSamplers(startSlot, ssNum, sampler.GetData());
	}
	void RDeviceD3D11::BindCSSamplerState(const Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetSamplers(startSlot, 1u, ss.GetAddressOf());
	}
	void RDeviceD3D11::BindCSSamplerStates(const Microsoft::WRL::ComPtr<ID3D11SamplerState>* ss, const UINT32& startSlot, const UINT32& ssNum)
	{
		TArray<ID3D11SamplerState*> sampler;
		sampler.SetNum(ssNum);
		for (UINT32 i = 0u; i < ssNum; i++)
		{
			sampler[i] = ss[i].Get();
		}
		m_ImmediateContext->CSSetSamplers(startSlot, ssNum, sampler.GetData());
	}
	void RDeviceD3D11::BindVSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindVSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.SetNum(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->VSSetConstantBuffers(startSlot, bufferNum, buffers.GetData());
	}
	void RDeviceD3D11::BindPSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindPSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.SetNum(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->PSSetConstantBuffers(startSlot, bufferNum, buffers.GetData());
	}
	void RDeviceD3D11::BindCSConstantBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& buffer, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetConstantBuffers(startSlot, 1u, buffer.GetAddressOf());
	}
	void RDeviceD3D11::BindCSConstantBuffers(const Microsoft::WRL::ComPtr<ID3D11Buffer>* buffer, const UINT32& startSlot, const UINT32& bufferNum)
	{
		TArray<ID3D11Buffer*> buffers;
		buffers.SetNum(bufferNum);
		for (UINT32 i = 0u; i < bufferNum; i++)
		{
			buffers[i] = buffer[i].Get();
		}
		m_ImmediateContext->CSSetConstantBuffers(startSlot, bufferNum, buffers.GetData());
	}
	void RDeviceD3D11::BindVSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->VSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindVSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.SetNum(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->VSSetShaderResources(startSlot, srvNum, srvs.GetData());
	}
	void RDeviceD3D11::BindPSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->PSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindPSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.SetNum(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->PSSetShaderResources(startSlot, srvNum, srvs.GetData());
	}
	void RDeviceD3D11::BindCSShaderResourceView(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& srv, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetShaderResources(startSlot, 1u, srv.GetAddressOf());
	}
	void RDeviceD3D11::BindCSShaderResourceViews(const Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>* srv, const UINT32& startSlot, const UINT32& srvNum)
	{
		TArray<ID3D11ShaderResourceView*> srvs;
		srvs.SetNum(srvNum);
		for (UINT32 i = 0u; i < srvNum; i++)
		{
			srvs[i] = srv[i].Get();
		}
		m_ImmediateContext->CSSetShaderResources(startSlot, srvNum, srvs.GetData());
	}
	void RDeviceD3D11::BindNoCSUnorderedAccessView(const UINT32& startSlot)
	{
		ID3D11UnorderedAccessView* uavs[] = { nullptr };
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uavs, nullptr);
	}
	void RDeviceD3D11::BindCSUnorderedAccessView(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const UINT32& startSlot)
	{
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, 1u, uav.GetAddressOf(), nullptr);
	}
	void RDeviceD3D11::BindCSUnorderedAccessViews(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>* uav, const UINT32& startSlot, const UINT32& uavNum)
	{
		TArray<ID3D11UnorderedAccessView*> uavs;
		uavs.SetNum(uavNum);
		for (UINT32 i = 0u; i < uavNum; i++)
		{
			uavs[i] = uav[i].Get();
		}
		m_ImmediateContext->CSSetUnorderedAccessViews(startSlot, uavNum, uavs.GetData(), nullptr);
	}
	void RDeviceD3D11::CopyTexture2DResource(const Microsoft::WRL::ComPtr<ID3D11Texture2D>& src, const Microsoft::WRL::ComPtr<ID3D11Texture2D>& dst)
	{
		m_ImmediateContext->CopyResource(dst.Get(), src.Get());
	}
	void RDeviceD3D11::ClearRenderTargetView(const RRenderTexture2D& InRTV, const Color4& InClearColor)
	{
		m_ImmediateContext->ClearRenderTargetView(InRTV.RenderTargetView.Get(), InClearColor.rgba);
	}
	void RDeviceD3D11::ClearDepthStencilView(const RRenderTexture2D& InDSV, const UINT32 InFlags, const FLOAT InDepth, const UINT32 InStencil)
	{
		UINT32 UsedClearFlags = 0u;
		RDeviceD3D11::TranslateClearDepthStencilFlag(UsedClearFlags, InFlags);
		UINT8 UsedClearStencil = static_cast<UINT8>(EMath::Min(InStencil, 0xffu));
		FLOAT UsedClearDepth = EMath::Clamp(InDepth, 0.f, 1.f);
		m_ImmediateContext->ClearDepthStencilView(InDSV.DepthStencilView.Get(), UsedClearFlags, UsedClearDepth, UsedClearStencil);
	}
	void RDeviceD3D11::ClearUnorderedAccessViewFloat(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Color4& clearValue)
	{
		m_ImmediateContext->ClearUnorderedAccessViewFloat(uav.Get(), clearValue.rgba);
	}
	void RDeviceD3D11::ClearUnorderedAccessViewUint(const Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>& uav, const Vector4Int& clearValue)
	{
		UINT32 values[4] = { static_cast<UINT32>(clearValue.x), static_cast<UINT32>(clearValue.y), static_cast<UINT32>(clearValue.z), static_cast<UINT32>(clearValue.w) };
		m_ImmediateContext->ClearUnorderedAccessViewUint(uav.Get(), values);
	}
	void RDeviceD3D11::SetInputLayoutAndVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout, const Microsoft::WRL::ComPtr<ID3D11Buffer>& vb, const UINT32& stride, const UINT32& offset)
	{
		m_ImmediateContext->IASetInputLayout(layout.Get());
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(0u, 1u, vb.GetAddressOf(), &stride, &offset);
	}
	void RDeviceD3D11::SetInputLayout(const Microsoft::WRL::ComPtr<ID3D11InputLayout>& layout)
	{
		m_ImmediateContext->IASetInputLayout(layout.Get());
	}
	void RDeviceD3D11::SetNoInputLayout()
	{
		m_ImmediateContext->IASetInputLayout(nullptr);
	}
	void RDeviceD3D11::SetVertexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InStride, UINT32 InOffset, UINT32 InStartSlot, UINT32 InBufferNum)
	{
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(InStartSlot, InBufferNum, InBuffer.GetAddressOf(), &InStride, &InOffset);
	}
	void RDeviceD3D11::SetNoVertexBuffer()
	{
		ID3D11Buffer* Buffers[] = { nullptr };
		UINT32 Stride = 0u, Offset = 0u;
		//D3D11_IA_VERTEX_INPUT_RESOURCE_SLOT_COUNT 
		m_ImmediateContext->IASetVertexBuffers(0u, 1u, Buffers, &Stride, &Offset);
	}
	void RDeviceD3D11::SetIndexBuffer(const Microsoft::WRL::ComPtr<ID3D11Buffer>& InBuffer, UINT32 InOffset, RFormatType InFormat)
	{
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_UNKNOWN;
		RDeviceD3D11::TranslateResourceFormat(IndexFormat, InFormat);
		m_ImmediateContext->IASetIndexBuffer(InBuffer.Get(), IndexFormat, InOffset);
	}
	void RDeviceD3D11::SetNoIndexBuffer()
	{
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R32_UINT;
		m_ImmediateContext->IASetIndexBuffer(nullptr, IndexFormat, 0u);
	}
	void RDeviceD3D11::SetPrimitiveTopology(RPrimitiveTopologyType topology)
	{
		D3D_PRIMITIVE_TOPOLOGY output = D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
		TranslatePrimitiveTopology(output, topology);
		m_ImmediateContext->IASetPrimitiveTopology(output);
	}
	void RDeviceD3D11::Draw(const UINT32& vertexCount, const UINT32& startVertexLocation)
	{
		m_ImmediateContext->Draw(vertexCount, startVertexLocation);
	}
	void RDeviceD3D11::DrawIndexed(const UINT32& indexCount, const UINT32& startIndexLocation, const INT32& baseVertexLocation)
	{
		m_ImmediateContext->DrawIndexed(indexCount, startIndexLocation, baseVertexLocation);
	}
	void RDeviceD3D11::DrawIndexedInstance(const UINT32& instanceCount, const UINT32& indexCountPerInstance, const UINT32& startInstanceLocation, const UINT32& startIndexLocation, const INT32& BaseVertexLocation)
	{
		m_ImmediateContext->DrawIndexedInstanced(indexCountPerInstance, instanceCount, startIndexLocation, BaseVertexLocation, startInstanceLocation);
	}
	void RDeviceD3D11::Dispatch(const UINT32& x, const UINT32& y, const UINT32& z)
	{
		m_ImmediateContext->Dispatch(x, y, z);
	}
	void RDeviceD3D11::DispatchIndirect(const Microsoft::WRL::ComPtr<ID3D11Buffer>& arg, const UINT32& alignedByteOffsetForArgs)
	{
		m_ImmediateContext->DispatchIndirect(arg.Get(), alignedByteOffsetForArgs);
	}
	BOOL32 RDeviceD3D11::CreateDepthStencilState(Microsoft::WRL::ComPtr<ID3D11DepthStencilState>& dss, const RDepthState& depthState, const RStencilState* stencilState)
	{
		D3D11_DEPTH_STENCIL_DESC dsd;
		{
			::ZeroMemory(&dsd, sizeof(dsd));
			RDeviceD3D11::TranslateDepthStencilState(dsd, depthState, stencilState);
		}
		HRESULT hr = m_Device->CreateDepthStencilState(&dsd, dss.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create depth stencil state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateBlendState(Microsoft::WRL::ComPtr<ID3D11BlendState>& OutBlendState, const RBlendDesc& InBlendDesc)
	{
		const INT32 BlendStateNum = InBlendDesc.BlendStates.Num<INT32>();
		if ((BlendStateNum <= 0) || (BlendStateNum > 8))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create blend state check failed."));
			return FALSE;
		}
		D3D11_BLEND_DESC D3DBlendDesc;
		{
			::ZeroMemory(&D3DBlendDesc, sizeof(D3DBlendDesc));
			D3DBlendDesc.AlphaToCoverageEnable = InBlendDesc.AlphaToCoverageEnable;
			D3DBlendDesc.IndependentBlendEnable = InBlendDesc.IndependentBlendEnable;
			RDeviceD3D11::TranslateBlendState(D3DBlendDesc.RenderTarget[0], InBlendDesc.BlendStates[0]);
			if (D3DBlendDesc.IndependentBlendEnable)
			{
				for (INT32 i = 1; i < BlendStateNum; i++)
				{
					RDeviceD3D11::TranslateBlendState(D3DBlendDesc.RenderTarget[i], InBlendDesc.BlendStates[i]);
				}
			}
		}
		HRESULT hr = m_Device->CreateBlendState(&D3DBlendDesc, OutBlendState.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create blend state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateRasterizerState(Microsoft::WRL::ComPtr<ID3D11RasterizerState>& rs, const RRasterizerState& rasterizerState)
	{
		D3D11_RASTERIZER_DESC rd;
		{
			::ZeroMemory(&rd, sizeof(rd));
			RDeviceD3D11::TranslateRasterizerState(rd.CullMode, rd.FillMode, rasterizerState);
			rd.FrontCounterClockwise = FALSE;
			rd.DepthBias = D3D11_DEFAULT_DEPTH_BIAS;
			rd.DepthBiasClamp = D3D11_DEFAULT_DEPTH_BIAS_CLAMP;
			rd.SlopeScaledDepthBias = D3D11_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
			rd.DepthClipEnable = TRUE;
			rd.ScissorEnable = FALSE;
			rd.MultisampleEnable = FALSE;
			rd.AntialiasedLineEnable = FALSE;
		}
		HRESULT hr = m_Device->CreateRasterizerState(&rd, rs.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create rasterizer state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateSamplerState(Microsoft::WRL::ComPtr<ID3D11SamplerState>& ss, const RSamplerState& samplerState)
	{
		D3D11_SAMPLER_DESC sd;
		{
			::ZeroMemory(&sd, sizeof(sd));
			RDeviceD3D11::TranslateSamplerState(sd, samplerState);
		}
		HRESULT hr = m_Device->CreateSamplerState(&sd, ss.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create sampler state failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::CreateQuery(Microsoft::WRL::ComPtr<ID3D11Query>& q, const RQueryDesc& queryDesc)
	{
		D3D11_QUERY_DESC qd;
		{
			::ZeroMemory(&qd, sizeof(qd));
			RDeviceD3D11::TranslateQueryDesc(qd, queryDesc);
		}
		HRESULT hr = m_Device->CreateQuery(&qd, q.ReleaseAndGetAddressOf());
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create query failed."));
			return FALSE;
		}
		return TRUE;
	}
	BOOL32 RDeviceD3D11::GetData(ID3D11Asynchronous* pAsync, void* output, const UINT32& size, RAsyncGetDataFlagType flag)
	{
		UINT32 getDataFlag = 0u;
		RDeviceD3D11::TranslateGetDataFlag(getDataFlag, flag);
		HRESULT hr = m_ImmediateContext->GetData(pAsync, output, size, getDataFlag);
		return (!(FAILED(hr)));
	}
	void RDeviceD3D11::Begin(ID3D11Asynchronous* pAsync)
	{
		m_ImmediateContext->Begin(pAsync);
	}
	void RDeviceD3D11::End(ID3D11Asynchronous* pAsync)
	{
		m_ImmediateContext->End(pAsync);
	}
	BOOL32 RDeviceD3D11::Map(const RStructuredBuffer& input, const UINT32& indexSubResource, RResourceMapType mapType, RResourceMapFlagType mapFlag, RMappedResource& output)
	{
		{
			if (!(input.AccessMapRead || input.AccessMapWrite))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer check failed (error input access flag)."));
				return FALSE;
			}
			BOOL32 needRead = mapType == RResourceMapType::RESOURCE_MAP_READ || mapType == RResourceMapType::RESOURCE_MAP_READ_WRITE;
			BOOL32 needWrite = mapType == RResourceMapType::RESOURCE_MAP_WRITE || mapType == RResourceMapType::RESOURCE_MAP_READ_WRITE || mapType == RResourceMapType::RESOURCE_MAP_WRITE_DISCARD || mapType == RResourceMapType::RESOURCE_MAP_WRITE_NO_OVERWRITE;
			if (input.AccessMapRead != needRead || input.AccessMapWrite != needWrite)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer check failed (error access flag)."));
				return FALSE;
			}
		}
		D3D11_MAPPED_SUBRESOURCE ms;
		{
			UINT32 d3dMapFlag; D3D11_MAP d3dMapType;
			::ZeroMemory(&ms, sizeof(ms));
			RDeviceD3D11::TranslateResourceMapType(d3dMapType, mapType);
			RDeviceD3D11::TranslateResourceMapFlag(d3dMapFlag, mapFlag);
			HRESULT hr = m_ImmediateContext->Map(
				input.Buffer.Get(),
				indexSubResource,
				d3dMapType,
				d3dMapFlag,
				&ms);
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Mapping resource buffer access failed."));
				return FALSE;
			}
		}
		output.pData = ms.pData;
		output.RowPitch = ms.RowPitch;
		output.DepthPitch = ms.DepthPitch;
		return TRUE;
	}
	void RDeviceD3D11::Unmap(const RStructuredBuffer& input, const UINT32& indexSubResource)
	{
		m_ImmediateContext->Unmap(input.Buffer.Get(), indexSubResource);
	}
	void RDeviceD3D11::TranslateBindFlag(UINT32& output, UINT8 input)
	{
		static RBindFlagType bindFlagTypes[] =
		{
			RBindFlagType::BIND_VERTEX_BUFFER,
			RBindFlagType::BIND_INDEX_BUFFER,
			RBindFlagType::BIND_CONSTANT_BUFFER,
			RBindFlagType::BIND_SHADER_RESOURCE,
			RBindFlagType::BIND_STREAM_OUTPUT,
			RBindFlagType::BIND_RENDER_TARGET,
			RBindFlagType::BIND_DEPTH_STENCIL,
			RBindFlagType::BIND_UNORDERED_ACCESS
		};
		static UINT32 d3dBindFlagTypes[] =
		{
			D3D11_BIND_FLAG::D3D11_BIND_VERTEX_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_INDEX_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_CONSTANT_BUFFER,
			D3D11_BIND_FLAG::D3D11_BIND_SHADER_RESOURCE,
			D3D11_BIND_FLAG::D3D11_BIND_STREAM_OUTPUT,
			D3D11_BIND_FLAG::D3D11_BIND_RENDER_TARGET,
			D3D11_BIND_FLAG::D3D11_BIND_DEPTH_STENCIL,
			D3D11_BIND_FLAG::D3D11_BIND_UNORDERED_ACCESS
		};
		output = 0u;
		if (input == RBindFlagType::BIND_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 8u; i++)
		{
			if ((input & bindFlagTypes[i]) != 0)
			{
				output |= d3dBindFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateUsageFlag(D3D11_USAGE& output, RUsageFlagType input)
	{
		static TMap<RUsageFlagType, D3D11_USAGE> usageFlagMap =
		{
			{ RUsageFlagType::USAGE_DEFAULT		, D3D11_USAGE::D3D11_USAGE_DEFAULT },
			{ RUsageFlagType::USAGE_IMMUTABLE	, D3D11_USAGE::D3D11_USAGE_IMMUTABLE },
			{ RUsageFlagType::USAGE_DYNAMIC		, D3D11_USAGE::D3D11_USAGE_DYNAMIC },
			{ RUsageFlagType::USAGE_STAGING		, D3D11_USAGE::D3D11_USAGE_STAGING }
		};
		output = usageFlagMap[input];
	}
	void RDeviceD3D11::TranslateCPUAccessFlag(UINT32& output, RCPUAccessFlagType input)
	{
		static RCPUAccessFlagType CPUAccessFlagTypes[] =
		{
			RCPUAccessFlagType::CPU_ACCESS_READ,
			RCPUAccessFlagType::CPU_ACCESS_WRITE
		};
		static UINT32 d3dCPUAccessFlagTypes[] =
		{
			D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_READ,
			D3D11_CPU_ACCESS_FLAG::D3D11_CPU_ACCESS_WRITE
		};
		output = 0u;
		if (input == RCPUAccessFlagType::CPU_ACCESS_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & CPUAccessFlagTypes[i]) != 0)
			{
				output |= d3dCPUAccessFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceMiscFlag(UINT32& output, RResourceMiscFlagType input)
	{
		static RResourceMiscFlagType miscFlagTypes[] =
		{
			RResourceMiscFlagType::RESOURCE_MISC_GENERATE_MIPS,
			RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE,
			RResourceMiscFlagType::RESOURCE_MISC_DRAWINDIRECT_ARGS,
			RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED,
			RResourceMiscFlagType::RESOURCE_MISC_RESOURCE_CLAMP
		};
		static UINT32 d3dMiscFlagTypes[] =
		{
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_GENERATE_MIPS,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_TEXTURECUBE,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
			D3D11_RESOURCE_MISC_FLAG::D3D11_RESOURCE_MISC_RESOURCE_CLAMP
		};
		output = 0u;
		if (input == RResourceMiscFlagType::RESOURCE_MISC_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 5u; i++)
		{
			if ((input & miscFlagTypes[i]) != 0)
			{
				output |= d3dMiscFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceFormat(DXGI_FORMAT& output, RFormatType input)
	{
		// The table is not in this file: it is shared with the D3D12 back-end
		// (RenderDevice/DXGIFormatTranslate), so the two APIs cannot drift apart on which
		// format they think a format is. A format the table does not carry is reported
		// there instead of turning into DXGI_FORMAT_UNKNOWN in silence.
		RDXGIFormatTranslate::TranslateToDXGI(output, input);
	}
	void RDeviceD3D11::TranslateUAVFlag(UINT32& output, RUAVFlagType input)
	{
		static RUAVFlagType UAVFlagTypes[] =
		{
			RUAVFlagType::UAV_FLAG_RAW,
			RUAVFlagType::UAV_FLAG_APPEND,
			RUAVFlagType::UAV_FLAG_COUNTER
		};
		static UINT32 d3dUAVFlagTypes[] =
		{
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_RAW,
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_APPEND,
			D3D11_BUFFER_UAV_FLAG::D3D11_BUFFER_UAV_FLAG_COUNTER
		};
		output = 0u;
		if (input == RUAVFlagType::UAV_FLAG_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & UAVFlagTypes[i]) != 0)
			{
				output |= d3dUAVFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateComparisonFunction(D3D11_COMPARISON_FUNC& output, RComparisonFunctionType input)
	{
		static TMap<RComparisonFunctionType, D3D11_COMPARISON_FUNC> comparisonFuncMap =
		{
			{ RComparisonFunctionType::COMPARISON_NEVER			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NEVER },
			{ RComparisonFunctionType::COMPARISON_LESS			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS },
			{ RComparisonFunctionType::COMPARISON_EQUAL			, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_EQUAL },
			{ RComparisonFunctionType::COMPARISON_LESS_EQUAL	, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_LESS_EQUAL },
			{ RComparisonFunctionType::COMPARISON_GREATER		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER },
			{ RComparisonFunctionType::COMPARISON_NOT_EQUAL		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_NOT_EQUAL },
			{ RComparisonFunctionType::COMPARISON_GREATER_EQUAL	, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_GREATER_EQUAL },
			{ RComparisonFunctionType::COMPARISON_ALWAYS		, D3D11_COMPARISON_FUNC::D3D11_COMPARISON_ALWAYS }
		};
		output = comparisonFuncMap[input];
	}
	void RDeviceD3D11::TranslateStencilStateType(D3D11_DEPTH_STENCILOP_DESC& output, const RStencilStateType& input)
	{
		static TMap<RStencilOperationType, D3D11_STENCIL_OP> stencilOpMap =
		{
			{ RStencilOperationType::STENCIL_OP_KEEP		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_KEEP },
			{ RStencilOperationType::STENCIL_OP_ZERO		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_ZERO },
			{ RStencilOperationType::STENCIL_OP_REPLACE		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_REPLACE },
			{ RStencilOperationType::STENCIL_OP_INCR_SAT	, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR_SAT },
			{ RStencilOperationType::STENCIL_OP_DECR_SAT	, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR_SAT },
			{ RStencilOperationType::STENCIL_OP_INVERT		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INVERT },
			{ RStencilOperationType::STENCIL_OP_INCR		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_INCR },
			{ RStencilOperationType::STENCIL_OP_DECR		, D3D11_STENCIL_OP::D3D11_STENCIL_OP_DECR }
		};
		output.StencilFailOp = stencilOpMap[input.StencilFailOp];
		output.StencilDepthFailOp = stencilOpMap[input.StencilDepthFailOp];
		output.StencilPassOp = stencilOpMap[input.StencilPassOp];
		RDeviceD3D11::TranslateComparisonFunction(output.StencilFunc, input.StencilFunc);
	}
	void RDeviceD3D11::TranslateDepthStencilState(D3D11_DEPTH_STENCIL_DESC& output, const RDepthState& depthInput, const RStencilState* stencilInput)
	{
		static TMap<RDepthWriteMaskType, D3D11_DEPTH_WRITE_MASK> depthWriteMap =
		{
			{ RDepthWriteMaskType::DEPTH_WRITE_MASK_ZERO	, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ZERO },
			{ RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL		, D3D11_DEPTH_WRITE_MASK::D3D11_DEPTH_WRITE_MASK_ALL }
		};
		output.DepthEnable = depthInput.DepthEnable;
		output.DepthWriteMask = depthWriteMap[depthInput.DepthWriteMask];
		RDeviceD3D11::TranslateComparisonFunction(output.DepthFunc, depthInput.DepthFunc);
		output.StencilEnable = FALSE;
		if (stencilInput)
		{
			output.StencilEnable = TRUE;
			output.StencilReadMask = static_cast<UINT8>(stencilInput->StencilReadMask & 0xffu);
			output.StencilWriteMask = static_cast<UINT8>(stencilInput->StencilWriteMask & 0xffu);
			RDeviceD3D11::TranslateStencilStateType(output.FrontFace, stencilInput->FrontFace);
			RDeviceD3D11::TranslateStencilStateType(output.BackFace, stencilInput->BackFace);
		}
	}
	void RDeviceD3D11::TranslateBlendState(D3D11_RENDER_TARGET_BLEND_DESC& output, const RBlendState& input)
	{
		static TMap<RBlendOptionType, D3D11_BLEND> blendMap =
		{
			{ RBlendOptionType::BLEND_ZERO				, D3D11_BLEND::D3D11_BLEND_ZERO },
			{ RBlendOptionType::BLEND_ONE				, D3D11_BLEND::D3D11_BLEND_ONE },
			{ RBlendOptionType::BLEND_SRC_COLOR			, D3D11_BLEND::D3D11_BLEND_SRC_COLOR },
			{ RBlendOptionType::BLEND_INV_SRC_COLOR		, D3D11_BLEND::D3D11_BLEND_INV_SRC_COLOR },
			{ RBlendOptionType::BLEND_SRC_ALPHA			, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA },
			{ RBlendOptionType::BLEND_INV_SRC_ALPHA		, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA },
			{ RBlendOptionType::BLEND_DEST_ALPHA		, D3D11_BLEND::D3D11_BLEND_DEST_ALPHA },
			{ RBlendOptionType::BLEND_INV_DEST_ALPHA	, D3D11_BLEND::D3D11_BLEND_INV_DEST_ALPHA },
			{ RBlendOptionType::BLEND_DEST_COLOR		, D3D11_BLEND::D3D11_BLEND_DEST_COLOR },
			{ RBlendOptionType::BLEND_INV_DEST_COLOR	, D3D11_BLEND::D3D11_BLEND_INV_DEST_COLOR },
			{ RBlendOptionType::BLEND_SRC_ALPHA_SAT		, D3D11_BLEND::D3D11_BLEND_SRC_ALPHA_SAT },
			{ RBlendOptionType::BLEND_BLEND_FACTOR		, D3D11_BLEND::D3D11_BLEND_BLEND_FACTOR },
			{ RBlendOptionType::BLEND_INV_BLEND_FACTOR	, D3D11_BLEND::D3D11_BLEND_INV_BLEND_FACTOR }
		};
		static TMap<RBlendOperationType, D3D11_BLEND_OP> blendOpMap =
		{
			{ RBlendOperationType::BLEND_OP_ADD				, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD },
			{ RBlendOperationType::BLEND_OP_SUBTRACT		, D3D11_BLEND_OP::D3D11_BLEND_OP_SUBTRACT },
			{ RBlendOperationType::BLEND_OP_REV_SUBTRACT	, D3D11_BLEND_OP::D3D11_BLEND_OP_REV_SUBTRACT },
			{ RBlendOperationType::BLEND_OP_MIN				, D3D11_BLEND_OP::D3D11_BLEND_OP_MIN },
			{ RBlendOperationType::BLEND_OP_MAX				, D3D11_BLEND_OP::D3D11_BLEND_OP_MAX }
		};
		static RColorWriteMaskType colorWriteMaskTypes[] =
		{
			RColorWriteMaskType::COLOR_WRITE_MASK_RED,
			RColorWriteMaskType::COLOR_WRITE_MASK_GREEN,
			RColorWriteMaskType::COLOR_WRITE_MASK_BLUE,
			RColorWriteMaskType::COLOR_WRITE_MASK_ALPHA
		};
		static D3D11_COLOR_WRITE_ENABLE d3dColorWriteMaskTypes[] =
		{
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_RED,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_GREEN,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_BLUE,
			D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALPHA
		};
		output.BlendEnable = input.BlendEnable;
		output.SrcBlend = blendMap[input.SrcBlend];
		output.DestBlend = blendMap[input.DstBlend];
		output.BlendOp = blendOpMap[input.BlendOp];
		output.SrcBlendAlpha = blendMap[input.SrcBlendAlpha];
		output.DestBlendAlpha = blendMap[input.DstBlendAlpha];
		output.BlendOpAlpha = blendOpMap[input.BlendOpAlpha];
		if (input.RenderTargetWriteMask == (RColorWriteMaskType::COLOR_WRITE_MASK_ALL))
		{
			output.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL;
		}
		else
		{
			output.RenderTargetWriteMask = 0u;
			for (UINT32 i = 0u; i < 4u; i++)
			{
				if ((input.RenderTargetWriteMask & colorWriteMaskTypes[i]) != 0)
				{
					output.RenderTargetWriteMask |= d3dColorWriteMaskTypes[i];
				}
			}
		}
	}
	void RDeviceD3D11::TranslateRasterizerState(D3D11_CULL_MODE& outputCull, D3D11_FILL_MODE& outputFill, const RRasterizerState& input)
	{
		static TMap<RCullModeType, D3D11_CULL_MODE> cullMap =
		{
			{ RCullModeType::CULL_NONE	, D3D11_CULL_MODE::D3D11_CULL_NONE },
			{ RCullModeType::CULL_FRONT	, D3D11_CULL_MODE::D3D11_CULL_FRONT },
			{ RCullModeType::CULL_BACK	, D3D11_CULL_MODE::D3D11_CULL_BACK }
		};
		static TMap<RFillModeType, D3D11_FILL_MODE> fillMap =
		{
			{ RFillModeType::FILL_WIREFRAME	, D3D11_FILL_MODE::D3D11_FILL_WIREFRAME },
			{ RFillModeType::FILL_SOLID		, D3D11_FILL_MODE::D3D11_FILL_SOLID }
		};
		outputCull = cullMap[input.CullMode];
		outputFill = fillMap[input.FillMode];
	}
	void RDeviceD3D11::TranslateSamplerState(D3D11_SAMPLER_DESC& output, const RSamplerState& input)
	{
		static TMap<RFilterType, D3D11_FILTER> filterMap =
		{
			{ RFilterType::FILTER_POINT			, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_POINT },
			{ RFilterType::FILTER_LINEAR		, D3D11_FILTER::D3D11_FILTER_MIN_MAG_MIP_LINEAR },
			{ RFilterType::FILTER_ANISOTROPIC	, D3D11_FILTER::D3D11_FILTER_ANISOTROPIC }
		};
		static TMap<RTextureAddressModeType, D3D11_TEXTURE_ADDRESS_MODE> textureAddressModeMap =
		{
			{ RTextureAddressModeType::TEXTURE_ADDRESS_WRAP			, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_WRAP },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_MIRROR		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_CLAMP },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_BORDER		, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_BORDER },
			{ RTextureAddressModeType::TEXTURE_ADDRESS_MIRROR_ONCE	, D3D11_TEXTURE_ADDRESS_MODE::D3D11_TEXTURE_ADDRESS_MIRROR_ONCE }
		};
		output.Filter = filterMap[input.Filter];
		output.AddressU = textureAddressModeMap[input.AddressU];
		output.AddressV = textureAddressModeMap[input.AddressV];
		output.AddressW = textureAddressModeMap[input.AddressW];
		output.MipLODBias = input.MipLODBias;
		output.MaxAnisotropy = input.MaxAnisotropy;
		// D3D11 honors a comparison function only for the comparison filter variants, and the
		// engine filter enum has none, so a non-NEVER function would be dropped silently here
		// (the D3D11 debug layer reports the same ignored ComparisonFunc). The desc is the
		// caller's to get right.
		PE_CHECK((ENGINE_RENDER_CORE_ERROR),
			(PE_TEXT("Sampler comparison function is set on a filter without a comparison variant.")),
			(input.ComparisonFunc == RComparisonFunctionType::COMPARISON_NEVER));
		RDeviceD3D11::TranslateComparisonFunction(output.ComparisonFunc, input.ComparisonFunc);
		output.BorderColor[0] = input.BorderColor.r;
		output.BorderColor[1] = input.BorderColor.g;
		output.BorderColor[2] = input.BorderColor.b;
		output.BorderColor[3] = input.BorderColor.a;
		output.MinLOD = input.MinLOD;
		output.MaxLOD = input.MaxLOD;
	}
	void RDeviceD3D11::TranslateClearDepthStencilFlag(UINT32& output, const UINT32& input)
	{
		static RClearDepthStencilFlagType clearDepthStencilFlagTypes[] =
		{
			RClearDepthStencilFlagType::CLEAR_DEPTH,
			RClearDepthStencilFlagType::CLEAR_STENCIL
		};
		static UINT32 d3dClearDepthStencilFlagTypes[] =
		{
			D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH,
			D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL,
		};
		output = 0u;
		if (input == RClearDepthStencilFlagType::CLEAR_NONE)
		{
			return;
		}
		for (UINT32 i = 0u; i < 2u; i++)
		{
			if ((input & clearDepthStencilFlagTypes[i]) != 0)
			{
				output |= d3dClearDepthStencilFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslatePrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY& output, RPrimitiveTopologyType input)
	{
		static TMap<RPrimitiveTopologyType, D3D_PRIMITIVE_TOPOLOGY> primitiveTopologyMap =
		{
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_UNDEFINED		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_POINTLIST		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_POINTLIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINELIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINESTRIP		, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST	, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST },
			{ RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLESTRIP	, D3D_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP }
		};
		output = primitiveTopologyMap[input];
	}
	void RDeviceD3D11::TranslateQueryDesc(D3D11_QUERY_DESC& output, const RQueryDesc& input)
	{
		static TMap<RQueryType, D3D11_QUERY> queryTypeMap =
		{
			{ RQueryType::QUERY_EVENT							, D3D11_QUERY::D3D11_QUERY_EVENT },
			{ RQueryType::QUERY_OCCLUSION						, D3D11_QUERY::D3D11_QUERY_OCCLUSION },
			{ RQueryType::QUERY_TIMESTAMP						, D3D11_QUERY::D3D11_QUERY_TIMESTAMP },
			{ RQueryType::QUERY_TIMESTAMP_DISJOINT				, D3D11_QUERY::D3D11_QUERY_TIMESTAMP_DISJOINT },
			{ RQueryType::QUERY_PIPELINE_STATISTICS				, D3D11_QUERY::D3D11_QUERY_PIPELINE_STATISTICS },
			{ RQueryType::QUERY_OCCLUSION_PREDICATE				, D3D11_QUERY::D3D11_QUERY_OCCLUSION_PREDICATE },
			{ RQueryType::QUERY_SO_STATISTICS					, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE			, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM0			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM0 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM0	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM0 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM1			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM1 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM1	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM1 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM2			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM2 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM2	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM2 },
			{ RQueryType::QUERY_SO_STATISTICS_STREAM3			, D3D11_QUERY::D3D11_QUERY_SO_STATISTICS_STREAM3 },
			{ RQueryType::QUERY_SO_OVERFLOW_PREDICATE_STREAM3	, D3D11_QUERY::D3D11_QUERY_SO_OVERFLOW_PREDICATE_STREAM3 }
		};
		static RQueryMiscFlagType queryMiscFlagTypes[] =
		{
			RQueryMiscFlagType::QUERY_MISC_PREDICATEHINT
		};
		static UINT32 d3dQueryMiscFlagTypes[] =
		{
			D3D11_QUERY_MISC_FLAG::D3D11_QUERY_MISC_PREDICATEHINT
		};
		output.Query = queryTypeMap[input.QueryType];
		output.MiscFlags = 0u;
		if (input.MiscFlags == RQueryMiscFlagType::QUERY_MISC_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 1u; i++)
		{
			if ((input.MiscFlags & queryMiscFlagTypes[i]) != 0)
			{
				output.MiscFlags |= d3dQueryMiscFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateGetDataFlag(UINT32& output, RAsyncGetDataFlagType input)
	{
		static RAsyncGetDataFlagType asyncGetDataFlagTypes[] =
		{
			RAsyncGetDataFlagType::ASYNC_GETDATA_DONOTFLUSH
		};
		static UINT32 d3dAsyncGetDataFlagTypes[] =
		{
			D3D11_ASYNC_GETDATA_FLAG::D3D11_ASYNC_GETDATA_DONOTFLUSH
		};
		output = 0u;
		if (input == RAsyncGetDataFlagType::ASYNC_GETDATA_DEFAULT)
		{
			return;
		}
		for (UINT32 i = 0u; i < 1u; i++)
		{
			if ((input & asyncGetDataFlagTypes[i]) != 0)
			{
				output |= d3dAsyncGetDataFlagTypes[i];
			}
		}
	}
	void RDeviceD3D11::TranslateResourceMapType(D3D11_MAP& output, RResourceMapType input)
	{
		static TMap<RResourceMapType, D3D11_MAP> resourceMapTypes =
		{
			{ RResourceMapType::RESOURCE_MAP_READ				, D3D11_MAP::D3D11_MAP_READ },
			{ RResourceMapType::RESOURCE_MAP_WRITE				, D3D11_MAP::D3D11_MAP_WRITE },
			{ RResourceMapType::RESOURCE_MAP_READ_WRITE			, D3D11_MAP::D3D11_MAP_READ_WRITE },
			{ RResourceMapType::RESOURCE_MAP_WRITE_DISCARD		, D3D11_MAP::D3D11_MAP_WRITE_DISCARD },
			{ RResourceMapType::RESOURCE_MAP_WRITE_NO_OVERWRITE	, D3D11_MAP::D3D11_MAP_WRITE_NO_OVERWRITE }
		};
		output = resourceMapTypes[input];
	}
	void RDeviceD3D11::TranslateResourceMapFlag(UINT32& output, RResourceMapFlagType input)
	{
		static TMap<RResourceMapFlagType, UINT32> resourceMapFlagTypes =
		{
			{ RResourceMapFlagType::RESOURCE_MAP_FLAG_NONE			, 0u },
			{ RResourceMapFlagType::RESOURCE_MAP_FLAG_DO_NOT_WAIT	, D3D11_MAP_FLAG::D3D11_MAP_FLAG_DO_NOT_WAIT }
		};
		output = resourceMapFlagTypes[input];
	}
	void RDeviceD3D11::TranslateInputLayoutDesc(D3D11_INPUT_ELEMENT_DESC& output, const RInputLayoutDesc& input)
	{
		static TMap<RInputClassificationType, D3D11_INPUT_CLASSIFICATION> classificationMap =
		{
			{ RInputClassificationType::INPUT_PER_VERTEX_DATA	, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_VERTEX_DATA },
			{ RInputClassificationType::INPUT_PER_INSTANCE_DATA	, D3D11_INPUT_CLASSIFICATION::D3D11_INPUT_PER_INSTANCE_DATA }
		};
		::ZeroMemory(&output, sizeof(output));

		enum ESemanticIndexType : UINT8
		{
			SEMANTIC_INDEX_PSIZE		= 0,
			SEMANTIC_INDEX_POSITIONT,
			SEMANTIC_INDEX_BINORMAL,
			SEMANTIC_INDEX_BLENDWEIGHT,
			SEMANTIC_INDEX_BLENDINDICES,
			SEMANTIC_INDEX_COLOR,
			SEMANTIC_INDEX_TANGENT,
			SEMANTIC_INDEX_NORMAL,
			SEMANTIC_INDEX_POSITION,
			SEMANTIC_INDEX_TEXCOORD,
			SEMANTIC_INDEX_COUNT
		};
		static const CHAR* StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_COUNT] =
		{
			"PSIZE",
			"POSITIONT",
			"BINORMAL",
			"BLENDWEIGHT",
			"BLENDINDICES",
			"COLOR",
			"TANGENT",
			"NORMAL",
			"POSITION",
			"TEXCOORD"
		};

		auto TranslateMemberToFormat = [](const RInputLayoutDesc& InDesc)->DXGI_FORMAT
		{
			CheckSlow(((InDesc.MemberFormat >= RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT) && (InDesc.MemberFormat < RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_COUNT)), (ENGINE_RENDER_CORE_ERROR));
			CheckSlow(((InDesc.MemberStride > 0u) && (InDesc.MemberStride <= 4u) && (InDesc.MemberStride != 3u)), (ENGINE_RENDER_CORE_ERROR));
			CheckSlow(((InDesc.MemberNum > 0u) && (InDesc.MemberNum <= 4u)), (ENGINE_RENDER_CORE_ERROR));
			if (InDesc.MemberNum == 1u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 2u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_SNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 3u)
			{
				if (InDesc.MemberStride == 1u)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Engine does not support [rgb8 type] this format."));
				}
				else if (InDesc.MemberStride == 2u)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Engine does not support [rgb16 type] this format."));
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32_UINT;
						}
					}
				}
			}
			else if (InDesc.MemberNum == 4u)
			{
				if (InDesc.MemberStride == 1u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R8G8B8A8_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 2u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UNORM:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UNORM;
						}
					}
				}
				else if (InDesc.MemberStride == 4u)
				{
					switch (InDesc.MemberFormat)
					{
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_INT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_SINT;
						}
					case RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_UINT:
						{
							return DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_UINT;
						}
					}
				}
			}
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Can not find this input layout member format."));
			return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
		};

		if (input.SemanticName == RShaderSemanticType::SHADER_SEMANTIC_NONE) { return; }
		{
			EString strSemanticName; UINT32 semanticName = input.SemanticName;
			output.SemanticIndex = semanticName & 0xfu;
			output.Format = TranslateMemberToFormat(input);
			if ((semanticName >> 15) & 0x1u)
			{
				//SHADER_SEMANTIC_TEXCOORD[n]
				//strSemanticName = "TEXCOORD";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_TEXCOORD];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32_FLOAT;
			}
			else if ((semanticName >> 14) & 0x1u)
			{
				//SHADER_SEMANTIC_POSITION[n]
				//strSemanticName = "POSITION";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_POSITION];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 13) & 0x1u)
			{
				//SHADER_SEMANTIC_NORMAL[n]
				//strSemanticName = "NORMAL";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_NORMAL];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 12) & 0x1u)
			{
				//SHADER_SEMANTIC_TANGENT[n]
				//strSemanticName = "TANGENT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_TANGENT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 11) & 0x1u)
			{
				//SHADER_SEMANTIC_COLOR[n]
				//strSemanticName = "COLOR";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_COLOR];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 10) & 0x1u)
			{
				//SHADER_SEMANTIC_BLENDINDICES[n]
				//strSemanticName = "BLENDINDICES";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BLENDINDICES];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R16G16B16A16_UINT;
			}
			else if ((semanticName >> 9) & 0x1u)
			{
				//SHADER_SEMANTIC_BLENDWEIGHT[n]
				//strSemanticName = "BLENDWEIGHT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BLENDWEIGHT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 8) & 0x1u)
			{
				//SHADER_SEMANTIC_BINORMAL[n]
				//strSemanticName = "BINORMAL";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_BINORMAL];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 7) & 0x1u)
			{
				//SHADER_SEMANTIC_POSITIONT[n]
				//strSemanticName = "POSITIONT";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_POSITIONT];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
			else if ((semanticName >> 6) & 0x1u)
			{
				//SHADER_SEMANTIC_PSIZE[n]
				//strSemanticName = "PSIZE";
				output.SemanticName = StaticSemanticIndexName[ESemanticIndexType::SEMANTIC_INDEX_PSIZE];
				//output.Format = DXGI_FORMAT::DXGI_FORMAT_R32G32B32A32_FLOAT;
			}
		}
		output.InputSlot = input.InputSlot;
		// The caller may declare where the element sits inside its vertex, which is what an
		// interleaved or padded stream needs; the sentinel means the API's own appending rule.
		output.AlignedByteOffset = ((input.UsesAppendOffset() != FALSE)
			? D3D11_APPEND_ALIGNED_ELEMENT : static_cast<UINT>(input.AlignedByteOffset));
		output.InputSlotClass = classificationMap[input.InputSlotClass];
		output.InstanceDataStepRate = input.InstanceDataStepRate;
	}
	void RDeviceD3D11::ClearFinalOutput()
	{
		m_ImmediateContext->ClearRenderTargetView(m_RenderTargetView.Get(), Color4(0.5f, 0.5f, 0.5f, 1.f).rgba);
		m_ImmediateContext->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_FLAG::D3D11_CLEAR_DEPTH | D3D11_CLEAR_FLAG::D3D11_CLEAR_STENCIL, 1.f, 0xffu);
	}
	void RDeviceD3D11::SetFinalOutput()
	{
		m_ImmediateContext->OMSetRenderTargets(1u, m_RenderTargetView.GetAddressOf(), m_DepthStencilView.Get());
		m_ImmediateContext->RSSetViewports(1u, &(m_Viewport));
	}

	// ============================================================
	// IRRHIDevice interface implementation
	//
	// Most methods delegate to the legacy direct-device APIs above.
	// Resource-creating methods heap-allocate the appropriate
	// concrete D3D11 resource class (which inherits IRD3D11xxx) and
	// return it through the abstract IRRHIxxx out parameter.
	// ============================================================

	BOOL8 RDeviceD3D11::Initialize(const RRHIDeviceInitDesc& InDesc) noexcept
	{
		SetInitializeData(InDesc.WindowHandle, Vector2Int(static_cast<INT32>(InDesc.BackBufferWidth), static_cast<INT32>(InDesc.BackBufferHeight)),
			24u, InDesc.RefreshRateHz, InDesc.bIsWindowed);
		// Call the legacy no-arg Initialize override (resolved via overload).
		this->RDeviceD3D11::Initialize();

		FrameInFlightCount	= (InDesc.BackBufferCount > 0u) ? InDesc.BackBufferCount : 2u;
		CurrentFrameIndex	= 0u;

		// Populate the engine-side backbuffer wrapper from the swapchain.
		if (m_SwapChain)
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBufferTex;
			HRESULT hr = m_SwapChain->GetBuffer(0u, IID_PPV_ARGS(BackBufferTex.GetAddressOf()));
			if (SUCCEEDED(hr))
			{
				BackBufferWrapper.Buffer			= BackBufferTex;
				BackBufferWrapper.RenderTargetView	= m_RenderTargetView;
				BackBufferWrapper.Width				= InDesc.BackBufferWidth;
				BackBufferWrapper.Height			= InDesc.BackBufferHeight;
				BackBufferWrapper.MipLevels			= 1u;
				BackBufferWrapper.ArraySize			= 1u;
				BackBufferWrapper.Format			= InDesc.BackBufferFormat;
				BackBufferWrapper.BindFlags			= static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET);
			}
		}
		return TRUE;
	}

	void RDeviceD3D11::Shutdown() noexcept
	{
		WaitForGPUIdle();
		CommandListFreeList.Empty();
		CommandListPool.Empty();		// TUniquePtr destructors release each command list
		BindlessSRVTable.Empty();
		BindlessUAVTable.Empty();
		BindlessSRVFreeIndices.Empty();
		BindlessUAVFreeIndices.Empty();
		// Root signatures are back-end objects with no native counterpart here, so they are
		// simply released.
		for (RD3D11RootSignature* Signature : RootSignaturePool)
		{
			delete Signature;
		}
		RootSignaturePool.Empty();
		RootSignatureCache.Empty();
		BackBufferWrapper.ReleaseRenderResource();
		this->ShutDown();
	}

	void RDeviceD3D11::BeginFrame() noexcept
	{
		// D3D11 has no per-frame fence wait; nothing to do.
	}

	void RDeviceD3D11::EndFrame() noexcept
	{
		CurrentFrameIndex = (CurrentFrameIndex + 1u) % ((FrameInFlightCount > 0u) ? FrameInFlightCount : 1u);
	}

	BOOL8 RDeviceD3D11::ResizeSwapChain(UINT32 InWidth, UINT32 InHeight) noexcept
	{
		if (!m_SwapChain || !m_Device || !m_ImmediateContext || InWidth == 0u || InHeight == 0u)
		{
			return FALSE;
		}

		// Flush any pending GPU work, then reset ALL context state to release
		// every internal COM AddRef (RTVs, DSV, SRVs, etc.) before ResizeBuffers.
		m_ImmediateContext->Flush();
		m_ImmediateContext->ClearState();

		// Release all references to back-buffer resources
		BackBufferWrapper.ReleaseRenderResource();
		BackBufferWrapper.Buffer.Reset();   // must release the back-buffer texture itself before ResizeBuffers
		m_RenderTargetView.Reset();
		m_DepthStencilView.Reset();
		m_DepthTexture.Reset();

		// Resize swap-chain buffers (0 = keep existing count and format)
		HRESULT hr = m_SwapChain->ResizeBuffers(0, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0u);
		if (FAILED(hr))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::ResizeSwapChain - ResizeBuffers failed."));
			return FALSE;
		}

		// Re-acquire back buffer and recreate RTV
		{
			Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBufferTex;
			hr = m_SwapChain->GetBuffer(0u, IID_PPV_ARGS(BackBufferTex.GetAddressOf()));
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::ResizeSwapChain - GetBuffer failed."));
				return FALSE;
			}
			hr = m_Device->CreateRenderTargetView(BackBufferTex.Get(), nullptr, m_RenderTargetView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::ResizeSwapChain - CreateRenderTargetView failed."));
				return FALSE;
			}
			BackBufferWrapper.Buffer			= BackBufferTex;
			BackBufferWrapper.RenderTargetView	= m_RenderTargetView;
			BackBufferWrapper.Width				= InWidth;
			BackBufferWrapper.Height			= InHeight;
			BackBufferWrapper.MipLevels			= 1u;
			BackBufferWrapper.ArraySize			= 1u;
			BackBufferWrapper.BindFlags			= static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET);
		}

		// Recreate depth texture and DSV at new size
		{
			D3D11_TEXTURE2D_DESC DepthDesc = {};
			DepthDesc.Width          = InWidth;
			DepthDesc.Height         = InHeight;
			DepthDesc.MipLevels      = 1u;
			DepthDesc.ArraySize      = 1u;
			DepthDesc.Format         = DXGI_FORMAT_R24G8_TYPELESS;
			DepthDesc.SampleDesc     = { 1u, 0u };
			DepthDesc.Usage          = D3D11_USAGE_DEFAULT;
			DepthDesc.BindFlags      = D3D11_BIND_DEPTH_STENCIL;
			DepthDesc.CPUAccessFlags = 0u;
			DepthDesc.MiscFlags      = 0u;
			hr = m_Device->CreateTexture2D(&DepthDesc, nullptr, m_DepthTexture.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::ResizeSwapChain - CreateTexture2D (depth) failed."));
				return FALSE;
			}

			D3D11_DEPTH_STENCIL_VIEW_DESC DsvDesc = {};
			DsvDesc.Format        = DXGI_FORMAT_D24_UNORM_S8_UINT;
			DsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
			DsvDesc.Flags         = 0u;
			hr = m_Device->CreateDepthStencilView(m_DepthTexture.Get(), &DsvDesc, m_DepthStencilView.ReleaseAndGetAddressOf());
			if (FAILED(hr))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::ResizeSwapChain - CreateDepthStencilView failed."));
				return FALSE;
			}
		}

		// Update the viewport to match the new dimensions
		m_Viewport.TopLeftX = 0.f;
		m_Viewport.TopLeftY = 0.f;
		m_Viewport.Width    = static_cast<FLOAT>(InWidth);
		m_Viewport.Height   = static_cast<FLOAT>(InHeight);
		m_Viewport.MinDepth = 0.f;
		m_Viewport.MaxDepth = 1.f;

		return TRUE;
	}

	void RDeviceD3D11::WaitForGPUIdle() noexcept
	{
		if (m_ImmediateContext)
		{
			m_ImmediateContext->Flush();
		}
	}

	/*
	* A second swap chain for an ImGui platform window.
	*
	* The device's main chain stays where it is (m_SwapChain + BackBufferWrapper): this one
	* only owns its own back buffer and RTV, so creating or resizing a viewport window can
	* never disturb the main window's frame. The wrapper is the same RRenderTexture2D the
	* main back buffer uses, which is also what IRCommandList::BeginRenderPass expects on
	* this back-end - the RTV is reached through IRD3D11Texture.
	*/
	class RD3D11WindowSwapChain final : public IRRHISwapChain
	{
	public:
		RD3D11WindowSwapChain(ID3D11Device* InDevice, const Microsoft::WRL::ComPtr<IDXGISwapChain>& InSwapChain, UINT32 InWidth, UINT32 InHeight)
			: Device(InDevice), SwapChain(InSwapChain), Width(InWidth), Height(InHeight)
		{
			CreateBackBuffer();
		}
		RD3D11WindowSwapChain(const RD3D11WindowSwapChain&) = delete;
		RD3D11WindowSwapChain& operator=(const RD3D11WindowSwapChain&) = delete;
		virtual ~RD3D11WindowSwapChain() override
		{
			BackBuffer.ReleaseRenderResource();
			SwapChain = nullptr;
			Device = nullptr;
		}

	public:
		virtual IRRHITexture* GetBackBuffer() override
		{
			return (BackBuffer.IsRenderResourceValid() != FALSE) ? static_cast<IRRHITexture*>(&BackBuffer) : nullptr;
		}
		virtual BOOL8 Resize(UINT32 InWidth, UINT32 InHeight) override
		{
			if ((!SwapChain) || (InWidth == 0u) || (InHeight == 0u))
			{
				return FALSE;
			}
			BackBuffer.ReleaseRenderResource();
			if (FAILED(SwapChain->ResizeBuffers(0u, InWidth, InHeight, DXGI_FORMAT_UNKNOWN, 0u)))
			{
				return FALSE;
			}
			Width = InWidth;
			Height = InHeight;
			CreateBackBuffer();
			return BackBuffer.IsRenderResourceValid();
		}
		virtual void Present(UINT32 InSyncInterval) override
		{
			if (SwapChain)
			{
				SwapChain->Present(InSyncInterval, 0u);
			}
		}
		BOOL8 IsValid() const
		{
			return BackBuffer.IsRenderResourceValid();
		}

	private:
		void CreateBackBuffer()
		{
			if ((!Device) || (!SwapChain))
			{
				return;
			}
			Microsoft::WRL::ComPtr<ID3D11Texture2D> BackBufferTexture;
			if (FAILED(SwapChain->GetBuffer(0u, IID_PPV_ARGS(BackBufferTexture.GetAddressOf()))))
			{
				return;
			}
			Microsoft::WRL::ComPtr<ID3D11RenderTargetView> RenderTargetView;
			if (FAILED(Device->CreateRenderTargetView(BackBufferTexture.Get(), nullptr, RenderTargetView.GetAddressOf())))
			{
				return;
			}
			BackBuffer.Buffer				= BackBufferTexture;
			BackBuffer.RenderTargetView		= RenderTargetView;
			BackBuffer.Width				= Width;
			BackBuffer.Height				= Height;
			BackBuffer.MipLevels			= 1u;
			BackBuffer.ArraySize			= 1u;
			BackBuffer.Format				= RFormatType::FORMAT_R8G8B8A8_UNORM;
			BackBuffer.BindFlags			= static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET);
		}

	private:
		Microsoft::WRL::ComPtr<ID3D11Device>	Device;
		Microsoft::WRL::ComPtr<IDXGISwapChain>	SwapChain;
		RRenderTexture2D						BackBuffer;
		UINT32									Width;
		UINT32									Height;
	};

	IRRHISwapChain* RDeviceD3D11::CreateSwapChainForWindow(HWND InWindowHandle, UINT32 InWidth, UINT32 InHeight, RFormatType InFormat) noexcept
	{
		if ((!m_Device) || (!m_SwapChain) || (!InWindowHandle) || (InWidth == 0u) || (InHeight == 0u))
		{
			return nullptr;
		}

		// The factory that made the main chain already targets the adapter this device runs
		// on, so a viewport chain made from it is on the same adapter by construction.
		Microsoft::WRL::ComPtr<IDXGIFactory> Factory;
		if (FAILED(m_SwapChain->GetParent(IID_PPV_ARGS(Factory.GetAddressOf()))) || (!Factory))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::CreateSwapChainForWindow - IDXGIFactory not found."));
			return nullptr;
		}

		DXGI_FORMAT SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		TranslateResourceFormat(SwapChainFormat, InFormat);
		if (SwapChainFormat == DXGI_FORMAT_UNKNOWN)
		{
			SwapChainFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		}

		DXGI_SWAP_CHAIN_DESC SwapChainDesc = {};
		SwapChainDesc.BufferDesc.Width	= InWidth;
		SwapChainDesc.BufferDesc.Height	= InHeight;
		SwapChainDesc.BufferDesc.Format	= SwapChainFormat;
		SwapChainDesc.SampleDesc.Count	= 1u;
		SwapChainDesc.SampleDesc.Quality= 0u;
		SwapChainDesc.BufferUsage		= DXGI_USAGE_RENDER_TARGET_OUTPUT;
		// One buffer is enough for a tool window: ImGui records and presents it in the same
		// frame, and the flip-model bookkeeping a second buffer buys is not worth it there.
		SwapChainDesc.BufferCount		= 1u;
		SwapChainDesc.OutputWindow		= InWindowHandle;
		SwapChainDesc.Windowed			= TRUE;
		SwapChainDesc.SwapEffect		= DXGI_SWAP_EFFECT_DISCARD;
		SwapChainDesc.Flags				= 0u;

		Microsoft::WRL::ComPtr<IDXGISwapChain> WindowSwapChain;
		if (FAILED(Factory->CreateSwapChain(m_Device.Get(), &SwapChainDesc, WindowSwapChain.GetAddressOf())) || (!WindowSwapChain))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("RDeviceD3D11::CreateSwapChainForWindow - CreateSwapChain failed."));
			return nullptr;
		}
		Factory->MakeWindowAssociation(InWindowHandle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES);

		RD3D11WindowSwapChain* Chain = new RD3D11WindowSwapChain(m_Device.Get(), WindowSwapChain, InWidth, InHeight);
		if (Chain->IsValid() == FALSE)
		{
			delete Chain;
			return nullptr;
		}
		return Chain;
	}

	// ----- Resource creation -----

	BOOL8 RDeviceD3D11::CreateBuffer(const RBufferDesc& InDesc, const RRHISubresourceData& InInitialData, IRRHIBuffer** OutBuffer) noexcept
	{
		if (!OutBuffer)
		{
			return FALSE;
		}
		*OutBuffer = nullptr;

		// The RHI expresses "no initial data" by leaving RRHISubresourceData::Data
		// null (same convention the D3D12 back-end uses); the legacy D3D11 factory
		// takes a nullable RSubresourceDataDesc* instead.
		RSubresourceDataDesc Legacy;
		const RSubresourceDataDesc* LegacyPtr = nullptr;
		if (InInitialData.Data)
		{
			Legacy.pSysMem			= InInitialData.Data;
			Legacy.SysMemPitch		= InInitialData.RowPitch;
			Legacy.SysMemSlicePitch	= InInitialData.SlicePitch;
			LegacyPtr = &Legacy;
		}

		auto* NewBuffer = new RBufferResource();
		if (!CreateBuffer(*NewBuffer, InDesc, LegacyPtr))
		{
			delete NewBuffer;
			return FALSE;
		}
		*OutBuffer = NewBuffer;
		return TRUE;
	}

	BOOL8 RDeviceD3D11::CreateTexture(const RTextureDesc& InDesc, const Color4& /*InClearColor*/, const RRHISubresourceData& InInitialSubresources, IRRHITexture** OutTexture) noexcept
	{
		if (!OutTexture)
		{
			return FALSE;
		}
		*OutTexture = nullptr;

		// InClearColor only matters to the D3D12 back-end, which bakes it into the
		// resource's clear-value hint. D3D11 has no per-resource clear value, so the
		// engine's explicit ClearRenderTargetView calls stay authoritative.

		const BOOL8 IsCube = (static_cast<UINT8>(InDesc.MiscFlags) & static_cast<UINT8>(RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE)) != 0u;
		if (IsCube)
		{
			// The RHI hands over a single subresource, while the legacy cube factory
			// wants one desc per face. The provided payload seeds face 0; the
			// remaining faces stay uninitialized and have to be filled through
			// IRRHIDevice::UploadTexture.
			RSubresourceDataDesc LegacyArray[6] = {};
			const RSubresourceDataDesc* LegacyArrayPtr = nullptr;
			if (InInitialSubresources.Data)
			{
				LegacyArray[0].pSysMem			= InInitialSubresources.Data;
				LegacyArray[0].SysMemPitch		= InInitialSubresources.RowPitch;
				LegacyArray[0].SysMemSlicePitch	= InInitialSubresources.SlicePitch;
				LegacyArrayPtr = LegacyArray;
			}
			auto* Tex = new RTextureCubeResource();
			if (!CreateTextureCube(*Tex, InDesc, LegacyArrayPtr))
			{
				delete Tex;
				return FALSE;
			}
			*OutTexture = Tex;
			return TRUE;
		}

		// Non-cube paths: the RHI carries a single subresource per creation call, so
		// only that one is forwarded; mip / array uploads go through UploadTexture
		// (legacy CreateTexture2D handles a single subresource).
		RSubresourceDataDesc Legacy;
		const RSubresourceDataDesc* LegacyPtr = nullptr;
		if (InInitialSubresources.Data)
		{
			Legacy.pSysMem			= InInitialSubresources.Data;
			Legacy.SysMemPitch		= InInitialSubresources.RowPitch;
			Legacy.SysMemSlicePitch	= InInitialSubresources.SlicePitch;
			LegacyPtr = &Legacy;
		}

		const UINT8 BindFlags = InDesc.BindFlags;
		const UINT8 RTorDSorUAVMask = static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET) |
			static_cast<UINT8>(RBindFlagType::BIND_DEPTH_STENCIL) |
			static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS);
		if ((BindFlags & RTorDSorUAVMask) != 0u)
		{
			auto* Tex = new RRenderTexture2D();
			if (!CreateRenderTexture2D(*Tex, InDesc, LegacyPtr))
			{
				delete Tex;
				return FALSE;
			}
			*OutTexture = Tex;
			return TRUE;
		}

		auto* Tex = new RTexture2DResource();
		if (!CreateTexture2D(*Tex, InDesc, LegacyPtr))
		{
			delete Tex;
			return FALSE;
		}
		*OutTexture = Tex;
		return TRUE;
	}

	BOOL8 RDeviceD3D11::CreateShader(const RRHIShaderDesc& InDesc, IRRHIShader** OutShader) noexcept
	{
		if (!OutShader)
		{
			return FALSE;
		}
		*OutShader = nullptr;

		auto* Shader = new RD3D11Shader();
		if (!CreateShaderObject(InDesc, *Shader))
		{
			delete Shader;
			return FALSE;
		}
		*OutShader = Shader;
		return TRUE;
	}

	BOOL8 RDeviceD3D11::CreateRootSignature(const RRHIRootSignatureDesc& InDesc, IRRHIRootSignature** OutSignature) noexcept
	{
		if (!OutSignature)
		{
			return FALSE;
		}

		const UINT64 LayoutHash = ((InDesc.LayoutHash != 0ull) ? InDesc.LayoutHash : HashRHIRootSignatureDesc(InDesc));

		// Signatures are shared by layout, exactly as on D3D12: an equal desc hands back
		// the object that is already live. There is no native object to build here - the
		// back-end binds constant buffers straight to register slots and has no descriptor
		// tables - so the desc is used for its identity alone.
		RD3D11RootSignature* Cached = nullptr;
		RootSignatureCache.FindValue(LayoutHash, Cached);
		if (Cached != nullptr)
		{
			*OutSignature = Cached;
			return TRUE;
		}

		RD3D11RootSignature* Signature = new RD3D11RootSignature();
		Signature->LayoutHash = LayoutHash;
		RootSignaturePool.Add(Signature);
		RootSignatureCache.Add(LayoutHash, Signature);
		*OutSignature = Signature;
		return TRUE;
	}

	void RDeviceD3D11::DestroyRootSignature(IRRHIRootSignature* InSignature) noexcept
	{
		if (!InSignature)
		{
			return;
		}

		RD3D11RootSignature* Signature = static_cast<RD3D11RootSignature*>(InSignature);
		RootSignatureCache.Remove(Signature->LayoutHash);
		for (INT32 i = static_cast<INT32>(RootSignaturePool.Num()) - 1; i >= 0; i--)
		{
			if (RootSignaturePool[i] == Signature)
			{
				RootSignaturePool.RemoveAt(i);
				// The wrapper is the caller's to release (see the interface contract: it
				// must not be called while live PSOs still reference the layout). Nothing
				// native hangs off it here, so the delete is the whole release.
				delete Signature;
				break;
			}
		}
	}

	BOOL8 RDeviceD3D11::CreateGraphicsPipelineState(const RRHIGraphicsPipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept
	{
		if (!OutPipelineState)
		{
			return FALSE;
		}
		*OutPipelineState = nullptr;

		auto* PSO = new RPipelineStateD3D11();
		PSO->bIsCompute			= FALSE;
		PSO->PrimitiveTopology	= InDesc.PrimitiveTopology;
		// Every caller takes its signature from the pipeline state cache, which derives it from
		// the program's reflection. This back-end has no root signature concept, so the object
		// only records which one the program was laid out with and the layout stays empty: a
		// root index is a register slot here, which is what the binding side reads the zero
		// layout as (see RRHIPipelineLayoutInfo). A desc without one is refused rather than
		// filled in, so no state can end up claiming a layout it was not built for.
		if (InDesc.RootSignature == nullptr)
		{
			delete PSO;
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D11 PSO: the desc names no root signature; build it through RPipelineStateCache so it carries its program's layout."));
			return FALSE;
		}
		PSO->RootSignature		= InDesc.RootSignature;

		if (InDesc.VertexShader)
		{
			auto* VS = static_cast<IRD3D11Shader*>(InDesc.VertexShader);
			PSO->VertexShader = VS->GetD3D11VS();
			if (auto* BakedInputLayout = VS->GetD3D11InputLayout())
			{
				PSO->InputLayout = BakedInputLayout;
			}

			// A desc always carries an input layout (the render layer fills in the vertex stage's own
			// one), so this rebuilds an equivalent layout for the default path too. That is a
			// deliberate trade: the check is exact only for a layout that differs, and comparing
			// arrays here would mean carrying the shader's own layout around for the one caller that
			// declares a different vertex format. The rebuilt layout is byte for byte what the baked
			// one describes, so nothing about the pipeline changes; the pass layer that owns pipeline
			// creation can tighten this once it knows which declarations are overrides.
			if (InDesc.InputLayouts && (InDesc.InputLayoutCount > 0u))
			{
				if ((VS->GetByteCode() == nullptr) || (VS->GetByteCodeSize() == 0ull))
				{
					delete PSO;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create input layout for a declared vertex data layout failed (no vertex shader byte code)."));
					return FALSE;
				}
				TArray<D3D11_INPUT_ELEMENT_DESC> DataLayouts;
				DataLayouts.SetNum(InDesc.InputLayoutCount);
				for (UINT32 LayoutIndex = 0u; LayoutIndex < InDesc.InputLayoutCount; LayoutIndex++)
				{
					RDeviceD3D11::TranslateInputLayoutDesc(DataLayouts[LayoutIndex], InDesc.InputLayouts[LayoutIndex]);
				}
				Microsoft::WRL::ComPtr<ID3D11InputLayout> DeclaredInputLayout;
				if (FAILED(m_Device->CreateInputLayout(DataLayouts.GetData(), InDesc.InputLayoutCount
					, VS->GetByteCode(), static_cast<ULONG>(VS->GetByteCodeSize())
					, DeclaredInputLayout.ReleaseAndGetAddressOf())))
				{
					delete PSO;
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Create input layout for a declared vertex data layout failed."));
					return FALSE;
				}
				PSO->InputLayout = DeclaredInputLayout;
			}
		}
		if (InDesc.PixelShader)
		{
			auto* PS = static_cast<IRD3D11Shader*>(InDesc.PixelShader);
			PSO->PixelShader = PS->GetD3D11PS();
		}

		Microsoft::WRL::ComPtr<ID3D11RasterizerState> RS;
		if (CreateRasterizerState(RS, InDesc.Rasterizer))
		{
			PSO->RasterizerState = RS;
		}
		Microsoft::WRL::ComPtr<ID3D11BlendState> BS;
		if (CreateBlendState(BS, InDesc.Blend))
		{
			PSO->BlendState = BS;
		}
		Microsoft::WRL::ComPtr<ID3D11DepthStencilState> DSS;
		if (CreateDepthStencilState(DSS, InDesc.Depth, &InDesc.Stencil))
		{
			PSO->DepthStencilState = DSS;
		}

		*OutPipelineState = PSO;
		return TRUE;
	}

	BOOL8 RDeviceD3D11::CreateComputePipelineState(const RRHIComputePipelineDesc& InDesc, IRRHIPipelineState** OutPipelineState) noexcept
	{
		if (!OutPipelineState || !InDesc.ComputeShader)
		{
			return FALSE;
		}
		*OutPipelineState = nullptr;

		auto* PSO = new RPipelineStateD3D11();
		PSO->bIsCompute = TRUE;
		auto* CS = static_cast<IRD3D11Shader*>(InDesc.ComputeShader);
		PSO->ComputeShader = CS->GetD3D11CS();
		// Same signature rule as the graphics factory above.
		if (InDesc.RootSignature == nullptr)
		{
			delete PSO;
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("D3D11 PSO: the desc names no root signature; build it through RPipelineStateCache so it carries its program's layout."));
			return FALSE;
		}
		PSO->RootSignature = InDesc.RootSignature;
		*OutPipelineState = PSO;
		return TRUE;
	}

	BOOL8 RDeviceD3D11::CreateSampler(const RSamplerState& InDesc, IRRHISampler** OutSampler) noexcept
	{
		if (!OutSampler)
		{
			return FALSE;
		}
		*OutSampler = nullptr;

		auto* Sampler = new RSamplerResource();
		if (!CreateSamplerState(Sampler->SamplerState, InDesc))
		{
			delete Sampler;
			return FALSE;
		}
		*OutSampler = Sampler;
		return TRUE;
	}

	void RDeviceD3D11::DestroyResource(IRRHIResource* InResource) noexcept
	{
		// Resource pointers handed out by Create* are heap-allocated; delete
		// transitively releases ComPtrs and bindless slots through the
		// derived classes' destructors. D3D11 has no GPU-side latency to
		// worry about here because all rendering is synchronous.
		delete InResource;
	}

	// ----- Bindless virtual handle tables -----

	UINT32 RDeviceD3D11::RegisterBindlessSRV(IRRHITexture* InTexture) noexcept
	{
		if (BindlessSRVFreeIndices.Num() > 0)
		{
			const UINT32 Idx = BindlessSRVFreeIndices[BindlessSRVFreeIndices.Num() - 1];
			BindlessSRVFreeIndices.Pop();
			BindlessSRVTable[Idx] = InTexture;
			return Idx;
		}
		const UINT32 Idx = BindlessSRVTable.Num<UINT32>();
		BindlessSRVTable.Add(InTexture);
		return Idx;
	}

	UINT32 RDeviceD3D11::RegisterBindlessSRV(IRRHIBuffer* InBuffer) noexcept
	{
		if (BindlessSRVFreeIndices.Num() > 0)
		{
			const UINT32 Idx = BindlessSRVFreeIndices[BindlessSRVFreeIndices.Num() - 1];
			BindlessSRVFreeIndices.Pop();
			BindlessSRVTable[Idx] = InBuffer;
			return Idx;
		}
		const UINT32 Idx = BindlessSRVTable.Num<UINT32>();
		BindlessSRVTable.Add(InBuffer);
		return Idx;
	}

	UINT32 RDeviceD3D11::RegisterBindlessUAV(IRRHITexture* InTexture) noexcept
	{
		if (BindlessUAVFreeIndices.Num() > 0)
		{
			const UINT32 Idx = BindlessUAVFreeIndices[BindlessUAVFreeIndices.Num() - 1];
			BindlessUAVFreeIndices.Pop();
			BindlessUAVTable[Idx] = InTexture;
			return Idx;
		}
		const UINT32 Idx = BindlessUAVTable.Num<UINT32>();
		BindlessUAVTable.Add(InTexture);
		return Idx;
	}

	UINT32 RDeviceD3D11::RegisterBindlessUAV(IRRHIBuffer* InBuffer) noexcept
	{
		if (BindlessUAVFreeIndices.Num() > 0)
		{
			const UINT32 Idx = BindlessUAVFreeIndices[BindlessUAVFreeIndices.Num() - 1];
			BindlessUAVFreeIndices.Pop();
			BindlessUAVTable[Idx] = InBuffer;
			return Idx;
		}
		const UINT32 Idx = BindlessUAVTable.Num<UINT32>();
		BindlessUAVTable.Add(InBuffer);
		return Idx;
	}

	void RDeviceD3D11::ReleaseBindlessIndex(UINT32 InIndex) noexcept
	{
		// We don't know whether the index was an SRV or UAV; try both tables.
		if (InIndex < BindlessSRVTable.Num<UINT32>() && BindlessSRVTable[InIndex])
		{
			BindlessSRVTable[InIndex] = nullptr;
			BindlessSRVFreeIndices.Add(InIndex);
		}
		if (InIndex < BindlessUAVTable.Num<UINT32>() && BindlessUAVTable[InIndex])
		{
			BindlessUAVTable[InIndex] = nullptr;
			BindlessUAVFreeIndices.Add(InIndex);
		}
	}

	IRRHIResource* RDeviceD3D11::GetBindlessResource(UINT32 InIndex)const noexcept
	{
		if (InIndex < BindlessSRVTable.Num<UINT32>() && BindlessSRVTable[InIndex])
		{
			return BindlessSRVTable[InIndex];
		}
		if (InIndex < BindlessUAVTable.Num<UINT32>() && BindlessUAVTable[InIndex])
		{
			return BindlessUAVTable[InIndex];
		}
		return nullptr;
	}

	// ----- Upload -----

	void RDeviceD3D11::UploadBuffer(IRRHIBuffer* InDest, const UINT8* InData, UINT64 InSize, UINT64 InDestOffset) noexcept
	{
		if (!InDest || !InData || InSize == 0u)
		{
			return;
		}
		auto* NativeBuffer = static_cast<IRD3D11Buffer*>(InDest)->GetD3D11Buffer();
		if (!NativeBuffer)
		{
			return;
		}
		// D3D11 forbids a non-null pDstBox for shader-constant buffers
		// (UpdateSubresource silently no-ops or warns, leaving the CB stale).
		// When the caller requests a full-buffer write (offset 0, size matches
		// the resource), pass nullptr so CBs and ordinary buffers both work.
		const UINT8 BindFlags = InDest->GetBindFlags();
		const BOOL8 bIsConstantBuffer = !!(BindFlags & static_cast<UINT8>(RBindFlagType::BIND_CONSTANT_BUFFER));
		const BOOL8 bFullUpdate = (InDestOffset == 0u) && (InSize == InDest->GetByteSize());
		if (bIsConstantBuffer || bFullUpdate)
		{
			m_ImmediateContext->UpdateSubresource(NativeBuffer, 0u, nullptr, InData, 0u, 0u);
			return;
		}
		D3D11_BOX Box;
		Box.left	= static_cast<UINT>(InDestOffset);
		Box.top		= 0u;
		Box.front	= 0u;
		Box.right	= static_cast<UINT>(InDestOffset + InSize);
		Box.bottom	= 1u;
		Box.back	= 1u;
		m_ImmediateContext->UpdateSubresource(NativeBuffer, 0u, &Box, InData, 0u, 0u);
	}

	void RDeviceD3D11::UploadTexture(IRRHITexture* InDest, const RRHISubresourceData* InSubresources, UINT32 InSubresourceCount, UINT32 InFirstSubresource) noexcept
	{
		if (!InDest || !InSubresources || InSubresourceCount == 0u)
		{
			return;
		}
		auto* NativeResource = static_cast<IRD3D11Texture*>(InDest)->GetD3D11Resource();
		if (!NativeResource)
		{
			return;
		}
		for (UINT32 i = 0u; i < InSubresourceCount; i++)
		{
			m_ImmediateContext->UpdateSubresource(NativeResource, InFirstSubresource + i, nullptr,
				InSubresources[i].Data, InSubresources[i].RowPitch, InSubresources[i].SlicePitch);
		}
	}

	// ----- Command lists -----

	IRCommandList* RDeviceD3D11::AcquireCommandList()
	{
		RCommandListD3D11* CommandList = nullptr;
		if (CommandListFreeList.Num() > 0)
		{
			CommandList = CommandListFreeList[CommandListFreeList.Num() - 1];
			CommandListFreeList.Pop();
		}
		else
		{
			TUniquePtr<RCommandListD3D11> NewCommandList = EMemory::MakeUnique<RCommandListD3D11>(this, m_Device);
			CommandList = NewCommandList.get();
			CommandListPool.Add(EMemory::Move(NewCommandList));
		}
		CommandList->Reset();
		return CommandList;
	}

	void RDeviceD3D11::SubmitCommandList(IRCommandList* InCommandList) noexcept
	{
		if (!InCommandList)
		{
			return;
		}
		// Immediate-mode wrapper on D3D11: every IRCommandList call has
		// already executed on the shared immediate context, so submit
		// just retires the command list back to the pool.
		auto* D3D11List = static_cast<RCommandListD3D11*>(InCommandList);
		D3D11List->Close();
		CommandListFreeList.Add(D3D11List);
	}

	void RDeviceD3D11::SubmitCommandLists(IRCommandList* const* InCommandLists, UINT32 InCount) noexcept
	{
		if (!InCommandLists)
		{
			return;
		}
		for (UINT32 i = 0u; i < InCount; i++)
		{
			SubmitCommandList(InCommandLists[i]);
		}
	}

	// ----- Backbuffer access -----

	IRRHITexture* RDeviceD3D11::GetCurrentBackBuffer()const noexcept
	{
		// const_cast: BackBufferWrapper is logically owned by the device and
		// always returned as a writable handle; the const-ness here is only
		// because GetCurrentBackBuffer is part of the IRRHIDevice contract.
		return const_cast<RRenderTexture2D*>(&BackBufferWrapper);
	}

	// ----- GPU timestamp queries (Phase P.c) -----

	// Backend-specific heap layout lives in DeviceD3D11.h so CommandListD3D11
	// can reach TimestampQueries[] directly. Disjoint state is used both as
	// a validity flag for the latched ticks and as the source of truth for
	// GPU frequency - under D3D11 the frequency CAN change between frames
	// if the GPU clock-throttles, so we re-read it each EndTimestampFrame.

	BOOL8 RDeviceD3D11::CreateTimestampHeap(UINT32 InCapacity, RRHITimestampHeap** OutHeap) noexcept
	{
		if (!OutHeap || InCapacity == 0u) { return FALSE; }
		*OutHeap = nullptr;
		if (!m_Device) { return FALSE; }

		RRHITimestampHeapD3D11* Heap = new RRHITimestampHeapD3D11();
		Heap->Capacity = InCapacity;

		RQueryDesc DisjointDesc;
		DisjointDesc.QueryType	= RQueryType::QUERY_TIMESTAMP_DISJOINT;
		DisjointDesc.MiscFlags	= RQueryMiscFlagType::QUERY_MISC_DEFAULT;
		if (!CreateQuery(Heap->DisjointQuery, DisjointDesc))
		{
			delete Heap;
			return FALSE;
		}

		// Zero-init the raw pointer array so the dtor's cleanup loop is
		// always well-defined (even if a CreateQuery partway through fails).
		Heap->TimestampQueries = new ID3D11Query*[InCapacity]();

		RQueryDesc TimestampDesc;
		TimestampDesc.QueryType	= RQueryType::QUERY_TIMESTAMP;
		TimestampDesc.MiscFlags	= RQueryMiscFlagType::QUERY_MISC_DEFAULT;
		for (UINT32 i = 0u; i < InCapacity; i++)
		{
			Microsoft::WRL::ComPtr<ID3D11Query> Q;
			if (!CreateQuery(Q, TimestampDesc))
			{
				// Heap dtor releases the slots we've populated so far.
				delete Heap;
				return FALSE;
			}
			Heap->TimestampQueries[i] = Q.Detach();   // transfer to raw pointer
		}

		*OutHeap = Heap;
		return TRUE;
	}

	void RDeviceD3D11::DestroyTimestampHeap(RRHITimestampHeap* InHeap) noexcept
	{
		if (!InHeap) { return; }
		// All ComPtr members release their underlying ID3D11Query interfaces
		// through the heap's dtor - nothing else to teardown.
		delete InHeap;
	}

	void RDeviceD3D11::BeginTimestampFrame(RRHITimestampHeap* InHeap) noexcept
	{
		RRHITimestampHeapD3D11* Heap = static_cast<RRHITimestampHeapD3D11*>(InHeap);
		if (!Heap || !Heap->DisjointQuery || !m_ImmediateContext) { return; }
		if (Heap->DisjointOpen) { return; }
		m_ImmediateContext->Begin(Heap->DisjointQuery.Get());
		Heap->DisjointOpen = TRUE;
	}

	void RDeviceD3D11::EndTimestampFrame(RRHITimestampHeap* InHeap) noexcept
	{
		RRHITimestampHeapD3D11* Heap = static_cast<RRHITimestampHeapD3D11*>(InHeap);
		if (!Heap || !Heap->DisjointQuery || !m_ImmediateContext) { return; }
		if (!Heap->DisjointOpen) { return; }
		m_ImmediateContext->End(Heap->DisjointQuery.Get());
		Heap->DisjointOpen     = FALSE;
		Heap->DisjointLatched  = TRUE;
		// LastFrameValid / LastFrequency get refreshed inside ResolveTimestamps
		// once the GPU has drained - keep them stale here so a too-eager
		// caller can't read pre-resolve data.
	}

	BOOL8 RDeviceD3D11::ResolveTimestamps(RRHITimestampHeap* InHeap, UINT32 InBeginSlot, UINT32 InCount, UINT64* OutTicks) noexcept
	{
		RRHITimestampHeapD3D11* Heap = static_cast<RRHITimestampHeapD3D11*>(InHeap);
		if (!Heap || !OutTicks || InCount == 0u) { return FALSE; }
		if (!Heap->DisjointLatched) { return FALSE; }
		if ((InBeginSlot + InCount) > Heap->Capacity) { return FALSE; }
		if (!m_ImmediateContext) { return FALSE; }

		// Pull the disjoint payload first. Both the validity flag and the
		// frequency live here; bailing if Disjoint == TRUE means we refuse
		// to emit ticks the GPU itself has flagged as untrustworthy.
		D3D11_QUERY_DATA_TIMESTAMP_DISJOINT DisjointData = {};
		HRESULT Hr = m_ImmediateContext->GetData(Heap->DisjointQuery.Get(), &DisjointData, sizeof(DisjointData), 0u);
		if (Hr != S_OK) { return FALSE; }
		Heap->LastFrequency  = static_cast<UINT64>(DisjointData.Frequency);
		Heap->LastFrameValid = (DisjointData.Disjoint == FALSE) ? TRUE : FALSE;
		if (!Heap->LastFrameValid) { return FALSE; }

		for (UINT32 i = 0u; i < InCount; i++)
		{
			ID3D11Query* Q = Heap->TimestampQueries[InBeginSlot + i];
			if (!Q) { OutTicks[i] = 0u; continue; }
			UINT64 Tick = 0u;
			Hr = m_ImmediateContext->GetData(Q, &Tick, sizeof(Tick), 0u);
			if (Hr != S_OK) { return FALSE; }
			OutTicks[i] = Tick;
		}
		return TRUE;
	}

	UINT64 RDeviceD3D11::GetGPUTicksPerSecond(const RRHITimestampHeap* InHeap)const noexcept
	{
		const RRHITimestampHeapD3D11* Heap = static_cast<const RRHITimestampHeapD3D11*>(InHeap);
		if (!Heap) { return 0u; }
		return Heap->LastFrequency;
	}

};
