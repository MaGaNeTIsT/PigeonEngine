#include "RHIResources.h"
#include "IRRHIDevice.h"

namespace PigeonEngine
{

	// =====================================================================
	// IRRHIDevice high-level wrapper factories
	// =====================================================================
	// All factories follow the same pattern: build the corresponding
	// RRHIxxxDesc / RRHIShaderDesc, call the virtual low-level Create
	// method, populate the wrapper with the returned pointer + metadata.
	// Back-end polymorphism happens once, in the low-level virtual call.

	BOOL8 IRRHIDevice::CreateVertexBufferResource(const TArray<UINT8, UINT64>& InVertexData, UINT32 InVertexCount, UINT32 InStride, RRHIVertexBuffer& OutResource) noexcept
	{
		OutResource = RRHIVertexBuffer{};
		if ((InVertexCount == 0u) || (InStride == 0u))
		{
			return FALSE;
		}
		const UINT32 ByteWidth = InVertexCount * InStride;
		const RBufferDesc Desc{ ByteWidth, RBindFlagType::BIND_VERTEX_BUFFER, InStride };
		const RRHISubresourceData SubData{ (InVertexData.IsEmpty()) ? nullptr : (InVertexData.GetData()), (InVertexData.IsEmpty()) ? 0ull : (InVertexData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(Desc, SubData, &OutBuffer) || (!OutBuffer))
		{
			return FALSE;
		}
		OutResource.Buffer			= OutBuffer;
		OutResource.Stride			= InStride;
		OutResource.VertexCount		= InVertexCount;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateIndexBufferResource(const TArray<UINT8, UINT64>& InIndexData, UINT32 InIndexCount, ERHIIndexFormat InFormat, RRHIIndexBuffer& OutResource) noexcept
	{
		OutResource = RRHIIndexBuffer{};
		if (InIndexCount == 0u)
		{
			return FALSE;
		}
		const UINT32 Stride = (InFormat == ERHIIndexFormat::RHI_INDEX_FORMAT_UINT16) ? 2u : 4u;
		const UINT32 ByteWidth = InIndexCount * Stride;
		const RBufferDesc Desc{ ByteWidth, RBindFlagType::BIND_INDEX_BUFFER, Stride };
		const RRHISubresourceData SubData{ (InIndexData.IsEmpty()) ? nullptr : (InIndexData.GetData()), (InIndexData.IsEmpty()) ? 0ull : (InIndexData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(Desc, SubData, &OutBuffer) || (!OutBuffer))
		{
			return FALSE;
		}
		OutResource.Buffer			= OutBuffer;
		OutResource.IndexCount		= InIndexCount;
		OutResource.Format			= InFormat;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateConstantBufferResource(const TArray<UINT8, UINT64>& InInitialData, RRHIConstantBuffer& OutResource) noexcept
	{
		OutResource = RRHIConstantBuffer{};
		// Round size up to 16-byte alignment (D3D11/D3D12 constant buffer rule).
		const UINT64 AlignedSize = (InInitialData.Num<UINT64>() + 15ull) & (~(15ull));
		RBufferDesc Desc{ AlignedSize, RBindFlagType::BIND_CONSTANT_BUFFER, sizeof(float) };
		const RRHISubresourceData SubData{ (InInitialData.IsEmpty()) ? nullptr : (InInitialData.GetData()), (InInitialData.IsEmpty()) ? 0ull : (InInitialData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(Desc, SubData, &OutBuffer) || (!OutBuffer))
		{
			return FALSE;
		}
		OutResource.Buffer			= OutBuffer;
		OutResource.SizeInBytes		= AlignedSize;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateStructuredBufferResource(const TArray<UINT8, UINT64>& InInitialData, UINT32 InElementCount, UINT32 InStride, UINT8 InBindFlags, RRHIStructuredBuffer& OutResource) noexcept
	{
		OutResource = RRHIStructuredBuffer{};
		if ((InElementCount == 0u) || (InStride == 0u) || (InBindFlags == 0u))
		{
			return FALSE;
		}
		const UINT32 ByteWidth = InElementCount * InStride;
		RBufferDesc Desc{ ByteWidth, InBindFlags, InStride };
		const RRHISubresourceData SubData{ (InInitialData.IsEmpty()) ? nullptr : (InInitialData.GetData()), (InInitialData.IsEmpty()) ? 0ull : (InInitialData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(Desc, SubData, &OutBuffer) || (!OutBuffer))
		{
			return FALSE;
		}
		OutResource.Buffer				= OutBuffer;
		OutResource.Stride				= InStride;
		OutResource.ElementCount		= InElementCount;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateTexture2DResource(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, RRHITexture2D& OutResource) noexcept
	{
		OutResource = RRHITexture2D{};
		IRRHITexture* OutTexture{ nullptr };
		if (!CreateTexture(InDesc, InClearColor, InInitialSubresources, &OutTexture) || (!OutTexture))
		{
			return FALSE;
		}
		OutResource.Texture			= OutTexture;
		OutResource.Width			= InDesc.Width;
		OutResource.Height			= InDesc.Height;
		OutResource.MipLevels		= InDesc.MipLevels;
		OutResource.Format			= InDesc.BufferFormat;
		OutResource.BindFlags		= InDesc.BindFlags;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateTextureCubeResource(const RTextureDesc& InDesc, const Color4& InClearColor, const RRHISubresourceData& InInitialSubresources, RRHITextureCube& OutResource) noexcept
	{
		OutResource = RRHITextureCube{};
		if ((InDesc.MiscFlags & RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE) == 0)
		{
			return FALSE;
		}
		IRRHITexture* OutTexture{ nullptr };
		// Six faces are what makes the desc a cube; the back-ends slice the resource by it.
		RTextureDesc CubeDesc = InDesc;
		CubeDesc.ArraySize = 6u;
		if (!CreateTexture(CubeDesc, InClearColor, InInitialSubresources, &OutTexture) || (!OutTexture))
		{
			return FALSE;
		}
		OutResource.Texture			= OutTexture;
		OutResource.Size			= InDesc.Width;
		OutResource.MipLevels		= InDesc.MipLevels;
		OutResource.Format			= InDesc.BufferFormat;
		OutResource.BindFlags		= InDesc.BindFlags;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateRenderTexture2DResource(const RTextureDesc& InDesc, const Color4& InClearColor, RRHIRenderTexture2D& OutResource) noexcept
	{
		OutResource = RRHIRenderTexture2D{};
		if ((InDesc.BindFlags & RBindFlagType::BIND_RENDER_TARGET) == 0)
		{
			return FALSE;
		}
		IRRHITexture* OutTexture{ nullptr };
		if (!CreateTexture(InDesc, InClearColor, {}, &OutTexture) || (!OutTexture))
		{
			return FALSE;
		}
		OutResource.Texture			= OutTexture;
		OutResource.Width			= InDesc.Width;
		OutResource.Height			= InDesc.Height;
		OutResource.Format			= InDesc.BufferFormat;
		OutResource.BindFlags		= InDesc.BindFlags;
		return TRUE;
	}

	BOOL8 IRRHIDevice::CreateSamplerResource(const RSamplerState& InDesc, RRHISamplerHandle& OutResource) noexcept
	{
		OutResource = RRHISamplerHandle{};
		IRRHISampler* OutSampler = nullptr;
		if (!CreateSampler(InDesc, &OutSampler) || (!OutSampler))
		{
			return FALSE;
		}
		OutResource.Sampler = OutSampler;
		return TRUE;
	}

// Macro to keep all the trivial Release implementations consistent.
// Each wrapper queues its IRRHIxxx pointer through the device's deferred
// delete and clears the local handle so IsRenderResourceValid() reads
// false afterwards.
#define PE_RHI_RESOURCE_RELEASE_IMPL(__Type, __Field) \
	void __Type::ReleaseRenderResource() noexcept\
	{\
		__Field = nullptr;\
	}\
	void __Type::Release(IRRHIDevice* InDevice) noexcept\
	{\
		if ((!!InDevice) && (!(!(__Field))))\
		{\
			InDevice->DestroyResource(__Field);\
		}\
		__Field = nullptr;\
	}\

	PE_RHI_RESOURCE_RELEASE_IMPL(RRHIVertexBuffer, Buffer)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHIIndexBuffer, Buffer)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHIConstantBuffer, Buffer)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHIStructuredBuffer, Buffer)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHITexture2D, Texture)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHITextureCube, Texture)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHIRenderTexture2D, Texture)
	PE_RHI_RESOURCE_RELEASE_IMPL(RRHISamplerHandle, Sampler)

#undef PE_RHI_RESOURCE_RELEASE_IMPL

};
