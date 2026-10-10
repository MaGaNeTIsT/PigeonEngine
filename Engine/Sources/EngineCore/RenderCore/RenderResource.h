#pragma once

#include <CoreMinimal.h>
#include <d3d11.h>
#pragma comment (lib, "d3d11.lib")
#pragma comment (lib, "winmm.lib")
#pragma comment (lib, "xaudio2.lib")
#include <RenderConfig/RenderConfig.h>
#include <RenderCommon.h>
#include <RHI/IRRHIResource.h>
#include <RHI/RFormatTraits.h>
#include <RenderDevice/D3D11Native.h>

namespace PigeonEngine
{

	/*
	* Engine side render resources.
	*
	* Each resource is the engine-visible half of one GPU object: it holds the abstract
	* IRRHIBuffer / IRRHITexture / IRRHISampler handle the back-end handed out, plus the
	* metadata the render layer needs at bind and draw time (sizes, strides, formats,
	* bind flags, bindless indices). Back-end private types (RD3D11*, RD3D12*) own the
	* native objects and are never referenced from here.
	*
	* The cached descriptor fields mirror the values the resource was created with and
	* are populated by the IRRHIDevice create path. GetResolvedRHIBuffer /
	* GetResolvedRHITexture return the handle to pass into IRCommandList.
	*
	* Shader stages are not represented here: a compiled stage is an RShaderStageResource
	* (RenderCore/Shader/ShaderStageResource.h), owned by its EShaderStageAsset.
	*/

	class RStructuredBuffer : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Buffer
	{
	public:
		RStructuredBuffer()
			: AccessMapRead(FALSE), AccessMapWrite(FALSE)
			, Buffer(nullptr), UnorderedAccessView(nullptr), ShaderResourceView(nullptr)
			, ByteSize(0u), StructureByteStride(0u), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
			, RHIBuffer(nullptr) {
		}
		RStructuredBuffer(const RStructuredBuffer& Other)
			: AccessMapRead(Other.AccessMapRead), AccessMapWrite(Other.AccessMapWrite)
			, Buffer(Other.Buffer), UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView)
			, ByteSize(Other.ByteSize), StructureByteStride(Other.StructureByteStride), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex)
			, RHIBuffer(Other.RHIBuffer) {
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
			RHIBuffer				= Other.RHIBuffer;
			return (*this);
		}
		// StructuredBuffer can bind UAV or SRV, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHIBuffer)
			{
				return TRUE;
			}
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
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
			RHIBuffer			= nullptr;
		}

		// IRRHIBuffer
		virtual UINT64	GetByteSize()const noexcept override { return ByteSize; }
		virtual UINT32	GetStructureByteStride()const noexcept override { return StructureByteStride; }
		virtual UINT8	GetBindFlags()const noexcept override { return BindFlags; }
		virtual UINT32	GetBindlessSRVIndex()const noexcept override { return BindlessSRVIndex; }
		virtual UINT32	GetBindlessUAVIndex()const noexcept override { return BindlessUAVIndex; }

		// IRD3D11Buffer native getters
		virtual ID3D11Buffer*				GetD3D11Buffer()const noexcept override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const noexcept override { return ShaderResourceView.Get(); }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const noexcept override { return UnorderedAccessView.Get(); }

		// Phase 1 transition helper: create the underlying structured buffer through the
		// abstract IRRHIDevice. The desc must carry the structured misc flag and the
		// element stride, which is what both back-ends key off to build a structured
		// resource. D3D11 unwraps the result into the ComPtr fields, D3D12 keeps the
		// abstract pointer in RHIBuffer.
		BOOL8 InitFromRHI(class IRRHIDevice* InRenderDevice, const RBufferDesc& InDesc, const struct RRHISubresourceData* InInitialData = nullptr);

		// Phase 1 transition helper: upload CPU data through IRRHIDevice::UploadBuffer.
		void UploadDataViaRHI(class IRRHIDevice* InRenderDevice, const void* InData, SIZE_T InSize, SIZE_T InDestOffset = 0u);

		// Returns the abstract IRRHIBuffer to pass into IRCommandList bindings. On D3D12
		// RHIBuffer is the real resource and `this` is a metadata wrapper; on D3D11
		// `this` itself IS-A IRRHIBuffer through the legacy chain.
		IRRHIBuffer* GetResolvedRHIBuffer() { return RHIBuffer ? RHIBuffer : static_cast<IRRHIBuffer*>(this); }
		const IRRHIBuffer* GetResolvedRHIBuffer() const { return RHIBuffer ? RHIBuffer : static_cast<const IRRHIBuffer*>(this); }

		BOOL8												AccessMapRead;
		BOOL8												AccessMapWrite;
		Microsoft::WRL::ComPtr<ID3D11Buffer>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView>	UnorderedAccessView;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT64												ByteSize;
		UINT32												StructureByteStride;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
		// D3D12 transition: populated only when the active back-end is D3D12.
		IRRHIBuffer*										RHIBuffer;
	};
	class RRenderTexture2D : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RRenderTexture2D()
			: Buffer(nullptr), RenderTargetView(nullptr), UnorderedAccessView(nullptr)
			, ShaderResourceView(nullptr), DepthStencilView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u), ArraySize(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), ViewFormats{}, BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
			, RHITexture(nullptr) {
		}
		RRenderTexture2D(const RRenderTexture2D& Other)
			: Buffer(Other.Buffer), RenderTargetView(Other.RenderTargetView)
			, UnorderedAccessView(Other.UnorderedAccessView), ShaderResourceView(Other.ShaderResourceView)
			, DepthStencilView(Other.DepthStencilView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels), ArraySize(Other.ArraySize)
			, Format(Other.Format), ViewFormats(Other.ViewFormats), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex)
			, RHITexture(Other.RHITexture) {
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
			ViewFormats			= Other.ViewFormats;
			BindFlags			= Other.BindFlags;
			BindlessSRVIndex	= Other.BindlessSRVIndex;
			BindlessUAVIndex	= Other.BindlessUAVIndex;
			RHITexture			= Other.RHITexture;
			return (*this);
		}
		// RenderTexture2D can bind RTV, UAV, SRV or DSV, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHITexture)
			{
				return TRUE;
			}
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
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
			ViewFormats			= RTextureFormatSet();
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
			RHITexture			= nullptr;
		}

		// IRRHITexture
		virtual UINT32      GetWidth()const noexcept override { return Width; }
		virtual UINT32      GetHeight()const noexcept override { return Height; }
		virtual UINT32      GetDepth()const noexcept override { return 1u; }
		virtual UINT32      GetMipLevels()const noexcept override { return MipLevels; }
		virtual UINT32      GetArraySize()const noexcept override { return ArraySize; }
		virtual RFormatType GetFormat()const noexcept override { return Format; }
		virtual UINT8       GetBindFlags()const noexcept override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const noexcept override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const noexcept override { return BindlessUAVIndex; }
		// Only the D3D11 path answers here: in D3D12 mode the object the render layer binds is
		// the back-end's own texture (RHITexture), which carries its resolved formats itself.
		virtual RFormatType GetViewFormat(ERHIViewFormatType InView)const noexcept override { return ViewFormats.GetViewFormat(InView); }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const noexcept override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const noexcept override { return ShaderResourceView.Get(); }
		virtual ID3D11RenderTargetView*		GetD3D11RTV()const noexcept override { return RenderTargetView.Get(); }
		virtual ID3D11DepthStencilView*		GetD3D11DSV()const noexcept override { return DepthStencilView.Get(); }
		virtual ID3D11UnorderedAccessView*	GetD3D11UAV()const noexcept override { return UnorderedAccessView.Get(); }

		// Phase 2 transition helper: create the underlying RT/DS texture
		// through IRRHIDevice::CreateRenderTexture2DResource. D3D11 unwraps
		// into the ComPtr fields; D3D12 stores the abstract pointer in
		// RHITexture so consumers can route to CommandList via
		// GetResolvedRHITexture() below.
		BOOL8 InitFromRHI(class IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc);

		// Returns the abstract IRRHITexture to feed into IRCommandList.
		// In D3D12 mode this is the actual D3D12 texture; in D3D11 mode
		// it's `this` (the wrapper IS-A IRRHITexture via IRD3D11Texture).
		IRRHITexture* GetResolvedRHITexture() { return RHITexture ? RHITexture : static_cast<IRRHITexture*>(this); }

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
		// The formats the resource and its views were created with, resolved from the desc by
		// RFormatTraits. Format above stays the content format the caller asked for.
		RTextureFormatSet									ViewFormats;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
		// D3D12 transition: populated only when active back-end is D3D12.
		IRRHITexture*										RHITexture;
	};
	class RTexture2DResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RTexture2DResource()
			: Buffer(nullptr), ShaderResourceView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u), ArraySize(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
			, RHITexture(nullptr) {
		}
		RTexture2DResource(const RTexture2DResource& Other)
			: Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels), ArraySize(Other.ArraySize)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex)
			, RHITexture(Other.RHITexture) {
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
			RHITexture			= Other.RHITexture;
			return (*this);
		}
		// Texture2DResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHITexture)
			{
				return TRUE;
			}
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
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
			RHITexture			= nullptr;
		}

		// IRRHITexture
		virtual UINT32      GetWidth()const noexcept override { return Width; }
		virtual UINT32      GetHeight()const noexcept override { return Height; }
		virtual UINT32      GetDepth()const noexcept override { return 1u; }
		virtual UINT32      GetMipLevels()const noexcept override { return MipLevels; }
		virtual UINT32      GetArraySize()const noexcept override { return ArraySize; }
		virtual RFormatType GetFormat()const noexcept override { return Format; }
		virtual UINT8       GetBindFlags()const noexcept override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const noexcept override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const noexcept override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const noexcept override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const noexcept override { return ShaderResourceView.Get(); }

		// Phase 1 transition helper: create the underlying texture through
		// IRRHIDevice::CreateTexture. D3D11 backend unwraps the returned
		// IRRHITexture into the ComPtr fields; D3D12 backend stores the
		// abstract pointer in RHITexture.
		BOOL8 InitFromRHI(class IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc, const struct RRHISubresourceData* InInitialData = nullptr);

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
		// D3D12 transition: populated only when the active back-end is D3D12.
		IRRHITexture*										RHITexture;
	};
	class RTextureCubeResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Texture
	{
	public:
		RTextureCubeResource()
			: Buffer(nullptr), ShaderResourceView(nullptr)
			, Width(0u), Height(0u), MipLevels(1u)
			, Format(RFormatType::FORMAT_UNKNOWN), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
			, RHITexture(nullptr) {
		}
		RTextureCubeResource(const RTextureCubeResource& Other)
			: Buffer(Other.Buffer), ShaderResourceView(Other.ShaderResourceView)
			, Width(Other.Width), Height(Other.Height), MipLevels(Other.MipLevels)
			, Format(Other.Format), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex)
			, RHITexture(Other.RHITexture) {
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
			RHITexture			= Other.RHITexture;
			return (*this);
		}
		// TextureCubeResource must bind SRV, so this function determines by buffer valid and SRV valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHITexture)
			{
				return TRUE;
			}
			return ((!!Buffer) && (!!ShaderResourceView));
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
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
			RHITexture			= nullptr;
		}

		// IRRHITexture
		virtual UINT32		GetWidth()const noexcept override { return Width; }
		virtual UINT32      GetHeight()const noexcept override { return Height; }
		virtual UINT32      GetDepth()const noexcept override { return 1u; }
		virtual UINT32      GetMipLevels()const noexcept override { return MipLevels; }
		virtual UINT32      GetArraySize()const noexcept override { return 6u; }
		virtual RFormatType GetFormat()const noexcept override { return Format; }
		virtual UINT8       GetBindFlags()const noexcept override { return BindFlags; }
		virtual UINT32      GetBindlessSRVIndex()const noexcept override { return BindlessSRVIndex; }
		virtual UINT32      GetBindlessUAVIndex()const noexcept override { return BindlessUAVIndex; }

		// IRD3D11Texture native getters
		virtual ID3D11Resource*				GetD3D11Resource()const noexcept override { return Buffer.Get(); }
		virtual ID3D11ShaderResourceView*	GetD3D11SRV()const noexcept override { return ShaderResourceView.Get(); }

		// Phase 1 transition helper: create the underlying cubemap through
		// IRRHIDevice::CreateTexture (cube path detected via MiscFlags).
		// D3D11 backend unwraps the returned IRRHITexture into the ComPtr
		// fields; D3D12 backend stores the abstract pointer in RHITexture.
		BOOL8 InitFromRHI(class IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc, const struct RRHISubresourceData* InInitialFaceData = nullptr, UINT32 InFaceCount = 0u);

		// Returns the abstract IRRHITexture to pass into IRCommandList bindings. On D3D12
		// RHITexture is the real resource and `this` is a metadata wrapper; on D3D11
		// `this` itself IS-A IRRHITexture through the legacy chain.
		IRRHITexture* GetResolvedRHITexture() { return RHITexture ? RHITexture : static_cast<IRRHITexture*>(this); }
		const IRRHITexture* GetResolvedRHITexture() const { return RHITexture ? RHITexture : static_cast<const IRRHITexture*>(this); }

		Microsoft::WRL::ComPtr<ID3D11Texture2D>				Buffer;
		Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	ShaderResourceView;
		UINT32												Width;
		UINT32												Height;
		UINT32												MipLevels;
		RFormatType											Format;
		UINT8												BindFlags;
		UINT32												BindlessSRVIndex;
		UINT32												BindlessUAVIndex;
		// D3D12 transition: populated only when the active back-end is D3D12.
		IRRHITexture*										RHITexture;
	};
	class RBufferResource : public ERTTIObject, public RRenderResourceInterface, public IRD3D11Buffer
	{
	public:
		RBufferResource()
			: Buffer(nullptr)
			, ByteSize(0u), StructureByteStride(0u), BindFlags(0u)
			, BindlessSRVIndex(RHI_INVALID_BINDLESS_INDEX), BindlessUAVIndex(RHI_INVALID_BINDLESS_INDEX)
			, RHIBuffer(nullptr) {
		}
		RBufferResource(const RBufferResource& Other)
			: Buffer(Other.Buffer)
			, ByteSize(Other.ByteSize), StructureByteStride(Other.StructureByteStride), BindFlags(Other.BindFlags)
			, BindlessSRVIndex(Other.BindlessSRVIndex), BindlessUAVIndex(Other.BindlessUAVIndex)
			, RHIBuffer(Other.RHIBuffer) {
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
			RHIBuffer			= Other.RHIBuffer;
			return (*this);
		}
		// BufferResource is raw buffer resource, so this function only determines by buffer valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHIBuffer)
			{
				return TRUE;
			}
			return (!!Buffer);
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
		{
			Buffer				= nullptr;
			ByteSize			= 0u;
			StructureByteStride	= 0u;
			BindFlags			= 0u;
			BindlessSRVIndex	= RHI_INVALID_BINDLESS_INDEX;
			BindlessUAVIndex	= RHI_INVALID_BINDLESS_INDEX;
			RHIBuffer			= nullptr;
		}

		// IRRHIBuffer
		virtual UINT64	GetByteSize()const noexcept override { return ByteSize; }
		virtual UINT32	GetStructureByteStride()const noexcept override { return StructureByteStride; }
		virtual UINT8	GetBindFlags()const noexcept override { return BindFlags; }
		virtual UINT32	GetBindlessSRVIndex()const noexcept override { return BindlessSRVIndex; }
		virtual UINT32	GetBindlessUAVIndex()const noexcept override { return BindlessUAVIndex; }

		// IRD3D11Buffer native getters
		virtual ID3D11Buffer* GetD3D11Buffer()const noexcept override { return Buffer.Get(); }

		// Phase 1 transition helper: create the underlying buffer through the
		// abstract IRRHIDevice. For D3D11 the returned heap RBufferResource
		// is unwrapped into the ComPtr field; for D3D12 the abstract pointer
		// is stored in RHIBuffer. Either path also updates ByteSize / Stride
		// / BindFlags on this resource.
		BOOL8 InitFromRHI(class IRRHIDevice* InRenderDevice, const RBufferDesc& InDesc, const struct RRHISubresourceData* InInitialData = nullptr);

		// Phase 1 transition helper: upload CPU data into the buffer through
		// IRRHIDevice::UploadBuffer. Picks RHIBuffer when set (D3D12) else
		// uses `this` (D3D11 RBufferResource IS-A IRRHIBuffer).
		void UploadDataViaRHI(class IRRHIDevice* InRenderDevice, const void* InData, SIZE_T InSize, SIZE_T InDestOffset = 0u);

		// Phase 2 transition helper: returns the abstract IRRHIBuffer to pass
		// into IRCommandList bindings. On D3D12 backends RHIBuffer is the real
		// resource and `this` is just a metadata wrapper; on D3D11 `this`
		// itself IS-A IRRHIBuffer through the legacy chain.
		IRRHIBuffer* GetResolvedRHIBuffer() { return RHIBuffer ? RHIBuffer : static_cast<IRRHIBuffer*>(this); }
		const IRRHIBuffer* GetResolvedRHIBuffer() const { return RHIBuffer ? RHIBuffer : static_cast<const IRRHIBuffer*>(this); }

		Microsoft::WRL::ComPtr<ID3D11Buffer>	Buffer;
		UINT64									ByteSize;
		UINT32									StructureByteStride;
		UINT8									BindFlags;
		UINT32									BindlessSRVIndex;
		UINT32									BindlessUAVIndex;
		// D3D12 transition: populated only when the active back-end is D3D12.
		IRRHIBuffer*							RHIBuffer;
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
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check index buffer short index failed."), (UseShort ? (IndexCount <= 65536u) : TRUE));
			return (RBufferResource::IsRenderResourceValid() && (IndexCount != 0u));
		}
		virtual void ReleaseRenderResource()noexcept override
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
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			return (RBufferResource::IsRenderResourceValid() && (Stride != 0u));
		}
		virtual void ReleaseRenderResource()noexcept override
		{
			RBufferResource::ReleaseRenderResource();
			Stride = 0u;
		}

		UINT32 Stride;
	};
	class RSamplerResource : public RRenderResourceInterface, public IRD3D11Sampler
	{
	public:
		RSamplerResource() : SamplerState(nullptr), RHISampler(nullptr) {}
		RSamplerResource(const RSamplerResource& Other) : SamplerState(Other.SamplerState), RHISampler(Other.RHISampler) {}
		virtual ~RSamplerResource() { ReleaseRenderResource(); }
		RSamplerResource& operator=(const RSamplerResource& Other)
		{
			SamplerState = Other.SamplerState;
			RHISampler = Other.RHISampler;
			return (*this);
		}
		// SamplerResource validation only determines by SamplerState valid.
		virtual BOOL8 IsRenderResourceValid()const noexcept override
		{
			if (RHISampler)
			{
				return TRUE;
			}
			return (!!SamplerState);
		}
		virtual BOOL8 InitRenderResource()noexcept override
		{
			// Render resource must init by specific type and input.
			// We do not want raw render resource init.
			return TRUE;
		}
		virtual void ReleaseRenderResource()noexcept override
		{
			SamplerState = nullptr;
			RHISampler = nullptr;
		}

		// IRD3D11Sampler native getter
		virtual ID3D11SamplerState* GetD3D11SamplerState()const noexcept override { return SamplerState.Get(); }

		Microsoft::WRL::ComPtr<ID3D11SamplerState> SamplerState;
		// D3D12 transition: populated only when the active back-end is D3D12.
		IRRHISampler*                              RHISampler;
	};
	class RSceneTextures final
	{
	public:
		void	InitResources(const Vector2Int& InTextureSize, const BOOL32 IsNeedStencil);
		void	ReleaseResources();
	public:
		RRenderTexture2D	SceneDepthStencil;
		RRenderTexture2D	SceneColor;
		RRenderTexture2D	GBufferA;
		RRenderTexture2D	GBufferB;
		RRenderTexture2D	GBufferC;
		// The identity / light channel mask target and the emissive target: what the deferred
		// lighting pass dispatches on and adds on top of the base pass colour. Their formats
		// are the ones Public/GBuffer/ShaderGBufferIO.hlsl states in its target table, and
		// GBufferF is the reserved last entry of that table, which the base pass writes and
		// nothing reads.
		RRenderTexture2D	GBufferD;
		RRenderTexture2D	GBufferE;
		RRenderTexture2D	GBufferF;
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
