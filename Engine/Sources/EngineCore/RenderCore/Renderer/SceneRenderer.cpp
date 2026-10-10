#include "SceneRenderer.h"
#include "ScenePass.h"
#include <RenderCommon.h>
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <RenderProxy/ViewProxy.h>
#include <Development/GPUProfiler.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <RenderProxy/BezierGrassSceneProxy.h>
#if _EDITOR_ONLY
#include <EngineCore/Editor/EditorLogManager.h>
#include <cstdio>
#include <RenderDevice/D3D12Native.h>
#include <imGUI/Headers/imGUIManager.h>
#endif	// _EDITOR_ONLY

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSceneRenderer, EManagerBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	/*
	* The engine shader programs the fixed-function passes run with. Each pass states its own
	* source - program, material (none here), vertex axis and the stages it needs - because a
	* shader is never resolved from a name; the process wide instance cache owns the resulting
	* instances, so the renderer keeps only the resolved programs.
	*/
	static const EShaderInstance* GetFullScreenProgramInstance()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("M_EngineFullScreenTriangle"));
		Request.ProgramName = EName(PE_TEXT("Copy"));
		Request.VertexFactory = EShaderVertexFactoryType::FULLSCREEN;
		Request.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_UV0;
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;

		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(Request);
	}
	static const EShaderInstance* GetSceneLightingProgramInstance()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("M_EngineSceneLighting"));
		Request.ProgramName = EName(PE_TEXT("SceneLighting"));
		// The lighting pass decodes every domain at runtime, but the shading model is
		// injected at compile time, so one domain is compiled into the pass; DefaultLit
		// is that one (design Section12).
		Request.DomainName = EString(PE_TEXT("DefaultLit"));
		Request.VertexFactory = EShaderVertexFactoryType::FULLSCREEN;
		Request.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_UV0;
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;

		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(Request);
	}
	/*
	* The environment SH bake. A compute program, so it declares no vertex axis and no domain -
	* the request's axis still has to be some value because the cache keys a variant on it, and
	* since the program compiles none of the VF machinery the value only labels the variant.
	*/
	static const EShaderInstance* GetEnvironmentSHProgramInstance()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("CS_EnvironmentSH"));
		Request.ProgramName = EName(PE_TEXT("EnvironmentSH"));
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_COMPUTE;

		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(Request);
	}

	/*
* A colour target a pass can actually write into: a texture the back-end created (the wrapper
* falls back to itself when it holds no back-end texture, which is an object the command list
* cannot resolve into a view) that carries the render target flag the pass's pipeline state
* declares. Anything else would leave the pass's slot unbound and its draws landing nowhere,
* so a pass checks its targets and says so instead of recording into nothing.
*/
static BOOL32 IsUsableColorTarget(const RRenderTexture2D& InTexture)
{
	if (!(InTexture.IsRenderResourceValid()))
	{
		return FALSE;
	}
	return (((InTexture.GetBindFlags() & static_cast<UINT8>(RBindFlagType::BIND_RENDER_TARGET)) != 0u) ? TRUE : FALSE);
}

	void RSceneRenderer::RLightLists::EnsureBuffers(IRRHIDevice* InRenderDevice)
	{
		if (!InRenderDevice)
		{
			return;
		}
		if (DirectionalLightBuffer.IsRenderResourceValid() == FALSE)
		{
			const UINT32 ElementSize = static_cast<UINT32>(sizeof(EDirectionalLightsGPU));
			DirectionalLightBuffer.InitFromRHI(InRenderDevice
				, RBufferDesc(static_cast<UINT64>(ElementSize) * RCommonSettings::RENDER_DIRECTIONAL_LIGHT_NUM_MAX
					, RBindFlagType::BIND_SHADER_RESOURCE
					, ElementSize
					, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));
			DirectionalLightStaging.Reserve(RCommonSettings::RENDER_DIRECTIONAL_LIGHT_NUM_MAX);
		}
		if (LocalLightBuffer.IsRenderResourceValid() == FALSE)
		{
			const UINT32 ElementSize = static_cast<UINT32>(sizeof(ELocalLightsGPU));
			LocalLightBuffer.InitFromRHI(InRenderDevice
				, RBufferDesc(static_cast<UINT64>(ElementSize) * RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX
					, RBindFlagType::BIND_SHADER_RESOURCE
					, ElementSize
					, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));
			LocalLightStaging.Reserve(RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX);
		}
	}
	void RSceneRenderer::RLightLists::Upload(IRRHIDevice* InRenderDevice)
	{
		if (!InRenderDevice)
		{
			return;
		}
		if ((DirectionalLightCount > 0u) && (DirectionalLightStaging.Num() >= static_cast<INT32>(DirectionalLightCount)))
		{
			DirectionalLightBuffer.UploadDataViaRHI(InRenderDevice, DirectionalLightStaging.GetData()
				, static_cast<UINT32>(sizeof(EDirectionalLightsGPU)) * DirectionalLightCount, 0u);
		}
		if ((LocalLightCount > 0u) && (LocalLightStaging.Num() >= static_cast<INT32>(LocalLightCount)))
		{
			LocalLightBuffer.UploadDataViaRHI(InRenderDevice, LocalLightStaging.GetData()
				, static_cast<UINT32>(sizeof(ELocalLightsGPU)) * LocalLightCount, 0u);
		}
	}

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr), CurrentViewProxy(nullptr), FullScreenProgram(nullptr), SceneLightingProgram(nullptr)
		, FinalOutputView(0u), NeedStencil(FALSE)
		, EnvironmentSHPipelineState(nullptr), EnvironmentSHProgram(nullptr), EnvironmentSHBakedSource(nullptr)
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
			if (bManageDebugWireframe && DebugWireframePrimitiveManager)
			{
				DebugWireframePrimitiveManager->Initialize();
			}
		}
#endif
		// The renderer wide sampler table. It is built in the register order the shaders
		// declare, so the binding loop below can use the array index as the register:
		// s0..s3 as Components/C_EngineSystem.json states them, s4..s6 as the generated
		// material sampler group does. A table in another order would have a shader sample
		// through the wrong state - a clamp where it declared wrap, a point filter where it
		// declared linear - which is invisible until the sampling artefact shows.
		{
			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

			struct RSamplerInitEntry
			{
				RSamplerType  Type;
				RSamplerState State;
			};
			const FLOAT SystemMaxLOD	= 1024.f;
			const FLOAT MaterialMaxLOD	= 16.f;
			const RSamplerInitEntry SamplerConfigs[] =
			{
				{ RSamplerType::SAMPLER_TYPE_LINEAR_WRAP,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						SystemMaxLOD) },
				{ RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						SystemMaxLOD) },
				{ RSamplerType::SAMPLER_TYPE_POINT_CLAMP,
					RSamplerState(RFilterType::FILTER_POINT,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						RTextureAddressModeType::TEXTURE_ADDRESS_CLAMP,
						SystemMaxLOD) },
				// Border white and a comparison that passes outside the map: a shadow lookup
				// outside the cascade reads "fully lit". The filter stays linear and the
				// comparison function stays NEVER because the engine's filter enum has no
				// comparison variant - the backend would only drop the comparison silently -
				// so the depth test belongs to the sampling code until the RHI grows one.
				{ RSamplerType::SAMPLER_TYPE_SHADOW,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_BORDER,
						RTextureAddressModeType::TEXTURE_ADDRESS_BORDER,
						RTextureAddressModeType::TEXTURE_ADDRESS_BORDER,
						SystemMaxLOD, 0.f, 1u, 0.f,
						Color4(1.f, 1.f, 1.f, 1.f),
						RComparisonFunctionType::COMPARISON_NEVER) },
				{ RSamplerType::SAMPLER_TYPE_MATERIAL_2D,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						MaterialMaxLOD) },
				{ RSamplerType::SAMPLER_TYPE_MATERIAL_CUBE,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						MaterialMaxLOD) },
				{ RSamplerType::SAMPLER_TYPE_MATERIAL_3D,
					RSamplerState(RFilterType::FILTER_LINEAR,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						RTextureAddressModeType::TEXTURE_ADDRESS_WRAP,
						MaterialMaxLOD) },
			};
			constexpr UINT32 SamplerConfigCount = PE_ARRAYSIZE(SamplerConfigs);
			for (UINT32 i = 0u; i < SamplerConfigCount; i++)
			{
				// The table's position is the register, so a config out of that order is a
				// source error rather than something to sort out at bind time.
				PE_CHECK((ENGINE_RENDER_CORE_ERROR),
					(PE_TEXT("RenderGraph: the sampler table has to be listed in register order.")),
					(static_cast<UINT32>(SamplerConfigs[i].Type) == i));

				IRRHISampler* OutSampler = nullptr;
				if (!RenderDevice->CreateSampler(SamplerConfigs[i].State, &OutSampler) || !OutSampler)
				{
					continue;
				}
				if (RenderDevice->GetBackendType() == ERHIBackendType::RHI_BACKEND_D3D11)
				{
					// D3D11 returns RSamplerResource*; lift the SamplerState into
					// the by-value array slot and discard the heap wrapper.
					RSamplerResource* D3D11Sampler = static_cast<RSamplerResource*>(OutSampler);
					Samplers[SamplerConfigs[i].Type].SamplerState = D3D11Sampler->SamplerState;
					delete D3D11Sampler;
				}
				else
				{
					// D3D12 (etc.): keep the abstract handle in the slot.
					Samplers[SamplerConfigs[i].Type].RHISampler = OutSampler;
				}
			}
		}

		// The pipeline state cache owns the root signature a program's reflection generates,
		// so every proxy has to ask it rather than create a pipeline state of its own.
		{
			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
			PipelineStateCache.SetDevice(RenderDevice);
		}

		// Phase 4.3.0.4: full-screen + scene-lighting shaders now come from
		// the engine shader assets M_EngineFullScreenTriangle (VS+PS for the
		// blit) and M_EngineSceneLighting (PS for deferred lighting; reuses
		// the FullScreenTriangle VS). Both are loaded once per process.
		{
			// The instances are kept, not only the programs: they are the stable handles a shader
			// reload reaches by re-pointing, and re-reading a program from one is a pure memory
			// read - which is what makes the refresh in OnShaderAssetReplaced legal on this side.
			FullScreenProgramInstance = GetFullScreenProgramInstance();
			FullScreenProgram = (FullScreenProgramInstance ? FullScreenProgramInstance->GetProgram() : nullptr);

			SceneLightingProgramInstance = GetSceneLightingProgramInstance();
			SceneLightingProgram = (SceneLightingProgramInstance ? SceneLightingProgramInstance->GetProgram() : nullptr);

			EnvironmentSHProgramInstance = GetEnvironmentSHProgramInstance();
			EnvironmentSHProgram = (EnvironmentSHProgramInstance ? EnvironmentSHProgramInstance->GetProgram() : nullptr);
		}

		// Build the engine driven pass states from the pass table. Each pass states its own
		// rasterizer / blend / depth / render targets in one place, so a pass and the state it
		// runs with cannot drift apart.
		//
		// The state comes from the pipeline state cache, not from the device directly: that is
		// what pairs the state with the root signature the pass program's reflection generated,
		// so a pass never draws against a signature its shaders were not compiled for.
		//
		// A per primitive pass is deliberately not built here. Its shaders belong to the object
		// that draws, so the object builds its own state from this pass and its own program -
		// which is also where that state gets its signature from.
		EnsureEnginePassPipelineStates();

		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Check render scene is not normally released."), (!Scene));
		Scene = new RScene();
	}
	void RSceneRenderer::OnShaderAssetReplaced(const EName& InShaderName)
	{
		/*
		* Everything this renderer derived from the shader that was just replaced is stale, and this
		* is the thread that owns all of it - so this is where it is dropped. Nothing is rebuilt
		* here: each piece is re-derived from the current asset by its next use, which is what makes
		* dropping enough.
		*
		* Order matters only for the pipeline states: they are dropped from the cache first, before
		* the pointers to them are cleared, so no state is left owned by nobody.
		*/
		PipelineStateCache.InvalidateForShaderName(InShaderName);

		// The programs the engine driven passes run. Their instances were re-pointed to the new
		// object before this command ran, so re-reading the programs is what makes them current -
		// and re-reading is all this may do, because a re-resolution that missed the cache would
		// load, and this runs on the render thread.
		FullScreenProgram = (FullScreenProgramInstance ? FullScreenProgramInstance->GetProgram() : nullptr);
		SceneLightingProgram = (SceneLightingProgramInstance ? SceneLightingProgramInstance->GetProgram() : nullptr);
		EnvironmentSHProgram = (EnvironmentSHProgramInstance ? EnvironmentSHProgramInstance->GetProgram() : nullptr);

		// The pass states latch on those programs, and the environment bake latches its own: all of
		// them are rebuilt from the programs above on the next frame that needs them.
		for (UINT32 StateIndex = 0u; StateIndex < RPipelineStateType::PIPELINE_STATE_COUNT; StateIndex++)
		{
			PipelineStates[StateIndex] = nullptr;
		}
		bEnginePassStatesReady = FALSE;
		EnvironmentSHPipelineState = nullptr;

		/*
		* The two blocks of the renderer's own. They latch the program their layout was read from,
		* and a block does not remember which shader that was - so they are released whenever any
		* shader is replaced. A block with no layout binds nothing and re-initializes from the
		* program it is next used with, so the cost of being wrong here is one rebuild and never a
		* wrong layout.
		*/
		GlobalParameterBlock.ReleaseRenderResources();
		IndirectLightingParameterBlock.ReleaseRenderResources();

		/*
		* The modules below cache a program of their own. Each one re-derives it from the instance
		* it holds - the instance is the stable handle - and drops the states it built from the old
		* program, exactly the way this function does for its own.
		*/
		RSkyLightSceneProxy::OnShaderAssetReplaced();
#if _EDITOR_ONLY
		if (DebugWireframePrimitiveManager)
		{
			DebugWireframePrimitiveManager->OnShaderAssetReplaced();
		}
		CImGUIManager::GetManagerSingleton()->OnShaderAssetReplaced();
#endif
	}
	void RSceneRenderer::EnsureEnginePassPipelineStates()
	{
		// Retry-able on purpose. A pass program only has device objects once the render thread has
		// executed the creation commands its asset load queued, and this is first called from
		// Initialize, which runs before that thread is started. A state that cannot be built yet
		// therefore has to stay missing rather than be latched as missing forever: the lighting
		// pass and the final blit both refuse to record without their state, so a one shot build
		// here costs the whole session its image.
		if (bEnginePassStatesReady != FALSE)
		{
			return;
		}

		// The engine driven passes carry their own program, so the programs they run are
		// looked up by the name the pass table states.
		TMap<EName, const EShaderProgram*> EnginePassPrograms;
		if (SceneLightingProgram)
		{
			EnginePassPrograms.Add(EName(PE_TEXT("SceneLighting")), SceneLightingProgram);
		}
		if (FullScreenProgram)
		{
			EnginePassPrograms.Add(EName(PE_TEXT("Copy")), FullScreenProgram);
		}

		BOOL32 bAllReady = TRUE;
		for (UINT32 PassIndex = 0u, PassNum = FRenderPassRegistry::GetPassNum(); PassIndex < PassNum; PassIndex++)
		{
			const FRenderPassDesc& PassDesc = FRenderPassRegistry::GetDesc(static_cast<ERenderPassType>(PassIndex));
			if (PassDesc.PipelineStateSlot >= static_cast<UINT32>(RPipelineStateType::PIPELINE_STATE_COUNT))
			{
				continue;
			}
			if (PipelineStates[PassDesc.PipelineStateSlot])
			{
				continue;
			}
			if (PassDesc.bIsFullScreen == FALSE)
			{
				continue;
			}

			const EShaderProgram* PassProgram = nullptr;
			if ((PassDesc.ProgramName == nullptr)
				|| (!EnginePassPrograms.FindValue(EName(PassDesc.ProgramName), PassProgram))
				|| (PassProgram == nullptr))
			{
				bAllReady = FALSE;
				continue;
			}

			RGraphicsPipelineStateDesc Desc;
			FRenderPassRegistry::FillPipelineStateDesc(PassDesc, PassProgram, Desc);
			PipelineStates[PassDesc.PipelineStateSlot] = PipelineStateCache.GetOrCreateGraphics(Desc);
			if (!(PipelineStates[PassDesc.PipelineStateSlot]))
			{
				bAllReady = FALSE;
			}
		}
		bEnginePassStatesReady = bAllReady;
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
			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
			// The cached states carry root signatures the device owns, so they go before the
			// device does.
			if (RenderDevice)
			{
				PipelineStateCache.InvalidateAll();
			}
			// The pass states came from that cache, which owns them: the array only drops its
			// pointers so they are not released twice.
			for (UINT32 i = 0u; i < RPipelineStateType::PIPELINE_STATE_COUNT; i++)
			{
				PipelineStates[i] = nullptr;
			}

			// Release per-engine samplers. On D3D12 these are RD3D12Sampler
			// objects held by Samplers[i].RHISampler; D3D11 stores the state
			// directly in Samplers[i].SamplerState which the destructor handles.
			for (UINT32 i = 0u; i < RSamplerType::SAMPLER_TYPE_COUNT; i++)
			{
				if (Samplers[i].RHISampler)
				{
					RenderDevice->DestroyResource(Samplers[i].RHISampler);
					Samplers[i].RHISampler = nullptr;
				}
				Samplers[i].SamplerState = nullptr;
			}
		}

		{
			delete Scene;
			Scene = nullptr;
		}

#if _EDITOR_ONLY
		{
			if (bManageDebugWireframe && DebugWireframePrimitiveManager)
			{
				DebugWireframePrimitiveManager->ShutDown();
			}
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
		if (bRenderDebugWireframe && DebugWireframePrimitiveManager)
		{
			DebugWireframePrimitiveManager->InitNewFrame();
		}
#endif
		InitRendererSettings();

		InitViews();

		// The light lists are gathered after the views and before the frame's parameters are
		// published, because the counts the gather produces are part of what UpdateGlobalParameters
		// writes. They do not depend on any view: a light list is a global fact, and the per view
		// shadow domains were already culled inside InitViews.
		InitLights();
	}
	void RSceneRenderer::Render()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		CurrentCommandList = RenderDevice->AcquireCommandList();
		// Publish to MainManager so scene proxies and other render-thread
		// callers can reach the active list without taking it as an arg.
		EMainManager::GetManagerSingleton()->SetActiveCommandList(CurrentCommandList);

		// Open the GPU profile frame: lazy-init the timestamp heap on first
		// call, resolve the slice this frame's slot is about to overwrite,
		// reset slot tracking. Auto-instrumented BeginRenderPass / EndRenderPass
		// brackets land into this frame's slice.
		PE_GPU_PROFILER_BEGIN_FRAME(CurrentCommandList);

		// The engine driven pass states are retried here, once per frame, because the first
		// attempt (renderer Initialize) runs before the shader device objects exist.
		EnsureEnginePassPipelineStates();

		// Bind the renderer wide sampler table across stages, one entry per register: the
		// shaders declare their samplers by these registers (s0..s6, see RSamplerType), and the
		// table is written once per frame because no draw changes it. A sampler a shader
		// declares that this table does not cover is a sampler nobody binds.
		for (UINT32 SamplerIndex = 0u; SamplerIndex < RSamplerType::SAMPLER_TYPE_COUNT; SamplerIndex++)
		{
			// Samplers[] are RSamplerResource (D3D11 wrapper) with a dual-mode
			// RHISampler field. In D3D12 mode the actual back-end sampler is
			// in RHISampler; the D3D11 ComPtr is null. Pick the right one so
			// CommandList's static_cast<IRD3D12Sampler*> lands on a real
			// RD3D12Sampler instance.
			IRRHISampler* SamplerPtr = Samplers[SamplerIndex].RHISampler
				? Samplers[SamplerIndex].RHISampler
				: static_cast<IRRHISampler*>(&Samplers[SamplerIndex]);
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_VERTEX, SamplerIndex, SamplerPtr);
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, SamplerIndex, SamplerPtr);
			CurrentCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, SamplerIndex, SamplerPtr);
		}

		// The environment bake, before anything reads it. Its dispatch is conditional on the sky's
		// cubemap having changed, so most frames spend one pointer comparison here.
		RenderEnvironmentSH();

		// Per frame values. A no-op until the first draw gives the block a layout, which is
		// also why the first frame that binds it publishes from EnsureGlobalParameterBlock.
		UpdateGlobalParameters();

		BasePass();

		FinalOutputPass();

#if _EDITOR_ONLY
		// Editor UI overlays the framebuffer between FinalOutput and Present.
		// Skipped in non-editor builds (the manager + renderer don't exist).
		ImGuiPass();
#endif

		// Close the GPU profile frame: emit ResolveTimestampHeap (D3D12) or
		// close disjoint (D3D11) before SubmitCommandList so the readback
		// is queued to fill in by the time the frame's fence retires.
		PE_GPU_PROFILER_END_FRAME(CurrentCommandList);

		RenderDevice->SubmitCommandList(CurrentCommandList);
		CurrentCommandList = nullptr;
		EMainManager::GetManagerSingleton()->SetActiveCommandList(nullptr);
	}
	BOOL32 RSceneRenderer::EnsureGlobalParameterBlock(IRRHIDevice* InRenderDevice, const EShaderProgram& InProgram)
	{
		if (GlobalParameterBlock.IsInitialized())
		{
			return TRUE;
		}
		if (!GlobalParameterBlock.EnsureInitialized(InRenderDevice, InProgram))
		{
			return FALSE;
		}
		// The block has just acquired a layout; publishing now keeps the first frame that
		// binds it from reading buffer defaults.
		UpdateGlobalParameters();
		return TRUE;
	}
	void RSceneRenderer::UpdateGlobalParameters()
	{
		if (!GlobalParameterBlock.IsInitialized())
		{
			// The block has no layout until a program declared it, and no value written
			// before that can reach a buffer. Registration happens on the first draw.
			return;
		}

		const EName BlockName(SHADER_BLOCK_GLOBAL_PARAMS);
		EShaderParameterSet& Parameters = GlobalParameterBlock.GetParameters();
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		const EGameTimer* GameTimer = EMainManager::GetManagerSingleton()->GetGameTimer();
		const FLOAT TotalTime = ((GameTimer != nullptr) ? static_cast<FLOAT>(GameTimer->GetClockTime()) : 0.f);
		const FLOAT DeltaTime = ((GameTimer != nullptr) ? static_cast<FLOAT>(GameTimer->GetDeltaTime()) : 0.f);

		// TimeParams carries (time, delta time, smooth delta time, real time), the engine wide
		// convention for the pair. No shader of the library reads it yet; it is published so
		// the first one that does finds a defined value instead of a buffer default.
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_TIME_PARAMS)
			, Vector4(TotalTime, DeltaTime, DeltaTime, TotalTime));
		WriteShaderScalar(Parameters, BlockName, EName(SHADER_FIELD_DELTA_TIME), DeltaTime);

		const UINT32 FrameIndex = RenderDevice ? RenderDevice->GetCurrentFrameIndex() : 0u;
		WriteShaderUIntArray(Parameters, BlockName, EName(SHADER_FIELD_FRAME_INDEX), &FrameIndex, 1u);

		// Reserved: nothing reads the flag word yet, and a neutral exposure is the identity
		// for the tone mapping that will.
		const UINT32 RenderFlags[4] = { 0u, 0u, 0u, 0u };
		WriteShaderUIntArray(Parameters, BlockName, EName(SHADER_FIELD_RENDER_FLAGS), RenderFlags, 4u);

		// Exposure is not here: it belongs to the view and is published by RViewProxy, because
		// two cameras on the same frame can be exposed differently and this block is one for
		// the whole frame.

		// The counts that address the two light lists. They are published here, with the frame,
		// because the lists are frame wide: the lighting pass binds the same buffers whichever
		// view it is recording, so a count is a property of the frame and not of the view.
		// A count and its list are always produced together by InitLights, so a published count
		// never addresses a list that was not uploaded.
		const UINT32 DirectionalLightCount = LightLists.DirectionalLightCount;
		const UINT32 LocalLightCount = LightLists.LocalLightCount;
		WriteShaderUIntArray(Parameters, BlockName, EName(SHADER_FIELD_DIRECTIONAL_LIGHT_COUNT), &DirectionalLightCount, 1u);
		WriteShaderUIntArray(Parameters, BlockName, EName(SHADER_FIELD_LOCAL_LIGHT_COUNT), &LocalLightCount, 1u);

		GlobalParameterBlock.Upload(RenderDevice);
	}
	void RSceneRenderer::BindGlobalParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram) const
	{
		GlobalParameterBlock.Bind(InCommandList, InProgram);
	}
	void RSceneRenderer::BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)
	{
		if (!InCommandList)
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		EnsureGlobalParameterBlock(RenderDevice, InProgram);
		BindGlobalParameters(InCommandList, InProgram);
		if (CurrentViewProxy)
		{
			CurrentViewProxy->EnsureParameterBlock(RenderDevice, InProgram);
			CurrentViewProxy->BindParameters(InCommandList, InProgram);
		}
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
			// Only render the main render view through the deferred pipeline.
			// Secondary views (e.g. editor camera) must not be rendered here -
			// they lack scene geometry passes and would clobber FinalOutputView.
			if (!ViewProxy->IsMainRenderView())
			{
				continue;
			}
			const EViewport& Viewport = ViewProxy->GetRenderViewport();
			const EViewport NewViewport(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.Width, Viewport.Height, Viewport.MinDepth, Viewport.MaxDepth);
			CurrentCommandList->SetViewports(&NewViewport, 1u);
			// D3D12 always runs the scissor test (D3D11 keeps it disabled by
			// default). Without an explicit SetScissorRects all draws would
			// have a zero scissor rectangle and get rejected wholesale.
			const ERect ViewScissor(
				Viewport.TopLeftX,
				Viewport.TopLeftY,
				Viewport.TopLeftX + Viewport.Width,
				Viewport.TopLeftY + Viewport.Height);
			CurrentCommandList->SetScissorRects(&ViewScissor, 1u);

			RSceneTextures* SceneTextures = ViewSceneTextures[ViewProxy->GetUniqueID()];
			// The view block is refreshed here and bound by each draw: a root signature
			// switch voids every root binding, so a view bound once per frame would be lost
			// the moment a material swaps in its own pipeline state.
			ViewProxy->UpdateRenderResource();
			CurrentViewProxy = ViewProxy;

			RenderBasePass(SceneTextures);

			RenderSky(ViewProxy);

			RenderLighting(ViewProxy, SceneTextures);

			RenderForward(ViewProxy, SceneTextures);
		}
		CurrentViewProxy = nullptr;
	}
	void RSceneRenderer::FinalOutputPass()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		// Backbuffer pass: blit SceneColor. The pass does not clear: the blit covers every
		// pixel of the back buffer, and the editor overlay draws over it afterwards, so a
		// clear would only be a full screen write nobody sees. It would also take the slow
		// path - the back buffer is a DXGI resource, and the engine cannot register the
		// optimized clear value a DXGI swap chain picks for it, so clearing it to a value of
		// our own is always a mismatching clear (D3D12 #820). A frame with no main view has no
		// blit either, so the back buffer then keeps the previous frame's contents instead of
		// a defined grey, which is what a frame that draws nothing deserves.
		IRRHITexture* BackBuffer = RenderDevice->GetCurrentBackBuffer();
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 1u;
		Pass.RenderTargets[0].Target		= BackBuffer;
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_DONT_CARE;
		Pass.RenderTargets[0].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target			= nullptr;
		Pass.PassName						= PE_TEXT("FinalOutput");
		CurrentCommandList->BeginRenderPass(Pass);

		// D3D12 has no implicit viewport / scissor from OMSetRenderTargets
		// (D3D11 inherits them from the bound RT). Set both explicitly to
		// the backbuffer extents so the FullScreenTriangle actually covers
		// the framebuffer; without this the draw clip-rejects and the back
		// buffer keeps whatever it held before.
		if (BackBuffer)
		{
			const FLOAT BackBufferWidth  = static_cast<FLOAT>(BackBuffer->GetWidth());
			const FLOAT BackBufferHeight = static_cast<FLOAT>(BackBuffer->GetHeight());
			const EViewport FullViewport(0.f, 0.f, BackBufferWidth, BackBufferHeight, 0.f, 1.f);
			CurrentCommandList->SetViewports(&FullViewport, 1u);
			const ERect FullScissor(0.f, 0.f, BackBufferWidth, BackBufferHeight);
			CurrentCommandList->SetScissorRects(&FullScissor, 1u);
		}

		// The blit belongs to a view: it reads that view's scene colour and it applies that
		// view's exposure, so the view block has to be published and bound here. BasePass
		// clears the current view when it finishes, so the blit cannot count on it still being
		// set - and an unbound ViewParams reads as whatever the last user of that root left
		// behind, which for exposure is a stale gain rather than a neutral one.
		CurrentViewProxy = FindViewProxy(FinalOutputView);
		if (CurrentViewProxy)
		{
			CurrentViewProxy->UpdateRenderResource();
		}

		// The state carries the copy program's signature, so the blocks the shader reads are
		// bound after it, never before.
		IRRHIPipelineState* FinalOutputState = PipelineStates[PIPELINE_STATE_FINAL_OUTPUT];
		if ((FinalOutputState) && (FullScreenProgram))
		{
			CurrentCommandList->SetPipelineState(FinalOutputState);
			BindEngineParameters(CurrentCommandList, *FullScreenProgram);

			FullScreenTriangle.BindPrimitiveBuffers(CurrentCommandList, *FullScreenProgram);
			if (FinalOutputView != 0u)
			{
				// The last pass wrote the scene colour as a render target; the blit samples it,
				// so it has to be readable before the view is bound. The next frame's base pass
				// transitions it back through its own render pass. A source without a back-end
				// texture would make the view bind below a silent no-op, and the copy would
				// sample whatever the slot held before.
				RSceneTextures* FinalSceneTextures = ViewSceneTextures[FinalOutputView];
				if (FinalSceneTextures && (FinalSceneTextures->SceneColor.IsRenderResourceValid() != FALSE))
				{
					IRRHITexture* FinalSource = FinalSceneTextures->SceneColor.GetResolvedRHITexture();
					TransitionToShaderReadable(FinalSource);

					// The source lands on the register the copy program's own reflection put it
					// at, read from the pixel stage's binding plan. A literal slot would agree
					// with that reflection only until the next binding is added to the program.
					const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*FullScreenProgram, EShaderStageType::SHADER_STAGE_PIXEL);
					if (PixelEntry)
					{
						EShaderParameterSet Parameters;
						Parameters.SetTexture(EName(), EName(PE_TEXT("CopySourceTexture")), FinalSource);
						BindShaderParameterResources(CurrentCommandList, Parameters
							, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL);
					}
				}
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("FinalOutput: the scene colour source is not a usable texture."));
				}
			}
			CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);
		}
		// Leaving the view published would make the next frame's first draw bind a view that is
		// no longer the one being recorded.
		CurrentViewProxy = nullptr;

		CurrentCommandList->EndRenderPass();

#if !_EDITOR_ONLY
		// Non-editor build: nothing follows FinalOutput, so flip the
		// backbuffer straight to PRESENT here. Editor build defers this to
		// ImGuiPass which needs the buffer in RENDER_TARGET to overlay UI.
		if (BackBuffer)
		{
			CurrentCommandList->ResourceBarrier(BackBuffer,
				ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET,
				ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);
		}
#endif
	}
#if _EDITOR_ONLY
	void RSceneRenderer::ImGuiPass()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice || !CurrentCommandList) { return; }

		IRRHITexture* BackBuffer = RenderDevice->GetCurrentBackBuffer();
		if (!BackBuffer) { return; }

		// Overlay editor UI onto the backbuffer. LoadOp = LOAD preserves the
		// FinalOutput blit underneath; depth target is null since ImGui PSO
		// disables depth. PassName lets the GPU profiler tag the pass.
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount        = 1u;
		Pass.RenderTargets[0].Target  = BackBuffer;
		Pass.RenderTargets[0].LoadOp  = ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.RenderTargets[0].StoreOp = ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target      = nullptr;
		Pass.PassName                 = PE_TEXT("ImGui");
		CurrentCommandList->BeginRenderPass(Pass);

		// EImGuiRenderer reads ImGui::GetDrawData() (produced earlier on the
		// main thread by PrepareDrawData) and replays it through the active
		// IRCommandList. Failure modes (uninitialized renderer, empty draw
		// list) are silent no-ops.
		CImGUIManager::GetManagerSingleton()->RenderDrawData();

		CurrentCommandList->EndRenderPass();

		// Transition the backbuffer to PRESENT now that the last draw of the
		// frame has landed. Mirrors the non-editor branch in FinalOutputPass.
		CurrentCommandList->ResourceBarrier(BackBuffer,
			ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET,
			ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);
	}
#endif
	void RSceneRenderer::TransitionToShaderReadable(IRRHITexture* InTexture)
	{
		if ((!InTexture) || (!CurrentCommandList))
		{
			return;
		}
		const ERHIResourceState CurrentState = InTexture->GetCurrentState();
		if (CurrentState == ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE)
		{
			return;
		}
		CurrentCommandList->ResourceBarrier(InTexture, CurrentState, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
	}
	void RSceneRenderer::TransitionDepthToShaderReadable(IRRHITexture* InTexture)
	{
		if ((!InTexture) || (!CurrentCommandList))
		{
			return;
		}
		// Sampling a depth buffer is a shader read on top of the depth read permission, so
		// the two states travel together; a depth-only read state would leave the view
		// unreadable by the pixel shader.
		const ERHIResourceState ReadState = static_cast<ERHIResourceState>(
			static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_READ)
			| static_cast<UINT16>(ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE));
		const ERHIResourceState CurrentState = InTexture->GetCurrentState();
		if (CurrentState == ReadState)
		{
			return;
		}
		CurrentCommandList->ResourceBarrier(InTexture, CurrentState, ReadState);
	}
	void RSceneRenderer::RenderBasePass(RSceneTextures* InSceneTextures)
	{
		// The pass writes into seven colour targets, so every one of them has to be a usable
		// render target: one that is not would leave its slot unbound and the pixels the base
		// pass meant to write there would be missing from the frame entirely.
		const RRenderTexture2D* const BasePassTargets[7] =
		{
			&InSceneTextures->SceneColor, &InSceneTextures->GBufferA, &InSceneTextures->GBufferB,
			&InSceneTextures->GBufferC, &InSceneTextures->GBufferD, &InSceneTextures->GBufferE,
			&InSceneTextures->GBufferF,
		};
		for (UINT32 TargetIndex = 0u; TargetIndex < PE_ARRAYSIZE(BasePassTargets); TargetIndex++)
		{
			if (!IsUsableColorTarget(*BasePassTargets[TargetIndex]))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("BasePass: a scene colour target is not a usable render target."));
				return;
			}
		}

		// MRT pass: 7 colour render targets + depth-stencil, one per entry of the GBuffer
		// contract table. The pass clears them through its own LoadOp=CLEAR rather than
		// through a separate clear of each texture.
		// Resolve each texture's actual back-end IRRHITexture pointer
		// (D3D11 path returns `this`; D3D12 path returns the RD3D12Texture
		// stored in RHITexture). Phase 4 collapses both paths once the
		// D3D11 wrapper class is retired from engine-side code.
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 7u;
		Pass.RenderTargets[0].Target		= InSceneTextures->SceneColor.GetResolvedRHITexture();
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[0].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[1].Target		= InSceneTextures->GBufferA.GetResolvedRHITexture();
		Pass.RenderTargets[1].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[1].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[2].Target		= InSceneTextures->GBufferB.GetResolvedRHITexture();
		Pass.RenderTargets[2].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[2].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[3].Target		= InSceneTextures->GBufferC.GetResolvedRHITexture();
		Pass.RenderTargets[3].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[3].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[4].Target		= InSceneTextures->GBufferD.GetResolvedRHITexture();
		Pass.RenderTargets[4].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[4].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[5].Target		= InSceneTextures->GBufferE.GetResolvedRHITexture();
		Pass.RenderTargets[5].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[5].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.RenderTargets[6].Target		= InSceneTextures->GBufferF.GetResolvedRHITexture();
		Pass.RenderTargets[6].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.RenderTargets[6].ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
		Pass.DepthStencil.Target			= InSceneTextures->SceneDepthStencil.GetResolvedRHITexture();
		Pass.DepthStencil.DepthLoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		Pass.DepthStencil.ClearDepth		= 1.f;
		Pass.PassName						= PE_TEXT("BasePass");
		CurrentCommandList->BeginRenderPass(Pass);

		// No pipeline state is bound here: this pass is per primitive, so the object that draws
		// owns the state, builds it from this pass and its own program, and binds it inside its
		// draw. The pass itself only frames the render targets and the loop.

		// Every registered primitive draws itself. The pass decides when, the primitive decides
		// how: its own program, its own geometry, its own bindings. Which kind of primitive it is
		// never enters this loop, so adding one does not touch the renderer.
		{
			const TArray<RPrimitiveSceneProxy*>& Primitives = Scene->GetPrimitives();
			for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = Primitives.Num<UINT32>(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
			{
				RPrimitiveSceneProxy* Primitive = Primitives[PrimitiveIndex];
#if _EDITOR_ONLY
				if (!Primitive)
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a null primitive scene proxy."));
					continue;
				}
				if (!(Primitive->IsRenderValid()))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Exist a render invalid primitive scene proxy."));
					continue;
				}
#endif
				Primitive->BindRenderResource();
				Primitive->Draw();
			}
		}

		// BezierGrass - parked with the rest of the grass render path, which is rebuilt from
		// the grass shaders in a task of its own. The loop stays so the place the pass ran in
		// is still visible, but nothing is recorded for it.
#if 0
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
#if 0
			if (BezierGrass->Property.bWireframe)
			{
				CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_WIREFRAME]);
			}
			else
#endif
			{
				CurrentCommandList->SetPipelineState(PipelineStates[PIPELINE_STATE_BASE_PASS_GRASS_SOLID]);
			}
			BezierGrass->DispatchComputeShader();
			BezierGrass->BindRenderResource();
			BezierGrass->Draw();
		}
#endif
		CurrentCommandList->EndRenderPass();
	}
	void RSceneRenderer::RenderLighting(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		// The pass owns its state, and the state carries the signature the lighting program's
		// reflection generated. Nothing can be bound before it, so the pass is skipped when the
		// state could not be built rather than recorded against a signature it does not own.
		const EShaderProgram* LightingProgram = SceneLightingProgram;
		IRRHIPipelineState* LightingState = PipelineStates[PIPELINE_STATE_LIGHTING];
		if ((!LightingProgram) || (!LightingState))
		{
			return;
		}
		(void)InViewProxy;

		// The pass blends into the scene colour target the base pass filled, so that target has
		// to be usable: a pass recorded against a target it cannot write would add nothing and
		// the frame would come out unlit.
		if (!IsUsableColorTarget(InSceneTextures->SceneColor))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Lighting: the scene colour target is not a usable render target."));
			return;
		}

		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount			= 1u;
		Pass.RenderTargets[0].Target	= InSceneTextures->SceneColor.GetResolvedRHITexture();
		Pass.RenderTargets[0].LoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;	// additive blend onto base pass colour
		Pass.RenderTargets[0].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target		= nullptr;
		Pass.PassName					= PE_TEXT("Lighting");
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(LightingState);

		// The shared blocks belong to the pass that draws it, not to whatever ran before: the
		// state above brought its own signature, which voided anything an earlier pass had left
		// bound. The light counts travel in the per frame block, so they land with the rest of
		// it instead of at a root written down here.
		BindEngineParameters(CurrentCommandList, *LightingProgram);

		// The environment's brightness and tint, which are this renderer's block and not the sky's:
		// they travel with the coefficients the bake produced, and the pass that shades reads them
		// beside those coefficients. Bound here rather than by the renderer's engine parameter call
		// because they belong to this pass alone.
		BindIndirectLightingParameters(CurrentCommandList, *LightingProgram);

		FullScreenTriangle.BindPrimitiveBuffers(CurrentCommandList, *LightingProgram);

		// The GBuffers and the depth buffer were render targets until now, so the pass has to
		// put them into a state a pixel shader may read before it binds them. The next frame's
		// base pass transitions them back through its own LoadOp=CLEAR render pass.
		TransitionToShaderReadable(InSceneTextures->GBufferA.GetResolvedRHITexture());
		TransitionToShaderReadable(InSceneTextures->GBufferB.GetResolvedRHITexture());
		TransitionToShaderReadable(InSceneTextures->GBufferC.GetResolvedRHITexture());
		TransitionToShaderReadable(InSceneTextures->GBufferD.GetResolvedRHITexture());
		TransitionToShaderReadable(InSceneTextures->GBufferE.GetResolvedRHITexture());
		TransitionDepthToShaderReadable(InSceneTextures->SceneDepthStencil.GetResolvedRHITexture());

		// Every texture the lighting program declares lands on the register its own reflection
		// put it at, read from the pixel stage's binding plan: the GBuffer targets this pass
		// reads what the base pass wrote, plus the depth buffer the world position comes back
		// from. A name the plan does not carry is a texture the compiler culled, and binding it
		// would land on another resource's register.
		//
		// The scene colour target is not among them and the program does not declare it: this pass
		// writes that target, and a resource cannot be sampled by the draw that writes it. The base
		// pass's emission reaches the frame through this pass's additive blend instead, so every
		// slot this list fills is a texture the pass actually reads.
		{
			const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*LightingProgram, EShaderStageType::SHADER_STAGE_PIXEL);
			if (PixelEntry)
			{
				EShaderParameterSet Parameters;
				Parameters.SetTexture(EName(), EName(PE_TEXT("DepthTexture")), InSceneTextures->SceneDepthStencil.GetResolvedRHITexture());
				Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferATexture")), InSceneTextures->GBufferA.GetResolvedRHITexture());
				Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferBTexture")), InSceneTextures->GBufferB.GetResolvedRHITexture());
				Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferCTexture")), InSceneTextures->GBufferC.GetResolvedRHITexture());
				Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferDTexture")), InSceneTextures->GBufferD.GetResolvedRHITexture());
				Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferETexture")), InSceneTextures->GBufferE.GetResolvedRHITexture());
				// The two light lists. They are frame wide resources like the per frame block
				// is: every view reads the same buffers, and the counts that bound their loops
				// travel in GlobalParams. A pass binds them through its own plan, which is why
				// nothing else in the frame has to care whether the lighting pass ran.
				Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("DirectionalLights")), &LightLists.DirectionalLightBuffer);
				Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("LocalLights")), &LightLists.LocalLightBuffer);
				// The environment's coefficients, written by the bake this frame and read here as the
				// indirect term's irradiance. A pass that has no sky, or a frame where the bake could
				// not be recorded, leaves the slot to the resource set's null fill - a null view reads
				// as zero coefficients, which is the environment contributing nothing, rather than as
				// whatever a previous pass left there.
				Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("EnvironmentSH")), &EnvironmentSHBuffer);
				BindShaderParameterResources(CurrentCommandList, Parameters
					, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL);
			}
		}

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
		// The dome carries its own pipeline state, built from the sky pass and
		// the engine sky program, and sets it inside its draw.

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
		// Same requirement as the lighting pass: the forward geometry lands on top of the scene
		// colour it reads back, so a target it cannot write would drop the whole pass.
		if (!IsUsableColorTarget(InSceneTextures->SceneColor))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), ("Forward: the scene colour target is not a usable render target."));
			return;
		}

		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount			= 1u;
		Pass.RenderTargets[0].Target	= InSceneTextures->SceneColor.GetResolvedRHITexture();
		Pass.RenderTargets[0].LoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.RenderTargets[0].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target		= InSceneTextures->SceneDepthStencil.GetResolvedRHITexture();
		Pass.DepthStencil.DepthLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.DepthStencil.DepthStoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.StencilLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.DepthStencil.StencilStoreOp= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.PassName					= PE_TEXT("Forward");
		CurrentCommandList->BeginRenderPass(Pass);

		// The debug primitives build and bind their own state, so the pass frames the targets
		// and leaves the pipeline state to them.
#if _EDITOR_ONLY
		{
			if (bRenderDebugWireframe && DebugWireframePrimitiveManager)
			{
				CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST);
				DebugWireframePrimitiveManager->RenderPrimitives_RenderThread(InViewProxy);
				CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			}
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

			// The per view half of the light work: each shadow domain of each light is culled
			// against its own cascade frustum, which was derived from this view's frustum and
			// therefore belongs to it. The light lists themselves are frame wide and are
			// gathered once in InitLights.
			{
				InitDirectionalLightShadowDomains(ViewProxy);
			}

			// Shadow maps
			{
				InitShadowTextures(ViewProxy);
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
	void RSceneRenderer::InitLights()
	{
		if (!Scene)
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		LightLists.EnsureBuffers(RenderDevice);

		GatherDirectionalLights();
		GatherLocalLights();

		// The buffers travel with the counts that address them: a count is only meaningful
		// together with the list it indexes, so the two are produced and published together.
		// The uploads are prefix writes, so a frame with fewer lights uploads fewer bytes
		// rather than leaving stale ones the loop would read.
		LightLists.Upload(RenderDevice);

#if _EDITOR_ONLY
		ValidateLightBufferLayout();
#endif
	}
	void RSceneRenderer::GatherDirectionalLights()
	{
		LightLists.DirectionalLightStaging.Empty();
		LightLists.DirectionalLightCount = 0u;

		RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLightSceneProxies = Scene->GetDirectionalLightSceneProxies();
		const UINT32 SceneLightNum = DirectionalLightSceneProxies.GetSceneProxyCount();
		for (UINT32 DirectionalLightIndex = 0u; DirectionalLightIndex < SceneLightNum; DirectionalLightIndex++)
		{
			if (LightLists.DirectionalLightCount >= RCommonSettings::RENDER_DIRECTIONAL_LIGHT_NUM_MAX)
			{
			// The list is full. Dropping the extra lights is the only option a fixed
			// capacity leaves, and it is reported rather than silent because a light that
			// quietly stops lighting is the kind of bug that gets blamed on the shading.
			PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] The directional light list is full at "))
				+ ToString(RCommonSettings::RENDER_DIRECTIONAL_LIGHT_NUM_MAX) + EString(PE_TEXT(" lights; the rest are dropped.")));
			break;
		}
		RDirectionalLightSceneProxy* LightProxy = DirectionalLightSceneProxies.SceneProxies[DirectionalLightIndex];
#if _EDITOR_ONLY
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that light proxy can not be null")), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			EDirectionalLightsGPU& Element = LightLists.DirectionalLightStaging.AddDefaultGetRef();
			EncodeDirectionalLight(LightProxy->GetLightData(), LightProxy->GetWorldRotation(), Element);
			LightLists.DirectionalLightCount += 1u;
		}
	}
	void RSceneRenderer::GatherLocalLights()
	{
		LightLists.LocalLightStaging.Empty();
		LightLists.LocalLightCount = 0u;

		// Point and spot share this one list, so the capacity is their sum and the type field in
		// each element is what tells them apart. One index range is what a tiled or clustered
		// cull will want to produce later, and it is one bind and one count rather than two.
		RSceneProxyMapping<RLocalLightSceneProxy>& LocalLightSceneProxies = Scene->GetLocalLightSceneProxies();
		const UINT32 SceneLightNum = LocalLightSceneProxies.GetSceneProxyCount();
		for (UINT32 LocalLightIndex = 0u; LocalLightIndex < SceneLightNum; LocalLightIndex++)
		{
			if (LightLists.LocalLightCount >= RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX)
			{
				// The list is full. Dropping the extra lights is the only option a fixed capacity
				// leaves, and it is reported rather than silent because a light that quietly
				// stops lighting is the kind of bug that gets blamed on the shading.
				PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] The local light list is full at "))
					+ ToString(RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX) + EString(PE_TEXT(" lights; the rest are dropped.")));
				break;
			}
			RLocalLightSceneProxy* LightProxy = LocalLightSceneProxies.SceneProxies[LocalLightIndex];
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that local light proxy can not be null")), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			ELocalLightsGPU& Element = LightLists.LocalLightStaging.AddDefaultGetRef();
			EncodeLocalLight(LightProxy->GetLightData(), LightProxy->GetWorldLocation(), LightProxy->GetWorldRotation(), Element);
			LightLists.LocalLightCount += 1u;
		}
	}
	void RSceneRenderer::RenderEnvironmentSH()
	{
		if ((!Scene) || (!CurrentCommandList) || (!EnvironmentSHProgram))
		{
			return;
		}

		// What to project: the sky's environment cube. A frame with no sky has nothing to bake, and
		// the comparison below then does not match, so the next frame that has one bakes.
		const RSkyLightSceneProxy* SkyLight = FindPrimarySkyLight();
		const IRRHITexture* SourceTexture = ((SkyLight != nullptr) ? SkyLight->GetEnvironmentTexture() : nullptr);
		if (!SourceTexture)
		{
			return;
		}
		// The one test that keeps the dispatch off the frame path. A sky light recreated by an
		// edit resolves to a different texture, so a swapped asset and a new sky light are the
		// same case here, and a frame where nothing moved costs one comparison.
		if (SourceTexture == EnvironmentSHBakedSource)
		{
			return;
		}

		if (!EnsureEnvironmentSHPipelineState())
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}
		if (EnvironmentSHBuffer.IsRenderResourceValid() == FALSE)
		{
			const UINT32 ElementSize = static_cast<UINT32>(sizeof(Vector4));
			EnvironmentSHBuffer.InitFromRHI(RenderDevice
				, RBufferDesc(static_cast<UINT64>(ElementSize) * RCommonSettings::RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM
					// Both views: the bake writes it as an unordered access view and the lighting
					// pass reads it as a shader resource view. A structured buffer's two views are
					// built independently by the back-end, so one buffer carries both.
					, static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE | RBindFlagType::BIND_UNORDERED_ACCESS)
					, ElementSize
					, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));

			// Created as zeros rather than left holding whatever the allocation came with. The
			// coefficients are read by the lighting pass in the same frame they are written, so a frame
			// where the dispatch cannot be recorded - a cubemap whose resource is not up yet, a plan
			// without the bindings - would otherwise shade the scene with uninitialised floats, which
			// is light from nowhere at best and a NaN at worst.
			Vector4 ZeroCoefficients[RCommonSettings::RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM];
			for (UINT32 CoefficientIndex = 0u; CoefficientIndex < RCommonSettings::RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM; CoefficientIndex++)
			{
				ZeroCoefficients[CoefficientIndex] = Vector4(0.f, 0.f, 0.f, 0.f);
			}
			EnvironmentSHBuffer.UploadDataViaRHI(RenderDevice, ZeroCoefficients
				, static_cast<UINT32>(sizeof(Vector4)) * RCommonSettings::RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM, 0u);
		}
		IRRHIBuffer* CoefficientBuffer = EnvironmentSHBuffer.GetResolvedRHIBuffer();
		if (!CoefficientBuffer)
		{
			return;
		}
		const EShaderStageEntry* ComputeEntry = ResolveShaderStageEntry(*EnvironmentSHProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
		if (!ComputeEntry)
		{
			return;
		}

		// A dispatch may only write from the unordered access state and the lighting pass may only
		// read from the shader resource state, so a pair of transitions brackets the dispatch. The
		// test before each one is what keeps a transition whose two states already match from
		// being emitted, which the API refuses rather than ignores.
		const ERHIResourceState StateBefore = CoefficientBuffer->GetCurrentState();
		if (StateBefore != ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			CurrentCommandList->ResourceBarrier(CoefficientBuffer, StateBefore, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		CurrentCommandList->SetPipelineState(EnvironmentSHPipelineState);

		// Bound by name through the compute stage's own plan, so neither a register nor a slot is
		// written down here - the same rule the light lists follow on the pixel stage.
		EShaderParameterSet Parameters;
		Parameters.SetTexture(EName(), EName(PE_TEXT("EnvironmentCube")), SourceTexture);
		Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("EnvironmentSHOutput")), &EnvironmentSHBuffer, TRUE);
		BindShaderParameterResources(CurrentCommandList, Parameters, ComputeEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE);

		// One threadgroup: the kernel hands one coefficient to each of nine lanes and lets each
		// scan the whole sample set itself, so there is no reduction, no group shared memory and
		// nothing here to size against the cubemap's resolution.
		CurrentCommandList->Dispatch(1u, 1u, 1u);

		CurrentCommandList->ResourceBarrier(CoefficientBuffer, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);

		EnvironmentSHBakedSource = SourceTexture;
	}
	BOOL32 RSceneRenderer::EnsureEnvironmentSHPipelineState()
	{
		if (EnvironmentSHPipelineState)
		{
			return TRUE;
		}
		if (!EnvironmentSHProgram)
		{
			return FALSE;
		}
		// A compute program carries no fixed function state, so unlike the passes in the pass
		// table this state is the program alone. It still goes through the cache, which is what
		// pairs it with the root signature the program's reflection generated.
		EnvironmentSHPipelineState = PipelineStateCache.GetOrCreateCompute(*EnvironmentSHProgram);
		return ((EnvironmentSHPipelineState != nullptr) ? TRUE : FALSE);
	}
	RSkyLightSceneProxy* RSceneRenderer::FindPrimarySkyLight() noexcept
	{
		if (!Scene)
		{
			return nullptr;
		}
		// The first sky light that has a cube behind it. Order is the scene's list order, which is
		// not a choice this makes: the frame's environment is whichever one the scene offers first,
		// and a scene with two skies is a scene that will have to decide which one it means.
		RSceneProxyMapping<RSkyLightSceneProxy>& SkyLights = Scene->GetSkyLightProxies();
		for (UINT32 SkyLightIndex = 0u, SkyLightNum = SkyLights.GetSceneProxyCount(); SkyLightIndex < SkyLightNum; SkyLightIndex++)
		{
			RSkyLightSceneProxy* SkyLight = SkyLights.SceneProxies[SkyLightIndex];
			if ((SkyLight != nullptr) && (SkyLight->GetEnvironmentTexture() != nullptr))
			{
				return SkyLight;
			}
		}
		return nullptr;
	}
	void RSceneRenderer::BindIndirectLightingParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)
	{
		if (!InCommandList)
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}
		if (!IndirectLightingParameterBlock.EnsureInitialized(RenderDevice, InProgram))
		{
			// The program does not declare the block, so there is nothing to publish into and nothing
			// to bind. A pass that shades declares it; this is a program that does not.
			return;
		}

		const RSkyLightSceneProxy* SkyLight = FindPrimarySkyLight();
		// No sky means no environment, and the block has to carry that rather than the previous
		// frame's values: the pair goes to zero, the indirect term contributes nothing, and a scene
		// without a sky looks like a scene without a sky.
		const Color3 AdjustColor = ((SkyLight != nullptr) ? SkyLight->GetAdjustColor() : Color3::Black());
		const FLOAT AdjustIntensity = ((SkyLight != nullptr) ? SkyLight->GetAdjustIntensity() : 0.f);

		// Written every frame and uploaded only when something moved: the block reports which
		// mirrors are stale, so a still sky costs a comparison rather than a copy.
		const EName BlockName(SHADER_BLOCK_INDIRECT_LIGHTING_PARAMS);
		EShaderParameterSet& Parameters = IndirectLightingParameterBlock.GetParameters();
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_SKY_LIGHT_COLOR_LUMINANCE)
			, Vector4(AdjustColor.r, AdjustColor.g, AdjustColor.b, AdjustIntensity));
		IndirectLightingParameterBlock.Upload(RenderDevice);
		IndirectLightingParameterBlock.Bind(InCommandList, InProgram);
	}
	void RSceneRenderer::ValidateLightBufferLayout()
	{
#if _EDITOR_ONLY
		if (!SceneLightingProgram)
		{
			return;
		}
		const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*SceneLightingProgram, EShaderStageType::SHADER_STAGE_PIXEL);
		if (!PixelEntry)
		{
			return;
		}

		// The reflected size of a structured buffer's element is its field count times the
		// space one field takes, and a field's ByteOffset is zero for a struct member - the
		// compiler only reports a constant buffer layout. So the size is derived from the
		// formats, which is exactly why every member of both light structs is one 16 byte slot:
		// it makes this sum equal to the field count times sixteen and the comparison below a
		// statement about the order of the list rather than about packing rules.
		const EName ResourceNames[] = { EName(PE_TEXT("DirectionalLights")), EName(PE_TEXT("LocalLights")) };
		const UINT32 CppStrides[] = { static_cast<UINT32>(sizeof(EDirectionalLightsGPU)), static_cast<UINT32>(sizeof(ELocalLightsGPU)) };
		const UINT32 GpuStrides[] =
		{
			(LightLists.DirectionalLightBuffer.IsRenderResourceValid() != FALSE) ? LightLists.DirectionalLightBuffer.GetStructureByteStride() : 0u,
			(LightLists.LocalLightBuffer.IsRenderResourceValid() != FALSE) ? LightLists.LocalLightBuffer.GetStructureByteStride() : 0u
		};

		for (UINT32 NameIndex = 0u; NameIndex < PE_ARRAYSIZE(ResourceNames); NameIndex++)
		{
			const EShaderReflectionResource* Resource = PixelEntry->Reflection.FindResource(ResourceNames[NameIndex], FALSE);
			if (!Resource)
			{
				continue;
			}
			UINT32 ShaderStride = 0u;
			for (UINT32 FieldIndex = 0u; FieldIndex < Resource->StructFields.Num<UINT32>(); FieldIndex++)
			{
				const EShaderReflectionField& Field = Resource->StructFields[FieldIndex];
				ShaderStride += (Field.Format.GetElementSlotBytes() * Field.GetElementNum());
			}
			if (ShaderStride != CppStrides[NameIndex])
			{
				PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] '")) + ResourceNames[NameIndex].ToString()
					+ EString(PE_TEXT("': the shader declares an element of ")) + ToString(ShaderStride)
					+ EString(PE_TEXT(" bytes but the c++ mirror is ")) + ToString(CppStrides[NameIndex])
					+ EString(PE_TEXT(" bytes. Components/C_Lighting.json and LightSceneProxy.h have drifted apart.")));
			}
			if ((GpuStrides[NameIndex] != 0u) && (GpuStrides[NameIndex] != CppStrides[NameIndex]))
			{
				PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] '")) + ResourceNames[NameIndex].ToString()
					+ EString(PE_TEXT("': the buffer was created with a stride of ")) + ToString(GpuStrides[NameIndex])
					+ EString(PE_TEXT(" but its element struct is ")) + ToString(CppStrides[NameIndex]) + EString(PE_TEXT(" bytes.")));
			}
		}

		// The environment coefficients are declared by the bake program rather than by the lighting
		// pass, so they are checked against that program's compute stage. What is checkable here is
		// the element stride: the element is a bare float4, so unlike the light structs the
		// reflection cannot state how many of them the shader writes. That count lives as a pair of
		// constants - RENDER_ENVIRONMENT_SH_COEFFICIENT_NUM and SH_COEFFICIENT_COUNT - and the
		// comment on the first one is where the pairing is recorded.
		if (EnvironmentSHProgram)
		{
			const EShaderStageEntry* ComputeEntry = ResolveShaderStageEntry(*EnvironmentSHProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
			const EShaderReflectionResource* Resource = (ComputeEntry ? ComputeEntry->Reflection.FindResource(EName(PE_TEXT("EnvironmentSHOutput")), FALSE) : nullptr);
			if (Resource)
			{
				UINT32 ShaderStride = 0u;
				for (UINT32 FieldIndex = 0u; FieldIndex < Resource->StructFields.Num<UINT32>(); FieldIndex++)
				{
					const EShaderReflectionField& Field = Resource->StructFields[FieldIndex];
					ShaderStride += (Field.Format.GetElementSlotBytes() * Field.GetElementNum());
				}
				const UINT32 CppStride = static_cast<UINT32>(sizeof(Vector4));
				if (ShaderStride != CppStride)
				{
					PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] 'EnvironmentSHOutput': the shader declares an element of "))
						+ ToString(ShaderStride) + EString(PE_TEXT(" bytes but the c++ element is "))
						+ ToString(CppStride) + EString(PE_TEXT(" bytes. Compute/EnvironmentSH/EnvironmentSH.json and the bake's buffer have drifted apart.")));
				}
				const UINT32 GpuStride = ((EnvironmentSHBuffer.IsRenderResourceValid() != FALSE) ? EnvironmentSHBuffer.GetStructureByteStride() : 0u);
				if ((GpuStride != 0u) && (GpuStride != CppStride))
				{
					PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] 'EnvironmentSHOutput': the buffer was created with a stride of "))
						+ ToString(GpuStride) + EString(PE_TEXT(" but its element is ")) + ToString(CppStride) + EString(PE_TEXT(" bytes.")));
				}
			}
		}
#endif	// _EDITOR_ONLY
	}
	void RSceneRenderer::ProcessOcclusionCull(RViewProxy* InViewProxy)
	{

	}
	RViewProxy* RSceneRenderer::FindViewProxy(const ObjectIdentityType InViewIdentity) noexcept
	{
		if (!Scene)
		{
			return nullptr;
		}
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
			if (ViewProxy && (ViewProxy->GetUniqueID() == InViewIdentity))
			{
				return ViewProxy;
			}
		}
		return nullptr;
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
	void RSceneRenderer::InitDirectionalLightShadowDomains(RViewProxy* InViewProxy)
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
