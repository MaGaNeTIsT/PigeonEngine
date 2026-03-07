#include "RenderResource.h"
#include <RenderDevice/DeviceD3D11.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RVertexShaderResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RPixelShaderResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RComputeShaderResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RStructuredBuffer, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RRenderTexture2D, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RRenderTexture3D, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RTexture2DResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RTextureCubeResource, ERTTIObject, RRenderResourceInterface>();
		RegisterClassType<RBufferResource, ERTTIObject, RRenderResourceInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

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
	}
	void RSceneTextures::InitResources(const Vector2Int& InTextureSize, const BOOL32 IsNeedStencil)
	{
		Vector2Int UsedTextureSize(EMath::Max(2, InTextureSize.x), EMath::Max(2, InTextureSize.y));

		const BOOL32 NeedRecreateTexture = (TextureSize.x != UsedTextureSize.x) || (TextureSize.y != UsedTextureSize.y);
		const BOOL32 NeedRecreateDepthStencil = NeedRecreateTexture || (NeedStencil != IsNeedStencil);

		if ((!NeedRecreateTexture) && (!NeedRecreateDepthStencil))
		{
			return;
		}

		TextureSize = UsedTextureSize;
		NeedStencil = IsNeedStencil;

		UINT32	TextureWidth = static_cast<UINT32>(TextureSize.x);
		UINT32	TextureHeight = static_cast<UINT32>(TextureSize.y);

		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

#if _EDITOR_ONLY
		BOOL32 Result = TRUE;
#endif
		if (NeedRecreateDepthStencil)
		{
#if _EDITOR_ONLY
			Result =
#endif
				RenderDevice->CreateRenderTexture2D(SceneDepthStencil,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_DEPTH_STENCIL | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_UNKNOWN, nullptr, nullptr, nullptr, 32u, NeedStencil));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif
		}
		if (NeedRecreateTexture)
		{
#if _EDITOR_ONLY
			Result =
#endif
				RenderDevice->CreateRenderTexture2D(SceneColor,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_R11G11B10_FLOAT));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

#if _EDITOR_ONLY
			Result =
#endif
				RenderDevice->CreateRenderTexture2D(GBufferA,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_R8G8B8A8_UNORM));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

#if _EDITOR_ONLY
			Result =
#endif
				RenderDevice->CreateRenderTexture2D(GBufferB,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_R10G10B10A2_UNORM));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif

#if _EDITOR_ONLY
			Result =
#endif
				RenderDevice->CreateRenderTexture2D(GBufferC,
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_R8G8B8A8_UNORM));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create scene textures failed."), (Result));
#endif
		}
	}
	void RSceneTextures::ClearResources()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->ClearDepthStencilView(SceneDepthStencil);
		RenderDevice->ClearRenderTargetView(SceneColor);
		RenderDevice->ClearRenderTargetView(GBufferA);
		RenderDevice->ClearRenderTargetView(GBufferB);
		RenderDevice->ClearRenderTargetView(GBufferC);
	}
	void RSceneTextures::ReleaseResources()
	{
		SceneDepthStencil.ReleaseRenderResource();
		SceneColor.ReleaseRenderResource();
		GBufferA.ReleaseRenderResource();
		GBufferB.ReleaseRenderResource();
		GBufferC.ReleaseRenderResource();
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
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
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
				RenderDevice->CreateRenderTexture2D(ShadowMaps[i],
					RTextureDesc(TextureWidth, TextureHeight,
						RBindFlagType::BIND_DEPTH_STENCIL | RBindFlagType::BIND_SHADER_RESOURCE,
						RFormatType::FORMAT_UNKNOWN, nullptr, nullptr, nullptr, RCommonSettings::RENDER_SHADOW_MAP_DEPTH_BIT));
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check create shadow texture failed."), (Result));
#endif
		}
	}
	void RShadowTexture::ClearResources()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Clear shadow texture's num can not smaller than 0."), (ShadowMaps.Num() > 0));
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		for (INT32 i = 0, n = ShadowMaps.Num(); i < n; i++)
		{
			RenderDevice->ClearDepthStencilView(ShadowMaps[i]);
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