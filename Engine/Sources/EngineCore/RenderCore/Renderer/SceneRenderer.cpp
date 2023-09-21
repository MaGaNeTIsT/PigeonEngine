#include "SceneRenderer.h"
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>
#include <ShaderAsset/ShaderAsset.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/LightSceneProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr), SimpleFullScreenVertexShader(nullptr), SimpleFullScreenPixelShader(nullptr), FinalOutputView(0u), NeedStencil(FALSE)
	{
	}
	RSceneRenderer::~RSceneRenderer()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene is not normally released."), (!Scene));
	}
	void RSceneRenderer::Initialize()
	{
		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState,
				RSamplerState(
					RFilterType::FILTER_POINT,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP));
			RenderDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState,
				RSamplerState(
					RFilterType::FILTER_POINT,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP));
			RenderDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState,
				RSamplerState(
					RFilterType::FILTER_LINEAR,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP));
			RenderDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState,
				RSamplerState(
					RFilterType::FILTER_LINEAR,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP));
		}

		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			RenderDevice->CreateRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_WIREFRAME].RasterizerState,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_WIREFRAME));
			RenderDevice->CreateRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_SOLID_NONE].RasterizerState,
				RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID));
			RenderDevice->CreateRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_SOLID_BACK].RasterizerState,
				RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID));
			RenderDevice->CreateRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_SOLID_FRONT].RasterizerState,
				RRasterizerState(RCullModeType::CULL_FRONT, RFillModeType::FILL_SOLID));
		}

		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			{
				const RBlendState BlendStates[] =
				{
					RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
								RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
								RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				};
				RenderDevice->CreateBlendState(Blend[RBlendType::BLEND_TYPE_OPAQUE_DEPTH].BlendState, BlendStates, 1u);
			}
			{
				const RBlendState BlendStates[] =
				{
					RBlendState(RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
								RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
								RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE),
				};
				RenderDevice->CreateBlendState(Blend[RBlendType::BLEND_TYPE_OPAQUE_BASEPASS].BlendState, BlendStates, 1u);
			}
		}

		{
			RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

			RStencilState TempNopStencilState(
				0xffu, 0xffu, RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP)
				, RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP), FALSE);

			RenderDevice->CreateDepthStencilState(DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_DEPTH_NOP_STENCIL_NOP].DepthStencilState,
				RDepthState(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, FALSE),
				&TempNopStencilState);
			RenderDevice->CreateDepthStencilState(DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_DEPTH_LESS_STENCIL_NOP].DepthStencilState,
				RDepthState(RComparisonFunctionType::COMPARISON_LESS),
				&TempNopStencilState);
			RenderDevice->CreateDepthStencilState(DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_DEPTH_LESS_EQUAL_STENCIL_NOP].DepthStencilState,
				RDepthState(RComparisonFunctionType::COMPARISON_LESS_EQUAL),
				&TempNopStencilState);
			RenderDevice->CreateDepthStencilState(DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_DEPTH_EQUAL_STENCIL_NOP].DepthStencilState,
				RDepthState(RComparisonFunctionType::COMPARISON_EQUAL),
				&TempNopStencilState);
		}

		{
			const EString ImportPath(ESettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
			const EString ImportVSName = EString("FullScreenTriangle_") + ESettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
			const RInputLayoutDesc TempShaderInputLayouts[] =
			{
				RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0)
			};
			const UINT32 TempShaderInputLayoutNum = 1u;
			TryLoadVertexShader(ESettings::ENGINE_SHADER_PATH, ImportVSName,
				SimpleFullScreenVertexShader,
				&ImportPath, &ImportVSName,
				TempShaderInputLayouts, &TempShaderInputLayoutNum);

			const EString ImportPSName = EString("FullScreenTriangle_") + ESettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(ESettings::ENGINE_SHADER_PATH, ImportPSName,
				SimpleFullScreenPixelShader,
				&ImportPath, &ImportPSName);
		}
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene is not normally released."), (!Scene));
		Scene = new RScene();
	}
	void RSceneRenderer::ShutDown()
	{
		if (ViewSceneTextures.Length() > 0u)
		{
			for (auto It = ViewSceneTextures.Begin(); It != ViewSceneTextures.End(); It++)
			{
				RSceneTextures*& TempPtr = It->second;
				if (TempPtr)
				{
					TempPtr->ReleaseResources();
				}
				delete (TempPtr);
				TempPtr = nullptr;
			}
			ViewSceneTextures.Clear();
		}
		if (ViewShadowMaps.Length() > 0u)
		{
			for (auto It = ViewShadowMaps.Begin(); It != ViewShadowMaps.End(); It++)
			{
				RShadowMapType& TempShadowMaps = It->second;
				if (TempShadowMaps.Length() > 0u)
				{
					for (auto ItSM = TempShadowMaps.Begin(); ItSM != TempShadowMaps.End(); ItSM++)
					{
						RShadowTexture*& TempPtr = ItSM->second;
						if (TempPtr)
						{
							TempPtr->ReleaseResources();
						}
						delete (TempPtr);
						TempPtr = nullptr;
					}
					TempShadowMaps.Clear();
				}
			}
			ViewSceneTextures.Clear();
		}

		{
			delete Scene;
			Scene = nullptr;
		}
	}
	RScene* RSceneRenderer::GetRenderScene()
	{
		return Scene;
	}
	void RSceneRenderer::InitNewFrame()
	{
		InitRendererSettings();

		InitViews();
	}
	void RSceneRenderer::Render()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState, 0u);
		RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState, 1u);
		RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState, 2u);
		RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState, 3u);

		RenderDevice->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		RenderDevice->SetRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_SOLID_BACK].RasterizerState);
		RenderDevice->SetBlendState(Blend[RBlendType::BLEND_TYPE_OPAQUE_BASEPASS].BlendState);
		RenderDevice->SetDepthStencilState(DepthStencil[RDepthStencilType::DEPTH_STENCIL_TYPE_DEPTH_LESS_EQUAL_STENCIL_NOP].DepthStencilState);

		BasePass();

		FinalOutputPass();
	}
	void RSceneRenderer::InitViews()
	{
		// Add, Update, Remove requires
		{
			RCommand& AddCommands = Scene->GetAddCommands();
			RCommand& UpdateCommands = Scene->GetUpdateCommands();
			RCommand& RemoveCommands = Scene->GetRemoveCommands();

			AddCommands.DoCommands();
			AddCommands.EmptyQueue();

			UpdateCommands.DoCommands();
			UpdateCommands.EmptyQueue();

			RemoveCommands.DoCommands();
			RemoveCommands.EmptyQueue();
		}

		// Reset view proxies per frame datas
		FinalOutputView = 0u;
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		{
			for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
			{
				RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
				if (!ViewProxy)
				{
					continue;
				}
#endif
				ViewProxy->ResetVisibilityMap();
				if (ViewProxy->IsMainRenderView())
				{
					FinalOutputView = ViewProxy->GetUniqueID();
				}
			}
		}

		for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
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
	void RSceneRenderer::BasePass()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

		const TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Length(); ViewIndex < ViewNum; ViewIndex++)
		{
			const RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			RenderDevice->SetViewport(ViewProxy->GetRenderViewport());

			RSceneTextures* SceneTextures = ViewSceneTextures[ViewProxy->GetUniqueID()];
			ViewProxy->BindRenderResource();

			RenderDevice->ClearRenderTargetView(SceneTextures->SceneColor.RenderTargetView);
			RenderDevice->ClearDepthStencilView(SceneTextures->SceneDepthStencil.DepthStencilView);
			RenderDevice->SetRenderTarget(SceneTextures->SceneColor.RenderTargetView, SceneTextures->SceneDepthStencil.DepthStencilView);

			RSceneProxyMapping<RStaticMeshSceneProxy>& StaticMeshes = Scene->GetStaticMeshSceneProxies();
			for (UINT32 StaticMeshIndex = 0u, StaticMeshNum = StaticMeshes.SceneProxies.Length(); StaticMeshIndex < StaticMeshNum; StaticMeshIndex++)
			{
				RStaticMeshSceneProxy* StaticMesh = StaticMeshes.SceneProxies[StaticMeshIndex];
#if _EDITOR_ONLY
				if ((!!StaticMesh) && (StaticMesh->IsRenderValid()))
#endif
				{
					StaticMesh->BindRenderResource();
					StaticMesh->Draw();
				}
			}
		}
	}
	void RSceneRenderer::FinalOutputPass()
	{
		RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		RenderDevice->ClearFinalOutput();
		RenderDevice->SetFinalOutput();
		FullScreenTriangle.BindPrimitiveBuffers();
		RenderDevice->SetInputLayout(SimpleFullScreenVertexShader->GetRenderResource()->InputLayout);
		RenderDevice->SetVSShader(SimpleFullScreenVertexShader->GetRenderResource()->Shader);
		RenderDevice->SetPSShader(SimpleFullScreenPixelShader->GetRenderResource()->Shader);
		if (FinalOutputView != 0u)
		{
			RenderDevice->BindPSShaderResourceView(ViewSceneTextures[FinalOutputView]->SceneColor.ShaderResourceView, 0u);
		}
		RenderDevice->DrawIndexed(FullScreenTriangle.GetIndexCount());
	}
	void RSceneRenderer::InitLights(RViewProxy* InViewProxy)
	{
		// Directional lights
		InitDirectionalLights(InViewProxy);

		// Shadow maps
		InitShadowTextures(InViewProxy);
	}
	void RSceneRenderer::ProcessOcclusionCull(RViewProxy* InViewProxy)
	{

	}
	void RSceneRenderer::InitRenderPasses(RViewProxy* InViewProxy)
	{
		{
			if (!(ViewSceneTextures.ContainsKey(InViewProxy->GetUniqueID())))
			{
				ViewSceneTextures.Add(InViewProxy->GetUniqueID(), nullptr);
			}
			RSceneTextures* SceneTextures = ViewSceneTextures[InViewProxy->GetUniqueID()];
			if (!SceneTextures)
			{
				SceneTextures = new RSceneTextures();
				ViewSceneTextures[InViewProxy->GetUniqueID()] = SceneTextures;
			}
			const EViewport& RenderViewport = InViewProxy->GetRenderViewport();
			SceneTextures->InitResources(Vector2Int(RenderViewport.Width, RenderViewport.Height), NeedStencil);
		}
	}
	void RSceneRenderer::InitDirectionalLights(RViewProxy* InViewProxy)
	{
		RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLightSceneProxies = Scene->GetDirectionalLightSceneProxies();
		for (UINT32 DirectionalLightIndex = 0u, DirectionalLightNum = DirectionalLightSceneProxies.GetSceneProxyCount(); DirectionalLightIndex < DirectionalLightNum; DirectionalLightIndex++)
		{
			RDirectionalLightSceneProxy* LightProxy = DirectionalLightSceneProxies.SceneProxies[DirectionalLightIndex];
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy can not be null"), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			LightProxy->GenerateViewInfo(InViewProxy);

			RDirectionalLightSceneProxy::RPerViewDomainInfoType& ViewDomainInfos = LightProxy->GetViewDomainInfos();
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy view domain infos can not be empty."), (ViewDomainInfos.ContainsKey(InViewProxy->GetUniqueID())));
			RDirectionalLightSceneProxy::RPerViewVisibilityMapType& VisibilityMaps = LightProxy->GetVisibilityMap();
			if (!(VisibilityMaps.ContainsKey(InViewProxy->GetUniqueID())))
			{
				VisibilityMaps.Add(InViewProxy->GetUniqueID(), TArray<RDirectionalLightSceneProxy::RVisibilityMapType>());
			}

			TArray<RDirectionalLightSceneProxy::RVisibilityMapType>&	LayerVisibilityMap	= VisibilityMaps[InViewProxy->GetUniqueID()];
			const TArray<EViewDomainInfo>&								DomainInfos			= ViewDomainInfos[InViewProxy->GetUniqueID()];
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), ((ViewDomainInfos.Length() > 0u) && ((!(LightProxy->IsLightUseCascadeShadow())) || ((LightProxy->IsLightUseCascadeShadow()) && (!!(LightProxy->GetCascadeShadowData())) && ((LightProxy->GetCascadeShadowData()->Layers.Length()) == (ViewDomainInfos.Length()))))));
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
	void RSceneRenderer::InitShadowTextures(RViewProxy* InViewProxy)
	{
		if (!(ViewShadowMaps.ContainsKey(InViewProxy->GetUniqueID())))
		{
			ViewShadowMaps.Add(InViewProxy->GetUniqueID(), RShadowMapType());
		}
		RShadowMapType& ShadowMaps = ViewShadowMaps[InViewProxy->GetUniqueID()];
		TMap<ObjectIdentityType, TArray<Vector2Int>>  LightProxyInfos;

		// Directional lights' shadow maps
		{
			RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLightSceneProxies = Scene->GetDirectionalLightSceneProxies();
			for (UINT32 DirectionalLightIndex = 0u, DirectionalLightNum = DirectionalLightSceneProxies.GetSceneProxyCount(); DirectionalLightIndex < DirectionalLightNum; DirectionalLightIndex++)
			{
				RDirectionalLightSceneProxy* LightProxy = DirectionalLightSceneProxies.SceneProxies[DirectionalLightIndex];
#if _EDITOR_ONLY
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy can not be null"), (!!LightProxy));
				if (!LightProxy)
				{
					continue;
				}
#endif
				TArray<Vector2Int> ShadowTextureSizes;
				RDirectionalLightSceneProxy::RPerViewDomainInfoType& ViewDomainInfos = LightProxy->GetViewDomainInfos();
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that light proxy view domain infos can not be empty."), (ViewDomainInfos.ContainsKey(InViewProxy->GetUniqueID())));
				const TArray<EViewDomainInfo>& DomainInfos = ViewDomainInfos[InViewProxy->GetUniqueID()];
				for (UINT32 DomainIndex = 0u, DomainNum = DomainInfos.Length(); DomainIndex < DomainNum; DomainIndex++)
				{
					const EViewDomainInfo& DomainInfo = DomainInfos[DomainIndex];
					ShadowTextureSizes.Add(Vector2Int(DomainInfo.RenderViewport.Width, DomainInfo.RenderViewport.Height));
				}
				LightProxyInfos.Add(LightProxy->GetUniqueID(), ShadowTextureSizes);
			}
		}

		// Init shadow maps
		for (auto It = LightProxyInfos.Begin(); It != LightProxyInfos.End(); It++)
		{
			const ObjectIdentityType& LightProxyID				= It->first;
			const TArray<Vector2Int>& LightProxyShadowMapSizes	= It->second;

			if (!(ShadowMaps.ContainsKey(LightProxyID)))
			{
				ShadowMaps.Add(LightProxyID, nullptr);
			}
			RShadowTexture* ShadowTexture = ShadowMaps[LightProxyID];
			if (!ShadowTexture)
			{
				ShadowTexture = new RShadowTexture();
				ShadowMaps[LightProxyID] = ShadowTexture;
			}
			ShadowTexture->InitResources(LightProxyShadowMapSizes);
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
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check element index of octree node is valid failed."), ((InNode.ElementIndex >= 0) && (InNode.ElementIndex < ((INT32)(OctreeElements.Length())))));
				const ROctreeElement& OctreeElement = OctreeElements[InNode.ElementIndex];
				for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = OctreeElement.Primitives.Length(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
				{
					const RPrimitiveSceneProxy* PrimitiveSceneProxy = OctreeElement.Primitives[PrimitiveIndex];
#if _EDITOR_ONLY
					PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check primitive scene proxy is null."), (!!PrimitiveSceneProxy));
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
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer scene octree deep with layer infos failed."), (InDeep < OctreeLayerNum));
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
	void RSceneRenderer::InitRendererSettings()
	{
		NeedStencil = IsNeedStencil();
	}
	BOOL32 RSceneRenderer::IsNeedStencil()const
	{
		//TODO
		return FALSE;
	}

};