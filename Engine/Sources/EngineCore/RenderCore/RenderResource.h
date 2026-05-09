#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>
#include <RHI/IRRHIResource.h>
#include <RenderDevice/D3D11Native.h>

namespace PigeonEngine
{

	/*
	* DX11-backend concrete render resources.
	*
	* These classes hold the live ID3D11* COM objects directly, while also
	* implementing the back-end-agnostic IRRHIxxx interfaces so the same
	* object can flow through both legacy DX11 call sites and the new
	* CommandList / IRRHIDevice abstraction.
	*
	* Cached descriptor fields (Width, Height, ByteSize, BindFlags, ...)
	* mirror the values the resource was created with. They are populated
	* by RDeviceD3D11::Create* at the same time the COM object is built;
	* until then they read as zero. The getters required by IRRHIxxx
	* return the cached values.
	*
	* BindlessSRVIndex / BindlessUAVIndex default to RHI_INVALID_BINDLESS_INDEX
	* and are filled in once the resource is registered with the bindless
	* descriptor heap (Phase 3, D3D12). For the D3D11 back-end the indices
	* are virtual handles maintained by RDeviceD3D11.
	*/

	class RVertexShaderResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Shader
	{
	public:
		RVertexShaderResource() : Shader(nullptr), InputLayout(nullptr), RawLayouts(nullptr), LayoutNum(0u) {}
		RVertexShaderResource(const RVertexShaderResource& Other)
			: Shader(Other.Shader), InputLayout(Other.InputLayout), RawLayouts(nullptr), LayoutNum(0u)
		{
			if ((!!(Other.RawLayouts)) && (Other.LayoutNum > 0u))
			{
				RawLayouts = new RInputLayoutDesc[Other.LayoutNum];
				LayoutNum = Other.LayoutNum;
				for (UINT32 i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
		}
		virtual ~RVertexShaderResource() { ReleaseRenderResource(); }
		RVertexShaderResource& operator=(const RVertexShaderResource& Other)
		{
			ReleaseRenderResource();
			Shader = Other.Shader;
			InputLayout = Other.InputLayout;
			if ((!!(Other.RawLayouts)) && (Other.LayoutNum > 0u))
			{
				RawLayouts = new RInputLayoutDesc[Other.LayoutNum];
				LayoutNum = Other.LayoutNum;
				for (UINT32 i = 0u, n = Other.LayoutNum; i < n; i++)
				{
					RawLayouts[i] = Other.RawLayouts[i];
				}
			}
			return (*this);
		}
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return ((!!Shader) && (!!InputLayout) && (!!RawLayouts) && (LayoutNum > 0u));
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Shader = nullptr;
			InputLayout = nullptr;
			if (RawLayouts)
			{
				delete[]RawLayouts;
				RawLayouts = nullptr;
			}
			LayoutNum = 0u;
		};

		// IRRHIShader
		virtual ERHIShaderStage GetShaderStage()const override { return ERHIShaderStage::RHI_SHADER_STAGE_VERTEX; }

		// IRD3D11Shader native getters
		virtual ID3D11VertexShader* GetD3D11VS()const override { return Shader.Get(); }
		virtual ID3D11InputLayout* GetD3D11InputLayout()const override { return InputLayout.Get(); }

		Microsoft::WRL::ComPtr<ID3D11VertexShader>	Shader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	InputLayout;
		RInputLayoutDesc*							RawLayouts;
		UINT32										LayoutNum;
	};
	class RPixelShaderResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Shader
	{
	public:
		RPixelShaderResource() : Shader(nullptr) {}
		RPixelShaderResource(const RPixelShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RPixelShaderResource() { ReleaseRenderResource(); }
		RPixelShaderResource& operator=(const RPixelShaderResource& Other)
		{
			Shader = Other.Shader;
			return (*this);
		}
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Shader);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Shader = nullptr;
		};

		// IRRHIShader
		virtual ERHIShaderStage GetShaderStage()const override { return ERHIShaderStage::RHI_SHADER_STAGE_PIXEL; }

		// IRD3D11Shader native getters
		virtual ID3D11PixelShader* GetD3D11PS()const override { return Shader.Get(); }

		Microsoft::WRL::ComPtr<ID3D11PixelShader> Shader;
	};
	class RComputeShaderResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Shader
	{
	public:
		RComputeShaderResource() : Shader(nullptr) {}
		RComputeShaderResource(const RComputeShaderResource& Other) : Shader(Other.Shader) {}
		virtual ~RComputeShaderResource() { ReleaseRenderResource(); }
		RComputeShaderResource& operator=(const RComputeShaderResource& Other)
		{
			Shader = Other.Shader;
			return (*this);
		}
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Shader);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Shader = nullptr;
		};

		// IRRHIShader
		virtual ERHIShaderStage GetShaderStage()const override { return ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE; }

		// IRD3D11Shader native getters
		virtual ID3D11ComputeShader* GetD3D11CS()const override { return Shader.Get(); }

		Microsoft::WRL::ComPtr<ID3D11ComputeShader> Shader;
	};
	class RStructuredBuffer : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Buffer
	{
	public:
		RStructuredBuffer()
			: AccessMapRead(FALSE), AccessMapWrite(FALSE)
			, Buffer(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr)
			, ByteSize(0u), StructureByteStride(0u), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RStructuredBuffer(const RStructuredBuffer& Other)
			: AccessMapRead(Other.AccessMapRead), AccessMapWrite(Other.AccessMapWrite)
			, Buffer(Other.Buffer), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView)
			, ByteSize(Other.ByteSize), StructureByteStride(Other.StructureByteStride), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RStructuredBuffer() { ReleaseRenderResource(); }
		RStructuredBuffer& operator=(const RStructuredBuffer& Other)
		{
			AccessMapRead			= Other.AccessMapRead;
			AccessMapWrite			= Other.AccessMapWrite;
			Buffer					= Other.Buffer;
			UnorderedAccessView		= Other.UnorderedAccessView;
			ShaderResourceView		= Other.ShaderResourceView;
			ByteSize				= Other.ByteSize;
			StructureByteStride		= Other.StructureByteStride;
			BindFlags				= Other.BindFlags;
			BindlessSRVIndex		= Other.BindlessSRVIndex;
			BindlessUAVIndex		= Other.BindlessUAVIndex;
			return (*this);
		}
		// StructuredBuffer can bind UAV or SRV, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			AccessMapRead		= FALSE;
			AccessMapWrite		= FALSE;
			Buffer				= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
			ByteSize			= 0u;
			StructureByteStride	= 0u;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		}

		// IRRHIBuffer
		virtual UINT32	GetByteSize()const override { return ByteSize; }
		virtual UINT32	GetStructureByteStride()const override { return StructureByteStride; }
		virtual UINT8	GetBindFlags()const override { return BindFlags; }
		virtual UINT32	GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32	GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Buffer native getters
		virtual ID3D11Buffer*				GetD3D11Buffer()const override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const override { return ShaderResourceView.Get(); }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const override { return UnorderedAccessView.Get(); }

		BOOL8												AccessMapRead;
		BOOL8												AccessMapWrite;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT32												ByteSize;
		UINT32												StructureByteStride;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
	};
	class RRenderTexture2D : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RRenderTexture2D()
			: Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr)
			, ShaderResourceView(nullptr), DepthStencilView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u), ArraySize(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RRenderTexture2D(const RRenderTexture2D& Other)
			: Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView)
			, UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView)
			, DepthStencilView(Other.DepthStencilView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels), ArraySize(Other.ArraySize)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RRenderTexture2D() { ReleaseRenderResource(); }
		RRenderTexture2D& operator=(const RRenderTexture2D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			DepthStencilView	= Other.DepthStencilView;
			Width				= Other.Width;
			Height				= Other.Height;
			MipLevels			= Other.MipLevels;
			ArraySize			= Other.ArraySize;
			Format				= Other.Format;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			return (*this);
		}
		// RenderTexture2D can bind RTV, UAV, SRV or DSV, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			RenderTargetView	= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
			DepthStencilView	= nullptr;
			Width				= 0u;
			Height				= 0u;
			MipLevels			= 1u;
			ArraySize			= 1u;
			Format				= RFormatType::FORMAT_UNKNOWN;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		}

		// IRRHITexture
		virtual UINT32      GetWidth()const override { return Width; }
		virtual UINT32      GetHeight()const override { return Height; }
		virtual UINT32      GetDepth()const override { return 1u; }
		virtual UINT32      GetMipLevels()const override { return MipLevels; }
		virtual UINT32      GetArraySize()const override { return ArraySize; }
		virtual RFormatType GetFormat()const override { return Format; }
		virtual UINT8       GetBindFlags()const override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const override { return ShaderResourceView.Get(); }
		virtual ID3D11RenderTargetView*		GetD3D11RTV()const override { return RenderTargetView.Get(); }
		virtual ID3D11DepthStencilView*		GetD3D11DSV()const override { return DepthStencilView.Get(); }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const override { return UnorderedAccessView.Get(); }

		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>		DepthStencilView;
		UINT32												Width;
		UINT32												Height;
		UINT32												MipLevels;
		UINT32												ArraySize;
		RFormatType											Format;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
	};
	class RRenderTexture3D : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RRenderTexture3D()
			: Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr)
			, Width(0u), Height(0u), Depth(0u), MipLevels(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RRenderTexture3D(const RRenderTexture3D& Other)
			: Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView)
			, UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView)
			, Width(Other.Width), Height(Other.Height), Depth(Other.Depth), MipLevels(Other.MipLevels)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RRenderTexture3D() { ReleaseRenderResource(); }
		RRenderTexture3D& operator=(const RRenderTexture3D& Other)
		{
			Buffer				= Other.Buffer;
			RenderTargetView	= Other.RenderTargetView;
			UnorderedAccessView	= Other.UnorderedAccessView;
			ShaderResourceView	= Other.ShaderResourceView;
			Width				= Other.Width;
			Height				= Other.Height;
			Depth				= Other.Depth;
			MipLevels			= Other.MipLevels;
			Format				= Other.Format;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			return (*this);
		}
		// RenderTexture3D can bind RTV, UAV or SRV, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			RenderTargetView	= nullptr;
			UnorderedAccessView	= nullptr;
			ShaderResourceView	= nullptr;
			Width				= 0u;
			Height				= 0u;
			Depth				= 0u;
			MipLevels			= 1u;
			Format				= RFormatType::FORMAT_UNKNOWN;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		};

		// IRRHITexture
		virtual UINT32      GetWidth()const override { return Width; }
		virtual UINT32      GetHeight()const override { return Height; }
		virtual UINT32      GetDepth()const override { return Depth; }
		virtual UINT32      GetMipLevels()const override { return MipLevels; }
		virtual UINT32      GetArraySize()const override { return 1u; }
		virtual RFormatType GetFormat()const override { return Format; }
		virtual UINT8       GetBindFlags()const override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const override { return ShaderResourceView.Get(); }
		virtual ID3D11RenderTargetView*		GetD3D11RTV()const override { return RenderTargetView.Get(); }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const override { return UnorderedAccessView.Get(); }

		Microsoft::WRL::ComPtr<ID3D11Texture3D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>		RenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT32												Width;
		UINT32												Height;
		UINT32												Depth;
		UINT32												MipLevels;
		RFormatType											Format;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
	};
	class RTexture2DResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RTexture2DResource()
			: Buffer(nullptr), ShaderResourceView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u), ArraySize(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RTexture2DResource(const RTexture2DResource& Other)
			: Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels), ArraySize(Other.ArraySize)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RTexture2DResource() { ReleaseRenderResource(); }
		RTexture2DResource& operator=(const RTexture2DResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
			Width				= Other.Width;
			Height				= Other.Height;
			MipLevels			= Other.MipLevels;
			ArraySize			= Other.ArraySize;
			Format				= Other.Format;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			return (*this);
		}
		// Texture2DResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
			Width				= 0u;
			Height				= 0u;
			MipLevels			= 1u;
			ArraySize			= 1u;
			Format				= RFormatType::FORMAT_UNKNOWN;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		}

		// IRRHITexture
		virtual UINT32      GetWidth()const override { return Width; }
		virtual UINT32      GetHeight()const override { return Height; }
		virtual UINT32      GetDepth()const override { return 1u; }
		virtual UINT32      GetMipLevels()const override { return MipLevels; }
		virtual UINT32      GetArraySize()const override { return ArraySize; }
		virtual RFormatType GetFormat()const override { return Format; }
		virtual UINT8       GetBindFlags()const override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const override { return ShaderResourceView.Get(); }

		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT32												Width;
		UINT32												Height;
		UINT32												MipLevels;
		UINT32												ArraySize;
		RFormatType											Format;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
	};
	class RTextureCubeResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RTextureCubeResource()
			: Buffer(nullptr), ShaderResourceView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RTextureCubeResource(const RTextureCubeResource& Other)
			: Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RTextureCubeResource() { ReleaseRenderResource(); }
		RTextureCubeResource& operator=(const RTextureCubeResource& Other)
		{
			Buffer				= Other.Buffer;
			ShaderResourceView	= Other.ShaderResourceView;
			Width				= Other.Width;
			Height				= Other.Height;
			MipLevels			= Other.MipLevels;
			Format				= Other.Format;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			return (*this);
		}
		// TextureCubeResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			ShaderResourceView	= nullptr;
			Width				= 0u;
			Height				= 0u;
			MipLevels			= 1u;
			Format				= RFormatType::FORMAT_UNKNOWN;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		}

		// IRRHITexture
		virtual UINT32		GetWidth()const override { return Width; }
		virtual UINT32      GetHeight()const override { return Height; }
		virtual UINT32      GetDepth()const override { return 1u; }
		virtual UINT32      GetMipLevels()const override { return MipLevels; }
		virtual UINT32      GetArraySize()const override { return 6u; }
		virtual RFormatType GetFormat()const override { return Format; }
		virtual UINT8       GetBindFlags()const override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const override { return ShaderResourceView.Get(); }

		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT32												Width;
		UINT32												Height;
		UINT32												MipLevels;
		RFormatType											Format;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
	};
	class RBufferResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Buffer
	{
	public:
		RBufferResource()
			: Buffer(nullptr)
			, ByteSize(0u), StructureByteStride(0u), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX) {
		}
		RBufferResource(const RBufferResource& Other)
			: Buffer(Other.Buffer)
			, ByteSize(Other.ByteSize), StructureByteStride(Other.StructureByteStride), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex) {
		}
		virtual ~RBufferResource() { ReleaseRenderResource(); }
		RBufferResource& operator=(const RBufferResource& Other)
		{
			Buffer				= Other.Buffer;
			ByteSize			= Other.ByteSize;
			StructureByteStride	= Other.StructureByteStride;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			return (*this);
		}
		// BufferResource is raw buffer resource, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			Buffer				= nullptr;
			ByteSize			= 0u;
			StructureByteStride	= 0u;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
		}

		// IRRHIBuffer
		virtual UINT32	GetByteSize()const override { return ByteSize; }
		virtual UINT32	GetStructureByteStride()const override { return StructureByteStride; }
		virtual UINT8	GetBindFlags()const override { return BindFlags; }
		virtual UINT32	GetBindlessSRVIndex()const override { return BindlessSRVIndex; }
		virtual UINT32	GetBindlessUAVIndex()const override { return BindlessUAVIndex; }

		// IRD3D11Buffer native getters
		virtual ID3D11Buffer* GetD3D11Buffer()const override { return Buffer.Get(); }

		Microsoft::WRL::ComPtr<ID3D11Buffer>	Buffer;
		UINT32									ByteSize;
		UINT32									StructureByteStride;
		UINT8									BindFlags;
		UINT32									BindlessSRVIndex;
		UINT32									BindlessUAVIndex;
	};
	class RIndexBufferResource : public RBufferResource
	{
	public:
		RIndexBufferResource() : UseShort(FALSE), IndexCount(0u) {}
		RIndexBufferResource(const RIndexBufferResource& Other) : RBufferResource(Other), UseShort(Other.UseShort), IndexCount(Other.IndexCount) {}
		virtual ~RIndexBufferResource() {}
		RIndexBufferResource& operator=(const RIndexBufferResource& Other)
		{
			RBufferResource::operator=(Other);
			UseShort	= Other.UseShort;
			IndexCount	= Other.IndexCount;
			return (*this);
		}
		virtual BOOL8 IsRenderResourceValid()const override
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check index buffer short index failed."), (UseShort ? (IndexCount <= 65536u) : TRUE));
			return (RBufferResource::IsRenderResourceValid() && (IndexCount != 0u));
		}
		virtual void ReleaseRenderResource()override
		{
			RBufferResource::ReleaseRenderResource();
			UseShort	= FALSE;
			IndexCount	= 0u;
		}

		BOOL32	UseShort;
		UINT32	IndexCount;
	};
	class RVertexBufferResource : public RBufferResource
	{
	public:
		RVertexBufferResource() : Stride(0u) {}
		RVertexBufferResource(const RVertexBufferResource& Other) : RBufferResource(Other), Stride(Other.Stride) {}
		virtual ~RVertexBufferResource() {}
		RVertexBufferResource& operator=(const RVertexBufferResource& Other)
		{
			RBufferResource::operator=(Other);
			Stride = Other.Stride;
			return (*this);
		}
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (RBufferResource::IsRenderResourceValid() && (Stride != 0u));
		}
		virtual void ReleaseRenderResource()override
		{
			RBufferResource::ReleaseRenderResource();
			Stride = 0u;
		}

		UINT32 Stride;
	};
	class RSamplerResource : public RRenderResourceInterface, public IRD3D11Sampler
	{
	public:
		RSamplerResource() : SamplerState(nullptr) {}
		RSamplerResource(const RSamplerResource& Other) : SamplerState(Other.SamplerState) {}
		virtual ~RSamplerResource() { ReleaseRenderResource(); }
		RSamplerResource& operator=(const RSamplerResource& Other)
		{
			SamplerState = Other.SamplerState;
			return (*this);
		}
		// SamplerResource validation only determines by SamplerState valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!SamplerState);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			SamplerState = nullptr;
		}

		// IRD3D11Sampler native getter
		virtual ID3D11SamplerState* GetD3D11SamplerState()const override { return SamplerState.Get(); }

		Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
	};
	class RRasterizerResource : public RRenderResourceInterface
	{
	public:
		RRasterizerResource() : RasterizerState(nullptr) {}
		RRasterizerResource(const RRasterizerResource& Other) : RasterizerState(Other.RasterizerState) {}
		virtual ~RRasterizerResource() { ReleaseRenderResource(); }
		RRasterizerResource& operator=(const RRasterizerResource& Other)
		{
			RasterizerState = Other.RasterizerState;
			return (*this);
		}
		// RasterizerResource validation only determines by RasterizerState valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!RasterizerState);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			RasterizerState = nullptr;
		}

		Microsoft::WRL::ComPtr<ID3D11RasterizerState>	RasterizerState;
	};
	class RBlendResource : public RRenderResourceInterface
	{
	public:
		RBlendResource() : BlendState(nullptr) {}
		RBlendResource(const RBlendResource& Other) : BlendState(Other.BlendState) {}
		virtual ~RBlendResource() { ReleaseRenderResource(); }
		RBlendResource& operator=(const RBlendResource& Other)
		{
			BlendState = Other.BlendState;
			return (*this);
		}
		// BlendResource validation only determines by BlendState valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!BlendState);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			BlendState = nullptr;
		}

		Microsoft::WRL::ComPtr<ID3D11BlendState>	BlendState;
	};
	class RDepthStencilResource : public RRenderResourceInterface
	{
	public:
		RDepthStencilResource() : DepthStencilState(nullptr) {}
		RDepthStencilResource(const RDepthStencilResource& Other) : DepthStencilState(Other.DepthStencilState) {}
		virtual ~RDepthStencilResource() { ReleaseRenderResource(); }
		RDepthStencilResource& operator=(const RDepthStencilResource& Other)
		{
			DepthStencilState = Other.DepthStencilState;
			return (*this);
		}
		// DepthStencilResource validation only determines by DepthStencilState valid.
		virtual BOOL8 IsRenderResourceValid()const override
		{
			return (!!DepthStencilState);
		}
		virtual BOOL8 InitRenderResource()override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()override
		{
			DepthStencilState = nullptr;
		}

		Microsoft::WRL::ComPtr<ID3D11DepthStencilState>	DepthStencilState;
	};
	class RSceneTextures final
	{
	public:
		void	InitResources(const Vector2Int& InTextureSize, const BOOL32 IsNeedStencil);
		void	ClearResources();
		void	ReleaseResources();
	public:
		RRenderTexture2D	SceneDepthStencil;
		RRenderTexture2D	SceneColor;
		RRenderTexture2D	GBufferA;
		RRenderTexture2D	GBufferB;
		RRenderTexture2D	GBufferC;
	public:
		const Vector2Int&	GetTextureSize()const;
		BOOL32				IsNeedStencil()const;
	private:
		Vector2Int			TextureSize;
		BOOL32				NeedStencil;
	public:
		RSceneTextures();
		~RSceneTextures();
		RSceneTextures(const RSceneTextures&) = delete;
		RSceneTextures& operator=(const RSceneTextures&) = delete;
	};
	class RShadowTexture final
	{
	public:
		void	InitResources(const TArray<Vector2Int>& InTextureSizes);
		void	ClearResources();
		void	ReleaseResources();
	public:
		TArray<RRenderTexture2D>	ShadowMaps;
	private:
		TArray<Vector2Int>			TextureSizes;
	public:
		RShadowTexture();
		~RShadowTexture();
		RShadowTexture(const RShadowTexture&) = delete;
		RShadowTexture& operator=(const RShadowTexture&) = delete;
	};

};
