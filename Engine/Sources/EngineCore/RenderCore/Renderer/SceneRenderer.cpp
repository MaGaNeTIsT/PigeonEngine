#include "SceneRenderer.h"
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSceneTextures::RSceneTextures()
		: TextureSize(Vector2Int::Zero())
	{
	}
	RSceneTextures::~RSceneTextures()
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(SceneDepthStencil.IsRenderResourceValid())));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(SceneColor.IsRenderResourceValid())));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferA.IsRenderResourceValid())));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferB.IsRenderResourceValid())));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene textures release failed."), (!(GBufferC.IsRenderResourceValid())));
	}
	void RSceneTextures::InitResources(const Vector2Int& InTextureSize)
	{
		Vector2Int UsedTextureSize(EMath::Max(2, InTextureSize.x), EMath::Max(2, InTextureSize.y));
		if ((TextureSize.x == UsedTextureSize.x) && (TextureSize.y == UsedTextureSize.y))
		{
			return;
		}
		TextureSize = UsedTextureSize;
		UINT32	TextureWidth	= static_cast<UINT32>(TextureSize.x);
		UINT32	TextureHeight	= static_cast<UINT32>(TextureSize.y);

		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		{
			RFormatType SRVFormat = RFormatType::FORMAT_R32_FLOAT;
			RenderDevice->CreateRenderTexture2D(SceneDepthStencil,
				RTextureDesc(TextureWidth, TextureHeight,
					RBindFlagType::BIND_DEPTH_STENCIL | RBindFlagType::BIND_SHADER_RESOURCE,
					RFormatType::FORMAT_UNKNOWN, &SRVFormat, nullptr, nullptr, 32u));
		}

		RenderDevice->CreateRenderTexture2D(SceneColor,
			RTextureDesc(TextureWidth, TextureHeight,
				RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
				RFormatType::FORMAT_R11G11B10_FLOAT));

		RenderDevice->CreateRenderTexture2D(GBufferA,
			RTextureDesc(TextureWidth, TextureHeight,
				RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
				RFormatType::FORMAT_R8G8B8A8_UNORM));

		RenderDevice->CreateRenderTexture2D(GBufferB,
			RTextureDesc(TextureWidth, TextureHeight,
				RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
				RFormatType::FORMAT_R10G10B10A2_UNORM));
		RenderDevice->CreateRenderTexture2D(GBufferC,
			RTextureDesc(TextureWidth, TextureHeight,
				RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
				RFormatType::FORMAT_R8G8B8A8_UNORM));
	}
	void RSceneTextures::ClearResources()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->ClearDepthStencilView(SceneDepthStencil.DepthStencilView);
		RenderDevice->ClearRenderTargetView(SceneColor.RenderTargetView);
		RenderDevice->ClearRenderTargetView(GBufferA.RenderTargetView);
		RenderDevice->ClearRenderTargetView(GBufferB.RenderTargetView);
		RenderDevice->ClearRenderTargetView(GBufferC.RenderTargetView);
	}
	void RSceneTextures::ReleaseResources()
	{
		SceneDepthStencil.ReleaseRenderResource();
		SceneColor.ReleaseRenderResource();
		GBufferA.ReleaseRenderResource();
		GBufferB.ReleaseRenderResource();
		GBufferC.ReleaseRenderResource();
	}

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr)
	{
	}
	RSceneRenderer::~RSceneRenderer()
	{
	}
	void RSceneRenderer::Initialize()
	{
		SceneTextures.InitResources(Vector2Int(RCommonSettings::RENDER_TARGET_WIDTH, RCommonSettings::RENDER_TARGET_HEIGHT));
	}
	void RSceneRenderer::ShutDown()
	{
		SceneTextures.ReleaseResources();
	}
	void RSceneRenderer::InitNewFrame()
	{
		InitViews();
	}
	void RSceneRenderer::Render()
	{

	}
	void RSceneRenderer::InitViews()
	{
		// Update requires
		{
			RCommand& UpdateCommands = Scene->GetUpdateCommands();
			UpdateCommands.DoCommands();
			UpdateCommands.EmptyQueue();
		}

		// Add or remove requires
		{
			RCommand& AddOrRemoveCommands = Scene->GetAddOrRemoveCommands();
			AddOrRemoveCommands.DoCommands();
			AddOrRemoveCommands.EmptyQueue();
		}

		for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			const EFrustum& ViewFrustum = ViewProxy->GetViewFrustum();

			// Init lights
			{
				InitLights(ViewProxy);
			}

			// Visible culling
			{
				RSceneRenderer::RVisibilityMapType& VisibilityMap = ViewProxy->GetVisibilityMap();
				OctreeCull(ViewFrustum, VisibilityMap);
				PrimitiveCull(ViewFrustum, VisibilityMap);
			}

			// Occlusion query
			{
				ProcessOcclusionCull(ViewProxy);
			}

			// Build render passes
			{
				InitRenderPasses(ViewProxy);
			}
		}
	}
	void RSceneRenderer::FinalOutputPass()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->SetFinalOutput();

	}
	void RSceneRenderer::InitLights(RViewProxy* InViewProxy)
	{
		// Directional lights
		InitDirectionalLights(InViewProxy);
	}
	void RSceneRenderer::ProcessOcclusionCull(RViewProxy* InViewProxy)
	{

	}
	void RSceneRenderer::InitRenderPasses(RViewProxy* InViewProxy)
	{
		//TODO
		SceneTextures.ClearResources();
		RDeviceD3D11::GetDeviceSingleton()->ClearFinalOutput();
	}
	void RSceneRenderer::InitDirectionalLights(RViewProxy* InViewProxy)
	{
		RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLightSceneProxies = Scene->GetDirectionalLightSceneProxies();
		for (UINT32 DirectionalLightIndex = 0u, DirectionalLightNum = DirectionalLightSceneProxies.GetSceneProxyCount(); DirectionalLightIndex < DirectionalLightNum; DirectionalLightIndex++)
		{
			RDirectionalLightSceneProxy* LightProxy = DirectionalLightSceneProxies.SceneProxies[DirectionalLightIndex];
#if _EDITOR_ONLY
			Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy can not be null"), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			LightProxy->GenerateViewInfo(InViewProxy);

			RDirectionalLightSceneProxy::RPerViewDomainInfoType& ViewDomainInfos = LightProxy->GetViewDomainInfos();
			Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy view domain infos can not be empty."), (ViewDomainInfos.ContainsKey(InViewProxy->GetUniqueID())));
			RDirectionalLightSceneProxy::RPerViewVisibilityMapType& VisibilityMaps = LightProxy->GetVisibilityMap();
			if (!(VisibilityMaps.ContainsKey(InViewProxy->GetUniqueID())))
			{
				VisibilityMaps.Add(InViewProxy->GetUniqueID(), TArray<RDirectionalLightSceneProxy::RVisibilityMapType>());
			}

			TArray<RDirectionalLightSceneProxy::RVisibilityMapType>&	LayerVisibilityMap	= VisibilityMaps[InViewProxy->GetUniqueID()];
			const TArray<EViewDomainInfo>&								DomainInfos			= ViewDomainInfos[InViewProxy->GetUniqueID()];
			Check((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), ((ViewDomainInfos.Length() > 0u) && ((!(LightProxy->IsLightUseCascadeShadow())) || ((LightProxy->IsLightUseCascadeShadow()) && (!!(LightProxy->GetCascadeShadowData())) && ((LightProxy->GetCascadeShadowData()->Layers.Length()) == (ViewDomainInfos.Length()))))));
			for (UINT32 DomainIndex = 0u, DomainNum = DomainInfos.Length(); DomainIndex < DomainNum; DomainIndex++)
			{
				if (DomainIndex >= LayerVisibilityMap.Length())
				{
					LayerVisibilityMap.Add(RDirectionalLightSceneProxy::RVisibilityMapType());
				}
				RDirectionalLightSceneProxy::RVisibilityMapType&	VisibilityMap	= LayerVisibilityMap[DomainIndex];
				const EViewDomainInfo&								DomainInfo		= DomainInfos[DomainIndex];
				OctreeCull(DomainInfo.ViewFrustum, VisibilityMap);
				PrimitiveCull(DomainInfo.ViewFrustum, VisibilityMap);
			}
		}
	}
	void RSceneRenderer::OctreeCull(const EFrustum& InViewFrustum, RSceneRenderer::RVisibilityMapType& InOutVisibilityMap)
	{
		ROctree& SceneOctree = Scene->GetSceneOctree();
		if (SceneOctree.GetPrimitiveNum() == 0u)
		{
			return;
		}

		const Vector3						TempAxis[3]			= { Vector3::XVector(), Vector3::YVector(), Vector3::ZVector() };
		const TArray<ROctreeElement>&		OctreeElements		= SceneOctree.GetElements();
		const TArray<ROctreeLayerInfo>&		OctreeLayerInfos	= SceneOctree.GetLayerInfos();
		const INT32							OctreeLayerNum		= (INT32)(OctreeLayerInfos.Length());

		INT32 TempDeep = 0;
		SceneOctree.BackwardRecursionNode(0, TempDeep,
			[&InOutVisibilityMap, &OctreeElements](ROctreeNode& InNode, INT32 InDeep)->INT32
			{
				Check((ENGINE_RENDER_CORE_ERROR), ("Check element index of octree node is valid failed."), ((InNode.ElementIndex >= 0) && (InNode.ElementIndex < ((INT32)(OctreeElements.Length())))));
				const ROctreeElement& OctreeElement = OctreeElements[InNode.ElementIndex];
				for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = OctreeElement.Primitives.Length(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
				{
					const RPrimitiveSceneProxy* PrimitiveSceneProxy = OctreeElement.Primitives[PrimitiveIndex];
#if _EDITOR_ONLY
					Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive scene proxy is null."), (!!PrimitiveSceneProxy));
					if (!PrimitiveSceneProxy)
					{
						continue;
					}
#endif
					//TODO We can use primitive bounds for more precise culling results.
					//TODO May use C++17 function for map type
					InOutVisibilityMap[PrimitiveSceneProxy->GetUniqueID()] = TRUE;
				}
				return (InDeep + 1);
			},
			[&TempAxis, &InViewFrustum, &OctreeLayerInfos, &OctreeLayerNum](ROctreeNode& InNode, INT32 InDeep)->BOOL32
			{
				Check((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene octree deep with layer infos failed."), (InDeep < OctreeLayerNum));
				const Vector3 NodeOrigin = InNode.Origin;
				const Vector3 NodeExtent = OctreeLayerInfos[InDeep].Extent;
				const FLOAT AxisExtent[3] = { NodeExtent.x, NodeExtent.x, NodeExtent.z };
				for (UINT32 i = 0u; i < 3u; i++)
				{
					const FLOAT NodeProjectOrigin = Vector3::Dot(TempAxis[i], NodeOrigin);
					if ((InViewFrustum.SeparateProjection[i].x > (NodeProjectOrigin + AxisExtent[i])) || (InViewFrustum.SeparateProjection[i].y < (NodeProjectOrigin - AxisExtent[i])))
					{
						return FALSE;
					}
				}
				return TRUE;
			});
	}
	void RSceneRenderer::PrimitiveCull(const EFrustum& InViewFrustum, RSceneRenderer::RVisibilityMapType& InOutVisibilityMap)
	{
		//Scene->GetSkeletalMeshSceneProxies;
	}

};