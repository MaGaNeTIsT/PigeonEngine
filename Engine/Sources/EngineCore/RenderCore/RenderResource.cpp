#include "RenderResource.h"
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RStructuredBuffer, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RRenderTexture2D, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RTexture2DResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RTextureCubeResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RBufferResource, ERTTIObject, RRenderResourceInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	BOOL8 RBufferResource::InitFromRHI(IRRHIDevice* InRenderDevice, const RBufferDesc& InDesc, const RRHISubresourceData* InInitialData)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		IRRHIBuffer* OutBuffer = nullptr;
		// The RHI spells "no initial data" as a default-constructed RRHISubresourceData
		// (Data == nullptr), which is the convention RDeviceD3D12::CreateBuffer uses.
		RRHISubresourceData InitialData;
		if (InInitialData)
		{
			InitialData = *InInitialData;
		}
		if (!InRenderDevice->CreateBuffer(InDesc, InitialData, &OutBuffer) || !OutBuffer)
		{
			return FALSE;
		}
		if (InRenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			// D3D11 backend's CreateBuffer wraps in a heap RBufferResource;
			// unwrap into this instance and destroy the wrapper.
			RBufferResource* D3D11Buf = static_cast<RBufferResource*>(OutBuffer);
			Buffer = D3D11Buf->Buffer;
			delete D3D11Buf;
		}
		else
		{
			// D3D12 (or other future back-ends): keep the abstract pointer.
			RHIBuffer = OutBuffer;
		}
		ByteSize            = InDesc.ByteWidth;
		StructureByteStride = InDesc.StructureByteStride;
		BindFlags           = InDesc.BindFlags;
		return TRUE;
	}

	void RBufferResource::UploadDataViaRHI(IRRHIDevice* InRenderDevice, const void* InData, SIZE_T InSize, SIZE_T InDestOffset)
	{
		if (!InRenderDevice || !InData || InSize == 0u)
		{
			return;
		}
		IRRHIBuffer* Target = RHIBuffer ? RHIBuffer : static_cast<IRRHIBuffer*>(this);
		InRenderDevice->UploadBuffer(Target, static_cast<const UINT8*>(InData), InSize, InDestOffset);
	}

	BOOL8 RStructuredBuffer::InitFromRHI(IRRHIDevice* InRenderDevice, const RBufferDesc& InDesc, const RRHISubresourceData* InInitialData)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		if (InRenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			// The D3D11 back-end builds the SRV / UAV pair for a structured resource at
			// creation time, which its own create helper already does; the generic buffer
			// path only produces the raw buffer. Ask for the structured resource.
			RSubresourceDataDesc Legacy;
			const RSubresourceDataDesc* LegacyPtr = nullptr;
			if (InInitialData && InInitialData->Data)
			{
				Legacy.pSysMem				= InInitialData->Data;
				Legacy.SysMemPitch			= InInitialData->RowPitch;
				Legacy.SysMemSlicePitch		= InInitialData->SlicePitch;
				LegacyPtr = &Legacy;
			}
			const UINT32 ElementNum = (InDesc.StructureByteStride > 0u)
				? static_cast<UINT32>(InDesc.ByteWidth / InDesc.StructureByteStride)
				: 0u;
			if (!RDeviceD3D11::GetDeviceSingleton()->CreateStructuredBuffer(*this
				, RStructuredBufferDesc(static_cast<UINT32>(InDesc.StructureByteStride), ElementNum), LegacyPtr))
			{
				return FALSE;
			}
		}
		else
		{
			// D3D12 (or other future back-ends): keep the abstract pointer; the
			// desc's structured misc flag and stride are what the back-end keys off.
			IRRHIBuffer* OutBuffer = nullptr;
			RRHISubresourceData InitialData;
			if (InInitialData)
			{
				InitialData = *InInitialData;
			}
			if (!InRenderDevice->CreateBuffer(InDesc, InitialData, &OutBuffer) || !OutBuffer)
			{
				return FALSE;
			}
			RHIBuffer = OutBuffer;
		}
		ByteSize            = InDesc.ByteWidth;
		StructureByteStride = InDesc.StructureByteStride;
		BindFlags           = InDesc.BindFlags;
		return TRUE;
	}

	void RStructuredBuffer::UploadDataViaRHI(IRRHIDevice* InRenderDevice, const void* InData, SIZE_T InSize, SIZE_T InDestOffset)
	{
		if (!InRenderDevice || !InData || InSize == 0u)
		{
			return;
		}
		IRRHIBuffer* Target = RHIBuffer ? RHIBuffer : static_cast<IRRHIBuffer*>(this);
		InRenderDevice->UploadBuffer(Target, static_cast<const UINT8*>(InData), InSize, InDestOffset);
	}

	BOOL8 RTexture2DResource::InitFromRHI(IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc, const RRHISubresourceData* InInitialData)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		IRRHITexture* OutTexture = nullptr;
		// Same "empty is a default-constructed RRHISubresourceData" convention as the
		// buffer path above; the clear colour only feeds the D3D12 clear-value hint.
		RRHISubresourceData InitialData;
		if (InInitialData)
		{
			InitialData = *InInitialData;
		}
		if (!InRenderDevice->CreateTexture(InDesc, Color4(0.f, 0.f, 0.f, 0.f), InitialData, &OutTexture) || !OutTexture)
		{
			return FALSE;
		}
		if (InRenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			// D3D11 backend returns RTexture2DResource* for SHADER_RESOURCE-only
			// textures. Unwrap into this instance and destroy the wrapper.
			RTexture2DResource* D3D11Tex = static_cast<RTexture2DResource*>(OutTexture);
			Buffer             = D3D11Tex->Buffer;
			ShaderResourceView = D3D11Tex->ShaderResourceView;
			Width              = D3D11Tex->Width;
			Height             = D3D11Tex->Height;
			MipLevels          = D3D11Tex->MipLevels;
			ArraySize          = D3D11Tex->ArraySize;
			Format             = D3D11Tex->Format;
			BindFlags          = D3D11Tex->BindFlags;
			delete D3D11Tex;
		}
		else
		{
			RHITexture = OutTexture;
			Width      = InDesc.Width;
			Height     = InDesc.Height;
			MipLevels  = InDesc.MipLevels;
			ArraySize  = InDesc.ArraySize;
			Format     = InDesc.BufferFormat;
			BindFlags  = InDesc.BindFlags;
		}
		return TRUE;
	}

	BOOL8 RTextureCubeResource::InitFromRHI(IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc, const RRHISubresourceData* InInitialFaceData, UINT32 InFaceCount)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		// Cube path is detected by the abstract CreateTexture via
		// MiscFlags == RESOURCE_MISC_TEXTURECUBE; make sure the caller's
		// desc has it set.
		RTextureDesc Desc = InDesc;
		Desc.MiscFlags = static_cast<RResourceMiscFlagType>(
			static_cast<UINT8>(Desc.MiscFlags) | static_cast<UINT8>(RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE));
		// A cube is a six-slice 2D array: the back-ends derive the resource slice count and
		// the cube SRV dimension from the array size, and the face uploads below address
		// subresources 1..5.
		Desc.ArraySize = 6u;

		IRRHITexture* OutTexture = nullptr;
		// The RHI takes a single subresource per creation call, so only the first face
		// of the caller's array reaches the back-end here; the remaining faces have to
		// be uploaded through IRRHIDevice::UploadTexture.
		RRHISubresourceData InitialFaceData;
		if (InInitialFaceData)
		{
			InitialFaceData = *InInitialFaceData;
		}
		if (!InRenderDevice->CreateTexture(Desc, Color4(0.f, 0.f, 0.f, 0.f), InitialFaceData, &OutTexture) || !OutTexture)
		{
			return FALSE;
		}
		if (InRenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			RTextureCubeResource* D3D11Tex = static_cast<RTextureCubeResource*>(OutTexture);
			Buffer             = D3D11Tex->Buffer;
			ShaderResourceView = D3D11Tex->ShaderResourceView;
			Width              = D3D11Tex->Width;
			Height             = D3D11Tex->Height;
			MipLevels          = D3D11Tex->MipLevels;
			Format             = D3D11Tex->Format;
			BindFlags          = D3D11Tex->BindFlags;
			delete D3D11Tex;
		}
		else
		{
			RHITexture = OutTexture;
			Width      = Desc.Width;
			Height     = Desc.Height;
			MipLevels  = Desc.MipLevels;
			Format     = Desc.BufferFormat;
			BindFlags  = Desc.BindFlags;
		}

		// The RHI carries one subresource per creation call, so the faces after the
		// first reach the back-end through the upload path. Face f's mip 0 sits at
		// subresource f * MipLevels (mips vary fastest inside a face).
		if (InInitialFaceData && InFaceCount > 1u)
		{
			IRRHITexture* UploadTarget = RHITexture ? RHITexture : static_cast<IRRHITexture*>(this);
			for (UINT32 Face = 1u; Face < InFaceCount; Face++)
			{
				InRenderDevice->UploadTexture(UploadTarget, &InInitialFaceData[Face], 1u, Face * Desc.MipLevels);
			}
		}
		return TRUE;
	}

	BOOL8 RRenderTexture2D::InitFromRHI(IRRHIDevice* InRenderDevice, const RTextureDesc& InDesc)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		IRRHITexture* OutTexture = nullptr;
		if (!InRenderDevice->CreateTexture(InDesc, Color4(0.f, 0.f, 0.f, 0.f), RRHISubresourceData(), &OutTexture) || !OutTexture)
		{
			return FALSE;
		}
		if (InRenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
		{
			// D3D11 backend returns RRenderTexture2D* for RT/DS/UAV-flagged
			// textures (see RDeviceD3D11::CreateTexture). Unwrap into self.
			RRenderTexture2D* D3D11Tex = static_cast<RRenderTexture2D*>(OutTexture);
			Buffer              = D3D11Tex->Buffer;
			RenderTargetView    = D3D11Tex->RenderTargetView;
			UnorderedAccessView = D3D11Tex->UnorderedAccessView;
			ShaderResourceView  = D3D11Tex->ShaderResourceView;
			DepthStencilView    = D3D11Tex->DepthStencilView;
			Width               = D3D11Tex->Width;
			Height              = D3D11Tex->Height;
			MipLevels           = D3D11Tex->MipLevels;
			ArraySize           = D3D11Tex->ArraySize;
			Format              = D3D11Tex->Format;
			// The view formats come along with the views: the render target comparison reads them
			// off this object when the D3D11 back-end is the one that created the texture.
			ViewFormats         = D3D11Tex->ViewFormats;
			BindFlags           = D3D11Tex->BindFlags;
			delete D3D11Tex;
		}
		else
		{
			RHITexture = OutTexture;
			Width      = InDesc.Width;
			Height     = InDesc.Height;
			MipLevels  = InDesc.MipLevels;
			ArraySize  = InDesc.ArraySize;
			Format     = InDesc.BufferFormat;
			BindFlags  = InDesc.BindFlags;
		}
		return TRUE;
	}

	RSceneTextures::RSceneTextures()
		: TextureSize(Vector2Int::Zero()), NeedStencil(FALSE)
	{
	}
	RSceneTextures::~RSceneTextures()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(SceneDepthStencil.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(SceneColor.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferA.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferB.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferC.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferD.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferE.IsRenderResourceValid())));
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferF.IsRenderResourceValid())));
	}
	void RSceneTextures::InitResources(const Vector2Int& InTextureSize, const BOOL32 IsNeedStencil)
	{
		Vector2Int UsedTextureSize(EMath::Max(2, InTextureSize.x), EMath::Max(2, InTextureSize.y));

		// The recorded size is not the only reason to rebuild. A target that has no back-end
		// texture has to be built whatever the size says - ReleaseResources clears the
		// resources without clearing the size, so a size-only test would leave the frame
		// drawing into wrappers that have nothing behind them. One target stands for all of
		// them because they are always built and released as a set.
		const BOOL32 NeedRecreateTexture = (TextureSize.x != UsedTextureSize.x)
			|| (TextureSize.y != UsedTextureSize.y)
			|| (SceneColor.IsRenderResourceValid() == FALSE);
		const BOOL32 NeedRecreateDepthStencil = NeedRecreateTexture || (NeedStencil != IsNeedStencil);

		if ((!NeedRecreateTexture) && (!NeedRecreateDepthStencil))
		{
			return;
		}

		const UINT32 TextureWidth = static_cast<UINT32>(UsedTextureSize.x);
		const UINT32 TextureHeight = static_cast<UINT32>(UsedTextureSize.y);

		// Phase 2: route through the abstract IRRHIDevice CreateTexture factory via
		// RRenderTexture2D::InitFromRHI. Works for both D3D11 and D3D12 back-ends - InitFromRHI
		// internally dispatches by backend type and stores the result in the appropriate field
		// (ComPtrs on D3D11, RHITexture on D3D12).
		//
		// The recorded size only moves once every target that needed rebuilding was built: a
		// failed creation is then retried by the next call instead of being remembered as done.
		// A target that silently stays a wrapper without a back-end texture is the worst kind of
		// failure here - a depth target with no depth view draws without occlusion, and a colour
		// target's view does not exist at all.
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		BOOL32 bAllTargetsCreated = TRUE;
		BOOL32 Result = TRUE;
		if (NeedRecreateDepthStencil)
		{
			Result = SceneDepthStencil.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_DEPTH_STENCIL | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_UNKNOWN, nullptr, nullptr, nullptr, 32u, NeedStencil));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif
		}
		if (NeedRecreateTexture)
		{
			// The GBuffer formats are the contract table's, which the pass table states as well:
			// the two have to agree, because a pipeline state is created with the formats its
			// pass declares and D3D12 refuses a draw whose bound target differs. SceneColor is
			// linear scene luminance in cd/m2, which the lighting pass adds to and which holds
			// the emissive a base pass surface wrote; its alpha is a free general purpose
			// channel, because the lighting pass blends with ZERO/ONE on alpha and therefore
			// preserves whatever the base pass put there. A is HDR base colour (UNORM would
			// clamp it), B is the octahedral normal with roughness and metallic in a 16 bit
			// channel pair, D carries the integer identity and light mask the lighting pass
			// dispatches on, and C, E and F follow the same table.
			//
			// Sixteen bit float and not eleven-ten, because the values are physically scaled:
			// sunlight is 100000 lux and a mirror reflecting it returns 1.5e9 cd/m2, so the
			// target has to hold what the shading produces rather than a display range. Neither
			// format reaches that - 65504 is the ceiling of both - which is why the direct
			// specular term carries its own guard; see PE_LIGHTING_SPECULAR_MAX. What sixteen
			// bit float buys over eleven-ten is the mantissa in the dark end, where a night
			// scene lives.
			Result = SceneColor.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R16G16B16A16_FLOAT));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			Result = GBufferA.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R16G16B16A16_FLOAT));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			Result = GBufferB.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R16G16B16A16_UNORM));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			Result = GBufferC.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R8G8B8A8_UNORM));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			Result = GBufferD.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R16G16B16A16_UINT));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			Result = GBufferE.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R11G11B10_FLOAT));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

			// The reserved last target of the contract table. It exists so the base pass pixel
			// stage has a target for every output it declares; nothing reads it.
			Result = GBufferF.InitFromRHI(RenderDevice,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_R8_UNORM));
			bAllTargetsCreated = ((bAllTargetsCreated != FALSE) && (Result != FALSE)) ? TRUE : FALSE;
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif
		}

		if (bAllTargetsCreated != FALSE)
		{
			TextureSize = UsedTextureSize;
			NeedStencil = IsNeedStencil;
		}
#if _EDITOR_ONLY
		else
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Scene textures could not be created; the views are retried by the next frame."));
		}
#endif
	}
	void RSceneTextures::ReleaseResources()
	{
		SceneDepthStencil.ReleaseRenderResource();
		SceneColor.ReleaseRenderResource();
		GBufferA.ReleaseRenderResource();
		GBufferB.ReleaseRenderResource();
		GBufferC.ReleaseRenderResource();
		GBufferD.ReleaseRenderResource();
		GBufferE.ReleaseRenderResource();
		GBufferF.ReleaseRenderResource();
	}
	const Vector2Int& RSceneTextures::GetTextureSize()const
	{
		return TextureSize;
	}
	BOOL32 RSceneTextures::IsNeedStencil()const
	{
		return NeedStencil;
	}

	RShadowTexture::RShadowTexture()
	{
	}
	RShadowTexture::~RShadowTexture()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shadow texture release failed."), (ShadowMaps.Num() == 0));
	}
	void RShadowTexture::InitResources(const TArray<Vector2Int>& InTextureSizes)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Init shadow texture's num can not smaller than 0."), (InTextureSizes.Num() > 0));
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		for (INT32 i = 0, n = InTextureSizes.Num(); i < n; i++)
		{
			const Vector2Int TextureSize(EMath::Max(2, InTextureSizes[i].x), EMath::Max(2, InTextureSizes[i].y));
			if (i >= TextureSizes.Num())
			{
				TextureSizes.Add(Vector2Int::Zero());
			}
			if (i >= ShadowMaps.Num())
			{
				ShadowMaps.AddDefaultGetRef();
			}
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shadow map num failed."), (TextureSizes.Num() == ShadowMaps.Num()));
			if ((TextureSizes[i].x == TextureSize.x) && (TextureSizes[i].y == TextureSize.y))
			{
				continue;
			}
			TextureSizes[i] = TextureSize;
			const UINT32 TextureWidth = static_cast<UINT32>(TextureSize.x);
			const UINT32 TextureHeight = static_cast<UINT32>(TextureSize.y);

#if _EDITOR_ONLY
			const BOOL32 Result =
#endif
				ShadowMaps[i].InitFromRHI(RenderDevice,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_DEPTH_STENCIL | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_UNKNOWN, nullptr, nullptr, nullptr, RCommonSettings::RENDER_SHADOW_MAP_DEPTH_BIT));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create shadow texture failed."), (Result));
#endif
		}
	}
	void RShadowTexture::ReleaseResources()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check shadow map num failed."), (TextureSizes.Num() == ShadowMaps.Num()));
		if (const INT32 ShadowMapNum = ShadowMaps.Num(); ShadowMapNum > 0)
		{
			for (INT32 i = 0; i < ShadowMapNum; i++)
			{
				ShadowMaps[i].ReleaseRenderResource();
			}
			ShadowMaps.Empty();
			TextureSizes.Empty();
		}
	}

};
