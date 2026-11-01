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
		const RBufferDesc BufferDesc{ ByteWidth, RBindFlagType::BIND_VERTEX_BUFFER, InStride };
		const RRHISubresourceData SubData{ (InVertexData.IsEmpty()) ? nullptr : (InVertexData.GetData()), (InVertexData.IsEmpty()) ? 0ull : (InVertexData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(BufferDesc, SubData, &OutBuffer) || (!OutBuffer))
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
		const RBufferDesc BufferDesc{ ByteWidth, RBindFlagType::BIND_INDEX_BUFFER, Stride };
		const RRHISubresourceData SubData{ (InIndexData.IsEmpty()) ? nullptr : (InIndexData.GetData()), (InIndexData.IsEmpty()) ? 0ull : (InIndexData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(BufferDesc, SubData, &OutBuffer) || (!OutBuffer))
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
		RBufferDesc BufferDesc{ AlignedSize, RBindFlagType::BIND_CONSTANT_BUFFER, sizeof(float) };
		const RRHISubresourceData SubData{ (InInitialData.IsEmpty()) ? nullptr : (InInitialData.GetData()), (InInitialData.IsEmpty()) ? 0ull : (InInitialData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(BufferDesc, SubData, &OutBuffer) || (!OutBuffer))
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
		RBufferDesc BufferDesc{ ByteWidth, InBindFlags, InStride };
		const RRHISubresourceData SubData{ (InInitialData.IsEmpty()) ? nullptr : (InInitialData.GetData()), (InInitialData.IsEmpty()) ? 0ull : (InInitialData.Num<UINT64>()), 0u, 0u };
		IRRHIBuffer* OutBuffer{ nullptr };
		if (!CreateBuffer(BufferDesc, SubData, &OutBuffer) || (!OutBuffer))
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

	// =====================================================================
	// IRRHIDevice engine default resources
	// =====================================================================
	// These exist for one consumer: a binding whose reflection says it is exposed to the
	// editor, but that no value has been published for yet. That is a legal intermediate state
	// (the shader exists before the artist assigns its texture), so the binding side needs a
	// defined object to fall back to rather than an undefined descriptor slot.

	IRRHITexture* IRRHIDevice::GetDefaultTexture(const RDefaultBindingTextureType InTextureType) noexcept
	{
		const UINT32 TextureIndex = static_cast<UINT32>(InTextureType);
		if (TextureIndex >= static_cast<UINT32>(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_COUNT))
		{
			return nullptr;
		}
		if (DefaultTextures[TextureIndex])
		{
			return DefaultTextures[TextureIndex];
		}

		// One black pixel per element. Every shape uses the same payload; the cube and array
		// upload below repeats it once per face / slice.
		const UINT8 BlackPixels[8] = {};
		RTextureDesc TextureDesc(1u, 1u, static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE)
			, RFormatType::FORMAT_R8G8B8A8_UNORM);
		UINT32 SubresourceNum = 1u;
		switch (InTextureType)
		{
		case RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_CUBE:
			TextureDesc.ArraySize = 6u;
			TextureDesc.MiscFlags = static_cast<UINT8>(RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE);
			SubresourceNum = 6u;
			break;
		case RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_3D:
			// The back-ends decide between 2D and 3D from Depth > 1; a 1x1x1 volume cannot be
			// expressed through the shared desc convention.
			TextureDesc.Depth = 2u;
			break;
		case RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D_ARRAY:
			// Two slices so the SRV is created as a texture array rather than a 2D texture.
			TextureDesc.ArraySize = 2u;
			SubresourceNum = 2u;
			break;
		default:
			break;
		}

		IRRHITexture* CreatedTexture = nullptr;
		if ((!CreateTexture(TextureDesc, Color4::Black(), RRHISubresourceData(), &CreatedTexture)) || (!CreatedTexture))
		{
			return nullptr;
		}

		RRHISubresourceData Subresources[6];
		for (UINT32 SubresourceIndex = 0u; SubresourceIndex < SubresourceNum; SubresourceIndex++)
		{
			// A 1x1 element is one row; SlicePitch covers the two depth slices of the 3D shape.
			Subresources[SubresourceIndex] = RRHISubresourceData(BlackPixels, sizeof(BlackPixels), 4u, 4u);
		}
		UploadTexture(CreatedTexture, Subresources, SubresourceNum, 0u);

		DefaultTextures[TextureIndex] = CreatedTexture;
		return CreatedTexture;
	}

	IRRHIBuffer* IRRHIDevice::GetDefaultBuffer(const BOOL8 InIsStructured, const BOOL8 InIsUnorderedAccess) noexcept
	{
		IRRHIBuffer*& DefaultBuffer = ((InIsUnorderedAccess != FALSE)
			? ((InIsStructured != FALSE) ? DefaultStructuredUnorderedAccessBuffer : DefaultByteAddressUnorderedAccessBuffer)
			: ((InIsStructured != FALSE) ? DefaultStructuredBuffer : DefaultByteAddressBuffer));
		if (DefaultBuffer)
		{
			return DefaultBuffer;
		}

		// Sixteen bytes is one float4 element, enough for the smallest structured or byte
		// address declaration to read a full element of zeros. The bind flag is the one the
		// binding needs: the upload path leaves the buffer in that state, and a single object
		// cannot be parked in both the shader-resource and unordered-access states at once.
		const UINT8 ZeroBytes[16] = {};
		const UINT32 StructureByteStride = (InIsStructured != FALSE) ? 16u : 0u;
		const UINT8 BindFlags = ((InIsUnorderedAccess != FALSE)
			? static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS)
			: static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE));
		const RBufferDesc BufferDesc(static_cast<UINT64>(sizeof(ZeroBytes)), BindFlags
			, StructureByteStride
			, ((InIsStructured != FALSE) ? RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED : RResourceMiscFlagType::RESOURCE_MISC_NONE));
		IRRHIBuffer* CreatedBuffer = nullptr;
		const RRHISubresourceData InitialData(ZeroBytes, sizeof(ZeroBytes), 0u, 0u);
		if ((!CreateBuffer(BufferDesc, InitialData, &CreatedBuffer)) || (!CreatedBuffer))
		{
			return nullptr;
		}

		DefaultBuffer = CreatedBuffer;
		return DefaultBuffer;
	}

	void IRRHIDevice::ReleaseDefaultResources() noexcept
	{
		for (UINT32 TextureIndex = 0u; TextureIndex < static_cast<UINT32>(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_COUNT); TextureIndex++)
		{
			if (DefaultTextures[TextureIndex])
			{
				DestroyResource(DefaultTextures[TextureIndex]);
				DefaultTextures[TextureIndex] = nullptr;
			}
		}
		if (DefaultStructuredBuffer)
		{
			DestroyResource(DefaultStructuredBuffer);
			DefaultStructuredBuffer = nullptr;
		}
		if (DefaultByteAddressBuffer)
		{
			DestroyResource(DefaultByteAddressBuffer);
			DefaultByteAddressBuffer = nullptr;
		}
		if (DefaultStructuredUnorderedAccessBuffer)
		{
			DestroyResource(DefaultStructuredUnorderedAccessBuffer);
			DefaultStructuredUnorderedAccessBuffer = nullptr;
		}
		if (DefaultByteAddressUnorderedAccessBuffer)
		{
			DestroyResource(DefaultByteAddressUnorderedAccessBuffer);
			DefaultByteAddressUnorderedAccessBuffer = nullptr;
		}
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
