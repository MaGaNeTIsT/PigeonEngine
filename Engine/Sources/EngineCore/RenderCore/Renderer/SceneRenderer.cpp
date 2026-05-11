#include "SceneRenderer.h"
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>
#include <ShaderAsset/ShaderAsset.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <RenderProxy/BezierGrassSceneProxy.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	void RViewLightCommonMaterialParameter::SetupParameters()
	{
		ClearParameter();
		BeginSetupParameter();
		AddParameter<Vector4Int, EShaderParameterValueType::SHADER_PARAMETER_TYPE_UINT4>(("_LightNum"));
		EndSetupParameter();
		CreateBuffer();
	}
	void RViewLightCommonMaterialParameter::UpdateRenderResource(const UINT32 InDirectionalLightNum, const UINT32 InPointLightNum, const UINT32 InSpotLightNum)
	{
		const UINT32 UsedDirectionalLightNum	= EMath::Min(RenderDirectionalLightNumMax, InDirectionalLightNum);
		const UINT32 UsedPointLightNum			= EMath::Min(RenderDirectionalLightNumMax, InPointLightNum);
		const UINT32 UsedSpotLightNum			= EMath::Min(RenderDirectionalLightNumMax, InSpotLightNum);
		const BOOL32 NeedUpdate = (UsedDirectionalLightNum != DirectionalLightNum) || (UsedPointLightNum != PointLightNum) || (UsedSpotLightNum != SpotLightNum);
		if (NeedUpdate)
		{
			SetLightNum(UsedDirectionalLightNum, UsedPointLightNum, UsedSpotLightNum);

			Vector4Int LightNum(Vector4Int::Zero());
			LightNum.v[DirectionalLightParameterIndex] = UsedDirectionalLightNum;
			LightNum.v[PointLightParameterIndex] = UsedPointLightNum;
			LightNum.v[SpotLightParameterIndex] = UsedSpotLightNum;

			(*this)["_LightNum"] = &TranslateUploadVectorType(LightNum);
			UploadBuffer();
		}
	}

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr), SimpleFullScreenVertexShader(nullptr), SimpleFullScreenPixelShader(nullptr), SceneLightingPixelShader(nullptr), FinalOutputView(0u), NeedStencil(FALSE)
	{
		for (UINT32 i = 0u; i < RPipelineStateType::PIPELINE_STATE_COUNT; i++)
		{
			PipelineStates[i] = nullptr;
		}
#if _EDITOR_ONLY
		{
			DebugWireframePrimitiveManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		}
#endif
	}
	RSceneRenderer::~RSceneRenderer()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene is not normally released."), (!Scene));
	}
	void RSceneRenderer::Initialize()
	{
#if _EDITOR_ONLY
		{
			DebugWireframePrimitiveManager->Initialize();
		}
#endif
		// Samplers stay as legacy RSamplerResource entries; they are not
		// part of any PSO (samplers are bound separately via SetSampler).
		{
			RDeviceD3D11* LegacyDevice = RDeviceD3D11::GetDeviceSingleton();
			LegacyDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState,
				RSamplerState(RFilterType::FILTER_POINT,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP));
			LegacyDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState,
				RSamplerState(RFilterType::FILTER_POINT,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP));
			LegacyDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState,
				RSamplerState(RFilterType::FILTER_LINEAR,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
					RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP));
			LegacyDevice->CreateSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState,
				RSamplerState(RFilterType::FILTER_LINEAR,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
					RTextureAddressModeType::TEXTURE_ADDRESS_WRAP));
		}

		// Shader assets need to load before pipeline state assembly so that
		// the full-screen lighting / output PSOs can pick up the shader
		// handles.
		{
			const EString ImportPath(EBaseSettings::ENGINE_RAW_SHADER_OUTPUT_PATH);
			const EString ImportVSName = EString("FullScreenTriangle") + EEngineSettings::ENGINE_IMPORT_VERTEX_SHADER_NAME_TYPE;
			const RInputLayoutDesc TempShaderInputLayouts[] =
			{
				RInputLayoutDesc(RShaderSemanticType::SHADER_SEMANTIC_TEXCOORD0, sizeof(FLOAT), 2u, RInputLayoutFormatType::INPUT_LAYOUT_FORMAT_FLOAT)
			};
			constexpr UINT32 TempShaderInputLayoutNum = PE_ARRAYSIZE(TempShaderInputLayouts);
			TryLoadVertexShader(EBaseSettings::ENGINE_SHADER_PATH, ImportVSName,
				SimpleFullScreenVertexShader,
				&ImportPath, &ImportVSName,
				TempShaderInputLayouts, &TempShaderInputLayoutNum);

			const EString ImportSimpleFullScreenPSName = EString("FullScreenTriangle") + EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(EBaseSettings::ENGINE_SHADER_PATH, ImportSimpleFullScreenPSName,
				SimpleFullScreenPixelShader,
				&ImportPath, &ImportSimpleFullScreenPSName);

			const EString ImportSceneLightingPSName = EString("SceneLighting") + EEngineSettings::ENGINE_IMPORT_PIXEL_SHADER_NAME_TYPE;
			TryLoadPixelShader(EBaseSettings::ENGINE_SHADER_PATH, ImportSceneLightingPSName,
				SceneLightingPixelShader,
				&ImportPath, &ImportSceneLightingPSName);
		}

		// Build pipeline states. Each PSO bakes (raster + blend + depth-
		// stencil + topology). Environment PSOs leave VS/PS empty; mesh
		// proxies running through them bind their own shaders inline.
		{
			IRRHIDevice* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

			const RStencilState NopStencil(
				0xffu, 0xffu,
				RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
				RStencilStateType(RComparisonFunctionType::COMPARISON_ALWAYS, RStencilOperationType::STENCIL_OP_KEEP),
				FALSE);

			const RBlendState BlendOff(
				RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
				RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
				RColorWriteMaskType::COLOR_WRITE_MASK_ALL, FALSE);
			const RBlendState BlendLighting(
				RBlendOptionType::BLEND_SRC_COLOR, RBlendOptionType::BLEND_DEST_COLOR, RBlendOperationType::BLEND_OP_ADD,
				RBlendOptionType::BLEND_ZERO, RBlendOptionType::BLEND_ONE, RBlendOperationType::BLEND_OP_ADD,
				RColorWriteMaskType::COLOR_WRITE_MASK_ALL, TRUE);

			const RDepthState DepthLessEqual(RComparisonFunctionType::COMPARISON_LESS_EQUAL);
			const RDepthState DepthNop(RComparisonFunctionType::COMPARISON_ALWAYS, RDepthWriteMaskType::DEPTH_WRITE_MASK_ALL, FALSE);

			IRRHIShader* FullScreenVS = nullptr;
			IRRHIShader* FullScreenPS = nullptr;
			IRRHIShader* LightingPS = nullptr;
			if (SimpleFullScreenVertexShader)
			{
				// Reuse the already-loaded RVertexShaderResource; its IRD3D11Shader
				// virtual getters expose the native ID3D11VertexShader and
				// pre-built ID3D11InputLayout.
				FullScreenVS = const_cast<RVertexShaderResource*>(SimpleFullScreenVertexShader->GetRenderResource());
			}
			if (SimpleFullScreenPixelShader)
			{
				FullScreenPS = const_cast<RPixelShaderResource*>(SimpleFullScreenPixelShader->GetRenderResource());
			}
			if (SceneLightingPixelShader)
			{
				LightingPS = const_cast<RPixelShaderResource*>(SceneLightingPixelShader->GetRenderResource());
			}

			// PIPELINE_STATE_BASE_PASS_MRT (env) - MRT into 4 GBuffers + depth, cull back, no blend, depth less-equal
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthLessEqual;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 4u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;	// SceneColor
				Desc.RenderTargetFormats[1]	= RFormatType::FORMAT_R8G8B8A8_UNORM;	// GBufferA
				Desc.RenderTargetFormats[2]	= RFormatType::FORMAT_R10G10B10A2_UNORM;// GBufferB
				Desc.RenderTargetFormats[3]	= RFormatType::FORMAT_R8G8B8A8_UNORM;	// GBufferC
				Desc.DepthStencilFormat		= RFormatType::FORMAT_R24G8_TYPELESS;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_BASE_PASS_MRT]);
			}
			// PIPELINE_STATE_BASE_PASS_GRASS_SOLID (env) - cull none
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthLessEqual;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 4u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;
				Desc.RenderTargetFormats[1]	= RFormatType::FORMAT_R8G8B8A8_UNORM;
				Desc.RenderTargetFormats[2]	= RFormatType::FORMAT_R10G10B10A2_UNORM;
				Desc.RenderTargetFormats[3]	= RFormatType::FORMAT_R8G8B8A8_UNORM;
				Desc.DepthStencilFormat		= RFormatType::FORMAT_R24G8_TYPELESS;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_SOLID]);
			}
			// PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME (env)
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_NONE, RFillModeType::FILL_WIREFRAME);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthLessEqual;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 4u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;
				Desc.RenderTargetFormats[1]	= RFormatType::FORMAT_R8G8B8A8_UNORM;
				Desc.RenderTargetFormats[2]	= RFormatType::FORMAT_R10G10B10A2_UNORM;
				Desc.RenderTargetFormats[3]	= RFormatType::FORMAT_R8G8B8A8_UNORM;
				Desc.DepthStencilFormat		= RFormatType::FORMAT_R24G8_TYPELESS;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME]);
			}
			// PIPELINE_STATE_LIGHTING (full) - additive full-screen pass
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.VertexShader			= FullScreenVS;
				Desc.PixelShader			= LightingPS;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendLighting;
				Desc.Depth					= DepthNop;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 1u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_LIGHTING]);
			}
			// PIPELINE_STATE_SKY (env) - cull front for sky dome
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_FRONT, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthLessEqual;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 1u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;
				Desc.DepthStencilFormat		= RFormatType::FORMAT_R24G8_TYPELESS;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_SKY]);
			}
			// PIPELINE_STATE_FORWARD (env) - forward-rendered geometry / debug primitives
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthLessEqual;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 1u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R11G11B10_FLOAT;
				Desc.DepthStencilFormat		= RFormatType::FORMAT_R24G8_TYPELESS;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_FORWARD]);
			}
			// PIPELINE_STATE_FINAL_OUTPUT (full) - blit to back buffer
			{
				RRHIGraphicsPipelineDesc Desc;
				Desc.VertexShader			= FullScreenVS;
				Desc.PixelShader			= FullScreenPS;
				Desc.Rasterizer				= RRasterizerState(RCullModeType::CULL_BACK, RFillModeType::FILL_SOLID);
				Desc.Blend					= BlendOff;
				Desc.Depth					= DepthNop;
				Desc.Stencil				= NopStencil;
				Desc.PrimitiveTopology		= RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST;
				Desc.RenderTargetCount		= 1u;
				Desc.RenderTargetFormats[0]	= RFormatType::FORMAT_R8G8B8A8_UNORM;
				RenderDevice->CreateGraphicsPipelineState(Desc, &PipelineStates[PIPELINE_STATE_FINAL_OUTPUT]);
			}
		}

		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene is not normally released."), (!Scene));
		Scene = new RScene();
	}
	void RSceneRenderer::ShutDown()
	{
		if (ViewSceneTextures.Num() > 0)
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
			ViewSceneTextures.Empty();
		}
		if (ViewShadowMaps.Num() > 0)
		{
			for (auto It = ViewShadowMaps.Begin(); It != ViewShadowMaps.End(); It++)
			{
				RShadowMapType& TempShadowMaps = It->second;
				if (TempShadowMaps.Num() > 0)
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
					TempShadowMaps.Empty();
				}
			}
			ViewSceneTextures.Empty();
		}

		// Release pipeline states.
		{
			IRRHIDevice* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
			for (UINT32 i = 0u; i < RPipelineStateType::PIPELINE_STATE_COUNT; i++)
			{
				if (PipelineStates[i])
				{
					RenderDevice->DestroyResource(PipelineStates[i]);
					PipelineStates[i] = nullptr;
				}
			}
		}

		{
			delete Scene;
			Scene = nullptr;
		}

#if _EDITOR_ONLY
		{
			DebugWireframePrimitiveManager->ShutDown();
		}
#endif
	}
	RScene* RSceneRenderer::GetRenderScene()
	{
		return Scene;
	}
	void RSceneRenderer::ResizeRenderTargets(UINT32 InWidth, UINT32 InHeight)
	{
		const Vector2Int NewSize(static_cast<INT32>(InWidth), static_cast<INT32>(InHeight));
		for (auto It = ViewSceneTextures.Begin(); It != ViewSceneTextures.End(); It++)
		{
			RSceneTextures* SceneTextures = It->second;
			if (SceneTextures)
			{
				SceneTextures->InitResources(NewSize, NeedStencil);
			}
		}
	}
	void RSceneRenderer::InitNewFrame()
	{
		// SwapCommandSlots is called by the GameThread at frame boundary
		// (between WaitForRenderIdle and KickRender) so that here we are
		// guaranteed the front slot is the freshly-published GameThread
		// payload and BackSlot is reserved for the next GameThread tick.

#if _EDITOR_ONLY
		DebugWireframePrimitiveManager->InitNewFrame();
#endif
		InitRendererSettings();

		InitViews();
	}
	void RSceneRenderer::Render()
	{
		IRRHIDevice* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
		CurrentCommandList = RenderDevice->AcquireCommandList();

		// Bind global samplers across stages. Sampler slots 0..3 are fixed
		// engine-wide (point clamp / point wrap / linear clamp / linear wrap).
		for (UINT32 SamplerIndex = 0u; SamplerIndex < RSamplerType::SAMPLER_TYPE_COUNT; SamplerIndex++)
		{
			IRRHISampler* SamplerPtr = &Samplers[SamplerIndex];
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_VERTEX, SamplerIndex, SamplerPtr);
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, SamplerIndex, SamplerPtr);
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, SamplerIndex, SamplerPtr);
		}

		BasePass();

		FinalOutputPass();

		RenderDevice->SubmitCommandList(CurrentCommandList);
		CurrentCommandList = nullptr;
	}
	void RSceneRenderer::BasePass()
	{
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check renderer failed that view proxy can not be null"), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			const EViewport& Viewport = ViewProxy->GetRenderViewport();
			const RRHIViewport NewViewport(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.Width, Viewport.Height, Viewport.MinDepth, Viewport.MaxDepth);
			CurrentCommandList->SetViewports(&NewViewport, 1u);

			RSceneTextures* SceneTextures = ViewSceneTextures[ViewProxy->GetUniqueID()];
			ViewProxy->UpdateRenderResource();
			ViewProxy->BindRenderResource(0u);

			RenderBasePass(SceneTextures);

			RenderSky(ViewProxy);

			RenderLighting(ViewProxy, SceneTextures);

			RenderForward(ViewProxy, SceneTextures);
		}
	}
	void RSceneRenderer::FinalOutputPass()
	{
		IRRHIDevice* RenderDevice = RDeviceD3D11::GetDeviceSingleton();

		// Backbuffer pass: clear to dark grey, blit SceneColor.
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 1u;
		Pass.RenderTargets[0].Target		= RenderDevice->GetCurrentBackBuffer();
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[0].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.RenderTargets[0].ClearColor	= Color4(0.5f, 0.5f, 0.5f, 1.f);
		Pass.DepthStencil.Target			= nullptr;
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_FINAL_OUTPUT]);

		FullScreenTriangle.BindPrimitiveBuffers();
		if (FinalOutputView != 0u)
		{
			CurrentCommandList->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, 0u, &(ViewSceneTextures[FinalOutputView]->SceneColor));
		}
		CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);

		CurrentCommandList->EndRenderPass();
	}
	void RSceneRenderer::RenderBasePass(RSceneTextures* InSceneTextures)
	{
		// MRT pass: 4 colour render targets + depth-stencil. LoadOp=CLEAR
		// replaces the legacy InSceneTextures->ClearResources() call.
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 4u;
		Pass.RenderTargets[0].Target		= &(InSceneTextures->SceneColor);
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[0].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[1].Target		= &(InSceneTextures->GBufferA);
		Pass.RenderTargets[1].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[1].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[2].Target		= &(InSceneTextures->GBufferB);
		Pass.RenderTargets[2].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[2].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[3].Target		= &(InSceneTextures->GBufferC);
		Pass.RenderTargets[3].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[3].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.DepthStencil.Target			= &(InSceneTextures->SceneDepthStencil);
		Pass.DepthStencil.DepthLoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.DepthStencil.StencilLoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.DepthStencil.ClearDepth		= 1.f;
		Pass.DepthStencil.ClearStencil		= 0u;
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_MRT]);

		// Static mesh part - mesh proxy binds its own VS/PS/IB/VB and Draws.
		RSceneProxyMapping<RStaticMeshSceneProxy>& StaticMeshes = Scene->GetStaticMeshSceneProxies();
		for (UINT32 StaticMeshIndex = 0u, StaticMeshNum = StaticMeshes.GetSceneProxyCount(); StaticMeshIndex < StaticMeshNum; StaticMeshIndex++)
		{
			RStaticMeshSceneProxy* StaticMesh = StaticMeshes.SceneProxies[StaticMeshIndex];
#if _EDITOR_ONLY
			if (!StaticMesh)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a null static mesh proxy."));
				continue;
			}
			if (!(StaticMesh->IsRenderValid()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a render invalid static mesh proxy."));
				continue;
			}
#endif
			StaticMesh->BindRenderResource();
			StaticMesh->Draw();
		}

		// BezierGrass - swaps in its own pipeline (wireframe / solid-none) per
		// proxy. Restore base PSO afterwards so subsequent meshes get the
		// expected raster mode back.
		RSceneProxyMapping<RBezierGrassSceneProxy>& BezierGrasses = Scene->GetBezierGrassSceneProxies();
		for (UINT32 BezierGrassIndex = 0u, NumBezierGrasses = BezierGrasses.GetSceneProxyCount(); BezierGrassIndex < NumBezierGrasses; BezierGrassIndex++)
		{
			RBezierGrassSceneProxy* BezierGrass = BezierGrasses.SceneProxies[BezierGrassIndex];
#if _EDITOR_ONLY
			if (!BezierGrass)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a null bezier grass proxy."));
				continue;
			}
			if (!(BezierGrass->IsRenderValid()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a render invalid bezier grass proxy."));
				continue;
			}
#endif
			if (BezierGrass->Property.bWireframe)
			{
				CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME]);
			}
			else
			{
				CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_SOLID]);
			}
			BezierGrass->DispatchComputeShader();
			BezierGrass->BindRenderResource();
			BezierGrass->Draw();
		}
		// Restore MRT pipeline for skeletal meshes.
		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_MRT]);

		// Skeletal mesh part
		RSceneProxyMapping<RSkeletalMeshSceneProxy>& SkeletalMeshes = Scene->GetSkeletalMeshSceneProxies();
		for (UINT32 SkeletalMeshIndex = 0u, SkeletalMeshNum = SkeletalMeshes.GetSceneProxyCount(); SkeletalMeshIndex < SkeletalMeshNum; SkeletalMeshIndex++)
		{
			RSkeletalMeshSceneProxy* SkeletalMesh = SkeletalMeshes.SceneProxies[SkeletalMeshIndex];
#if _EDITOR_ONLY
			if (!SkeletalMesh)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a null skeletal mesh proxy."));
				continue;
			}
			if (!(SkeletalMesh->IsRenderValid()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a render invalid skeletal mesh proxy."));
				continue;
			}
#endif
			SkeletalMesh->BindRenderResource();
			SkeletalMesh->Draw();
		}

		CurrentCommandList->EndRenderPass();
	}
	void RSceneRenderer::RenderLighting(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		const RViewLightCommonMaterialParameter* LightCommonParams = ViewLightCommonParams.FindValueAsPtr(InViewProxy->GetUniqueID());
		Check((!!LightCommonParams), (ENGINE_RENDER_CORE_ERROR));
		// Light count CB binds at slot 1 across stages (matches the legacy
		// RootIndex semantics broadcast by RCommandListD3D11).
		CurrentCommandList->SetGraphicsRootConstantBuffer(1u, const_cast<RBufferResource*>(&(LightCommonParams->GetConstantBuffer())));

		const RDirectionalLightMaterialParameter* DLightParams = ViewDLightParams.FindValueAsPtr(InViewProxy->GetUniqueID());
		if (!DLightParams)
		{
			return;
		}

		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount			= 1u;
		Pass.RenderTargets[0].Target	= &(InSceneTextures->SceneColor);
		Pass.RenderTargets[0].LoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;	// additive blend onto base pass colour
		Pass.RenderTargets[0].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target		= nullptr;
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_LIGHTING]);

		FullScreenTriangle.BindPrimitiveBuffers();

		CurrentCommandList->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, 0u, &(InSceneTextures->GBufferA));
		CurrentCommandList->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, 1u, &(InSceneTextures->GBufferB));
		CurrentCommandList->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, 2u, &(InSceneTextures->GBufferC));
		// Directional light parameter buffer lives in an RStructuredBuffer.
		IRRHIBuffer* DLightBuffer = const_cast<RStructuredBuffer*>(&(DLightParams->GetStructBuffer()));
		CurrentCommandList->SetShaderResourceView(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, 3u, DLightBuffer);

		CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);

		CurrentCommandList->EndRenderPass();
	}
	void RSceneRenderer::RenderSky(const RViewProxy* InViewProxy)
	{
		RSceneProxyMapping<RSkyLightSceneProxy>& SkyLights = Scene->GetSkyLightProxies();
		const UINT32 SkyLightNum = SkyLights.GetSceneProxyCount();
		if (SkyLightNum == 0u)
		{
			return;
		}

		// Sky writes back to the same MRT bound by RenderBasePass; the only
		// difference is rasterizer state (cull FRONT for inside-the-dome).
		// We rebind the same render targets explicitly so the pass is
		// self-contained on D3D12 in Phase 3.
		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_SKY]);

		for (UINT32 SkyLightIndex = 0u; SkyLightIndex < SkyLightNum; SkyLightIndex++)
		{
			RSkyLightSceneProxy* SkyLight = SkyLights.SceneProxies[SkyLightIndex];
#if _EDITOR_ONLY
			if (!SkyLight)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a null sky light proxy."));
				continue;
			}
			if (!(SkyLight->IsRenderValid()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a render invalid sky light proxy."));
				continue;
			}
#endif
			SkyLight->BindRenderResource();
			SkyLight->Draw();
#if _EDITOR_ONLY
			break;
#endif
		}
	}
	void RSceneRenderer::RenderForward(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount			= 1u;
		Pass.RenderTargets[0].Target	= &(InSceneTextures->SceneColor);
		Pass.RenderTargets[0].LoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.RenderTargets[0].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target		= &(InSceneTextures->SceneDepthStencil);
		Pass.DepthStencil.DepthLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.DepthStencil.DepthStoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.StencilLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.DepthStencil.StencilStoreOp= ERHIStoreOp::RHI_STORE_OP_STORE;
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_FORWARD]);

#if _EDITOR_ONLY
		{
			CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST);
			DebugWireframePrimitiveManager->RenderPrimitives_RenderThread(InViewProxy);
			CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
#endif

		CurrentCommandList->EndRenderPass();
	}
	void RSceneRenderer::InitViews()
	{
		// Add, Update, Remove requires
		{
			RCommand& AddCommands = Scene->GetAddCommandsForRender();
			RCommand& UpdateCommands = Scene->GetUpdateCommandsForRender();
			RCommand& RemoveCommands = Scene->GetRemoveCommandsForRender();

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
			for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
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

		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
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
	void RSceneRenderer::InitLights(RViewProxy* InViewProxy)
	{
		if (!(ViewLightCommonParams.ContainsKey(InViewProxy->GetUniqueID())))
		{
			ViewLightCommonParams.Add(InViewProxy->GetUniqueID(), RViewLightCommonMaterialParameter());
			ViewLightCommonParams[InViewProxy->GetUniqueID()].SetupParameters();
		}

		// Directional lights
		const UINT32 DLightNum = InitDirectionalLights(InViewProxy);

		const UINT32 PLightNum = 0u;
		const UINT32 SLightNum = 0u;

		ViewLightCommonParams[InViewProxy->GetUniqueID()].UpdateRenderResource(DLightNum, PLightNum, SLightNum);

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
	UINT32 RSceneRenderer::InitDirectionalLights(RViewProxy* InViewProxy)
	{
		UINT32 UsedDLightNum = 0u;
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

			if (!(ViewDLightParams.ContainsKey(InViewProxy->GetUniqueID())))
			{
				ViewDLightParams.Add(InViewProxy->GetUniqueID(), RDirectionalLightMaterialParameter());
			}
			UsedDLightNum += 1u;

			TArray<RDirectionalLightSceneProxy::RVisibilityMapType>&	LayerVisibilityMap	= VisibilityMaps[InViewProxy->GetUniqueID()];
			const TArray<EViewDomainInfo>&								DomainInfos			= ViewDomainInfos[InViewProxy->GetUniqueID()];
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), ((ViewDomainInfos.Num() > 0) && ((!(LightProxy->IsLightUseCascadeShadow())) || ((LightProxy->IsLightUseCascadeShadow()) && (!!(LightProxy->GetCascadeShadowData())) && ((LightProxy->GetCascadeShadowData()->Layers.Num()) == (ViewDomainInfos.Num()))))));
			for (INT32 DomainIndex = 0, DomainNum = DomainInfos.Num(); DomainIndex < DomainNum; DomainIndex++)
			{
				if (DomainIndex >= LayerVisibilityMap.Num())
				{
					LayerVisibilityMap.Add(RDirectionalLightSceneProxy::RVisibilityMapType());
				}
				RDirectionalLightSceneProxy::RVisibilityMapType&	VisibilityMap	= LayerVisibilityMap[DomainIndex];
				const EViewDomainInfo&								DomainInfo		= DomainInfos[DomainIndex];
				OctreeCull(DomainInfo.ViewFrustum, VisibilityMap);
				PrimitiveCull(DomainInfo.ViewFrustum, VisibilityMap);
			}
		}

		{
			RDirectionalLightMaterialParameter& DLightParams = ViewDLightParams[InViewProxy->GetUniqueID()];
			if (UsedDLightNum > 0u)
			{
				if (DLightParams.GetElementNum() != UsedDLightNum)
				{
					DLightParams.ClearBuffer();
					DLightParams.ClearParameter();
					DLightParams.BeginSetupStructParameter(UsedDLightNum);
					DLightParams.AddLightParameters();
					DLightParams.EndSetupStructParameter();
					DLightParams.CreateBuffer();

					UINT32 DLightIndex = 0u;
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
						RDirectionalLightSceneProxy::RPerViewDomainInfoType& ViewDomainInfos = LightProxy->GetViewDomainInfos();
						const TArray<EViewDomainInfo>& DomainInfos = ViewDomainInfos[InViewProxy->GetUniqueID()];
						PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), (ViewDomainInfos.Num() > 0));
						const EViewDomainInfo& DomainInfo = DomainInfos[0];

						DLightParams.UpdateParameterValue(DLightIndex, LightProxy->GetWorldRotation(), DomainInfo, LightProxy->GetLightData());
						DLightIndex += 1u;
					}
					DLightParams.UploadBuffer();
				}
				else
				{
					BOOL32 NeedUpload = FALSE; UINT32 DLightIndex = 0u;
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
						if (LightProxy->IsNeedUpdateParams())
						{
							RDirectionalLightSceneProxy::RPerViewDomainInfoType& ViewDomainInfos = LightProxy->GetViewDomainInfos();
							const TArray<EViewDomainInfo>& DomainInfos = ViewDomainInfos[InViewProxy->GetUniqueID()];
							PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check light view domain invalid."), (ViewDomainInfos.Num() > 0));
							const EViewDomainInfo& DomainInfo = DomainInfos[0];

							DLightParams.UpdateParameterValue(DLightIndex, LightProxy->GetWorldRotation(), DomainInfo, LightProxy->GetLightData());

							NeedUpload = TRUE;
						}
						DLightIndex += 1u;
					}
					if (NeedUpload)
					{
						DLightParams.UploadBuffer();
					}
				}
			}
			else
			{
				DLightParams.ClearBuffer();
				DLightParams.ClearParameter();
			}
		}

		return UsedDLightNum;
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
				for (INT32 DomainIndex = 0, DomainNum = DomainInfos.Num(); DomainIndex < DomainNum; DomainIndex++)
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
		const INT32							OctreeLayerNum		= OctreeLayerInfos.Num();

		INT32 TempDeep = 0;
		SceneOctree.BackwardRecursionNode(0, TempDeep,
			[&InOutVisibilityMap, &OctreeElements](ROctreeNode& InNode, INT32 InDeep)->INT32
			{
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check element index of octree node is valid failed."), ((InNode.ElementIndex >= 0) && (InNode.ElementIndex < (OctreeElements.Num()))));
				const ROctreeElement& OctreeElement = OctreeElements[InNode.ElementIndex];
				for (INT32 PrimitiveIndex = 0, PrimitiveNum = OctreeElement.Primitives.Num(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
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
				const FLOAT AxisExtent[3] = { NodeExtent.x, NodeExtent.y, NodeExtent.z };
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