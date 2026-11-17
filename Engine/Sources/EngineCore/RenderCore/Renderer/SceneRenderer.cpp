#include "SceneRenderer.h"
#include "ScenePass.h"
#include "ViewPrimitiveList.h"
#include <RenderCommon.h>
#include <algorithm>
#include <RenderDevice/DeviceD3D11.h>
#include <RHI/IRRHIDevice.h>
#include <Main/MainManager.h>
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <RenderProxy/ViewProxy.h>
#include <RenderProxy/ShadowViewProxy.h>
#include <Renderer/ShadowAtlas.h>
#include <RenderConfig/ShadowConfig.h>
#include <Development/GPUProfiler.h>
#include <RenderProxy/SkyLightProxy.h>
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <RenderProxy/BezierGrassSceneProxy.h>
#include <MultiThread/Thread.h>
// The diagnostics this file reports are written from code that runs in every configuration, and
// the header itself is a no-op under ENGINE_SHIPPING_MODE, which is what keeps this include out of the
// editor-only block below.
#include <EngineCore/Editor/EditorLogManager.h>
#if ENGINE_EDITOR_ONLY
#include <cstdio>
#include <RenderDevice/D3D12Native.h>
#include <imGUI/Headers/imGUIManager.h>
#endif	// ENGINE_EDITOR_ONLY

namespace PigeonEngine
{

	// The passes this file drives own their resource bindings by name, so the claim tables live
	// next to the drawing code. EnvironmentCube overlaps with the sky dome (each publishes its own
	// cube); everything else here has exactly one publisher.
	static const FShaderBindingClaim GEnvironmentSHBindingClaims[] =
	{
		{ EName(PE_TEXT("EnvironmentCube")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("EnvironmentSHOutput")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GEnvironmentSHBindingClaims, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_SH));

	static const FShaderBindingClaim GLightingBindingClaims[] =
	{
		// The depth buffer, the normal target and the light list are also published by the shadow
		// mask pass for its own draw. A name two scopes publish has to be OVERLAPPING in both - a
		// mixed pair is what the plan audit reports as a source defect.
		{ EName(PE_TEXT("DepthTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("GBufferATexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("GBufferBTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("GBufferCTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("GBufferDTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("GBufferETexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("DirectionalLights")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("LocalLights")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		{ EName(PE_TEXT("EnvironmentSH")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		// The specular pair is produced by their own passes and read here; two scopes own each
		// name, so the claim overlaps and neither publisher is accused of the other's hole.
		{ EName(PE_TEXT("PrefilteredEnvironmentCube")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("EnvironmentBRDFLUT")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		// Produced by the shadow mask pass this frame and read here. The resolver writes it as a
		// render target rather than publishing it as a binding, so the lighting pass is its only
		// publisher.
		{ EName(PE_TEXT("DirectionalShadowMask0")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		// Published by AddWaterLightingParameters for the water program: the scene colour copy
		// the refraction term samples. The deferred lighting pass does not declare it.
		{ EName(PE_TEXT("WaterRefractionColor")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
		// The local light shadow term samples the atlas and the slot table from the same list this
		// pass binds, and the mask pass publishes the same two names for its own draw.
		{ EName(PE_TEXT("ShadowMapAtlas")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("ShadowSlots")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GLightingBindingClaims, EName(SHADER_BINDING_SCOPE_LIGHTING));

	static const FShaderBindingClaim GShadowMaskBindingClaims[] =
	{
		// Read from the base pass's targets and the light list the lighting pass also reads; both
		// passes publish them for their own draw, so the claim overlaps.
		{ EName(PE_TEXT("DepthTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("GBufferBTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("DirectionalLights")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		// The atlas and the slot table are published by this pass and by the lighting pass, which
		// samples them for the local light shadow term. Two scopes own each name.
		{ EName(PE_TEXT("ShadowMapAtlas")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		{ EName(PE_TEXT("ShadowSlots")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GShadowMaskBindingClaims, EName(SHADER_BINDING_SCOPE_SHADOW_MASK));

	static const FShaderBindingClaim GEnvironmentPrefilterBindingClaims[] =
	{
		// The raw cube is read by the sky dome, the SH bake and this pass; each publishes its
		// own value for its own draw.
		{ EName(PE_TEXT("EnvironmentCube")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
		// Written by this pass and read by the lighting pass.
		{ EName(PE_TEXT("PrefilteredEnvironmentCube")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GEnvironmentPrefilterBindingClaims, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_PREFILTER));

	static const FShaderBindingClaim GEnvironmentBRDFBindingClaims[] =
	{
		// Generated here and sampled by the lighting pass.
		{ EName(PE_TEXT("EnvironmentBRDFLUT")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_OVERLAPPING },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GEnvironmentBRDFBindingClaims, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_BRDF));

	static const FShaderBindingClaim GCopyBindingClaims[] =
	{
		{ EName(PE_TEXT("CopySourceTexture")), EShaderBindingClaimPolicyType::SHADER_BINDING_CLAIM_ONLY_ONE },
	};
	PE_REGISTER_SHADER_BINDING_CLAIMS(GCopyBindingClaims, EName(SHADER_BINDING_SCOPE_COPY));

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
	/*
	* The water refraction background copy. Same full-screen vertex shader as the display copy,
	* with a name and a program of its own; the pixel shader copies the linear scene colour
	* without applying the display transform, because the water material samples the result in
	* the same frame.
	*/
	static const EShaderInstance* GetWaterRefractionCopyProgramInstance()
	{
		EShaderAssemblyRequest Request;
		// A name of its own: the assembly cache keys on it, and a second program under the
		// display copy's name would resolve to that assembly and its stage sources.
		Request.ShaderName = EName(PE_TEXT("M_EngineWaterRefractionCopy"));
		Request.ProgramName = EName(PE_TEXT("WaterRefractionCopy"));
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
	* The directional shadow resolver. It declares no domain and no material: the pass reads the
	* GBuffer for a world normal and the light list for the cascade slots, and every shading
	* decision it makes is the shadow test itself, so nothing of the material path is compiled
	* into it.
	*/
	static const EShaderInstance* GetShadowMaskProgramInstance()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("M_EngineShadowMask"));
		Request.ProgramName = EName(PE_TEXT("ShadowMask"));
		Request.VertexFactory = EShaderVertexFactoryType::FULLSCREEN;
		Request.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_UV0;
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;

		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(Request);
	}
	/*
	* The environment SH bake. A compute program, so it declares no vertex axis and no domain -
	* the request's axis still has to be some value because the cache keys a variant on it, and
	* since the program compiles none of the VF machinery the value only labels the variant.
	*
	* The request is built by its own function because two callers name it: the start up path that
	* resolves it synchronously and the per frame retry that may not load. Both must ask for the
	* same assembly, and one shared request is what keeps them from drifting apart.
	*/
	static EShaderAssemblyRequest MakeEnvironmentSHRequest()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("CS_EnvironmentSH"));
		Request.ProgramName = EName(PE_TEXT("EnvironmentSH"));
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_COMPUTE;
		return Request;
	}
	static const EShaderInstance* GetEnvironmentSHProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(MakeEnvironmentSHRequest());
	}
	// Any thread: a cache hit or nothing, with a request recorded for the load thread. This is the
	// retry a bake runs when its program was not assembled at start up.
	static const EShaderInstance* RequireEnvironmentSHProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(MakeEnvironmentSHRequest());
	}
	/*
	* The specular IBL pair. Both are compute programs; the prefilter takes the raw cube and a
	* per-mip output view, the BRDF pass takes no environment and writes the DFG table.
	*/
	static EShaderAssemblyRequest MakeEnvironmentPrefilterRequest()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("CS_EnvironmentPrefilter"));
		Request.ProgramName = EName(PE_TEXT("EnvironmentPrefilter"));
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_COMPUTE;
		return Request;
	}
	static const EShaderInstance* GetEnvironmentPrefilterProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(MakeEnvironmentPrefilterRequest());
	}
	static const EShaderInstance* RequireEnvironmentPrefilterProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(MakeEnvironmentPrefilterRequest());
	}
	static EShaderAssemblyRequest MakeEnvironmentBRDFRequest()
	{
		EShaderAssemblyRequest Request;
		Request.ShaderName = EName(PE_TEXT("CS_EnvironmentBRDF"));
		Request.ProgramName = EName(PE_TEXT("EnvironmentBRDF"));
		Request.StageMask = ENGINE_SHADER_STAGE_MASK_COMPUTE;
		return Request;
	}
	static const EShaderInstance* GetEnvironmentBRDFProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(MakeEnvironmentBRDFRequest());
	}
	static const EShaderInstance* RequireEnvironmentBRDFProgramInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(MakeEnvironmentBRDFRequest());
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

	void RSceneRenderer::RViewLightBuffers::EnsureBuffers(IRRHIDevice* InRenderDevice)
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
		}
		if (LocalLightBuffer.IsRenderResourceValid() == FALSE)
		{
			const UINT32 ElementSize = static_cast<UINT32>(sizeof(ELocalLightsGPU));
			LocalLightBuffer.InitFromRHI(InRenderDevice
				, RBufferDesc(static_cast<UINT64>(ElementSize) * RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX
					, RBindFlagType::BIND_SHADER_RESOURCE
					, ElementSize
					, RResourceMiscFlagType::RESOURCE_MISC_BUFFER_STRUCTURED));
		}
	}
	void RSceneRenderer::RLightLists::ReserveStaging()
	{
		DirectionalLightStaging.Reserve(RCommonSettings::RENDER_DIRECTIONAL_LIGHT_NUM_MAX);
		LocalLightStaging.Reserve(RCommonSettings::RENDER_LOCAL_LIGHT_NUM_MAX);
	}
	void RSceneRenderer::RLightLists::UploadForView(IRRHIDevice* InRenderDevice
		, RSceneRenderer::RViewLightBuffers& InOutBuffers
		, const RSceneRenderer::RShadowLightAddressMapType& InShadowAddresses)
	{
		if (!InRenderDevice)
		{
			return;
		}
		// The staging prefix is re-encoded from this view's table, then uploaded. A light the
		// table does not name keeps slot zero, which is the slot table's "casts no shadow"
		// sentinel - that is what makes a view without usable shadow data shade unshadowed
		// rather than sample an atlas it does not own.
		const UINT32 DirectionalNum = EMath::Min(DirectionalLightCount, DirectionalLightStaging.Num<UINT32>());
		for (UINT32 LightIndex = 0u; LightIndex < DirectionalNum; LightIndex++)
		{
			UINT32 ShadowSlot	= 0u;
			UINT32 CascadeCount	= 0u;
			const FShadowLightAddress* ShadowAddress = InShadowAddresses.FindValueAsPtr(DirectionalLightIdentities[LightIndex]);
			if (ShadowAddress)
			{
				ShadowSlot		= ShadowAddress->FirstSlot;
				CascadeCount	= ShadowAddress->ViewNum;
			}
			DirectionalLightStaging[LightIndex].ShadowAddressing = Vector4Int(
				static_cast<INT32>(ShadowSlot), static_cast<INT32>(CascadeCount), 0, 0);
		}

		const UINT32 LocalNum = EMath::Min(LocalLightCount, LocalLightStaging.Num<UINT32>());
		for (UINT32 LightIndex = 0u; LightIndex < LocalNum; LightIndex++)
		{
			UINT32 ShadowSlot = 0u;
			const FShadowLightAddress* ShadowAddress = InShadowAddresses.FindValueAsPtr(LocalLightIdentities[LightIndex]);
			if (ShadowAddress)
			{
				ShadowSlot = ShadowAddress->FirstSlot;
			}
			// Only the shadow slot moves; the type, the cone cosines and the photometric values
			// were encoded once for the frame.
			LocalLightStaging[LightIndex].SpotAndTypeParams.w = static_cast<INT32>(ShadowSlot);
		}

		if ((DirectionalLightCount > 0u) && (DirectionalLightStaging.Num() >= static_cast<INT32>(DirectionalLightCount)))
		{
			InOutBuffers.DirectionalLightBuffer.UploadDataViaRHI(InRenderDevice, DirectionalLightStaging.GetData()
				, static_cast<UINT32>(sizeof(EDirectionalLightsGPU)) * DirectionalLightCount, 0u);
		}
		if ((LocalLightCount > 0u) && (LocalLightStaging.Num() >= static_cast<INT32>(LocalLightCount)))
		{
			InOutBuffers.LocalLightBuffer.UploadDataViaRHI(InRenderDevice, LocalLightStaging.GetData()
				, static_cast<UINT32>(sizeof(ELocalLightsGPU)) * LocalLightCount, 0u);
		}
	}

	RSceneRenderer::RSceneRenderer()
		: Scene(nullptr), CurrentCommandList(nullptr), CurrentViewProxy(nullptr), FullScreenProgram(nullptr)
		, WaterRefractionCopyProgram(nullptr), SceneLightingProgram(nullptr), ShadowMaskProgram(nullptr)
		, FinalOutputView(0u), NeedStencil(FALSE)
		, EnvironmentSHPipelineState(nullptr), EnvironmentSHProgram(nullptr), EnvironmentSHBakedSource(nullptr)
		, bIndirectLightingParamsReported(FALSE)
		, PrefilteredEnvironmentMipCount(0u), PrefilteredEnvironmentResolutionExponent(0u)
		, EnvironmentPrefilterBakedSource(nullptr), EnvironmentPrefilterBakedMean(0.f)
		, EnvironmentBRDFLUTUAVView(nullptr), bEnvironmentBRDFLUTBaked(FALSE)
		, EnvironmentPrefilterPipelineState(nullptr), EnvironmentBRDFPipelineState(nullptr)
		, EnvironmentPrefilterProgram(nullptr), EnvironmentBRDFProgram(nullptr)
		, EnvironmentPrefilterProgramInstance(nullptr), EnvironmentBRDFProgramInstance(nullptr)
	{
		for (UINT32 i = 0u; i < RPipelineStateType::PIPELINE_STATE_COUNT; i++)
		{
			PipelineStates[i] = nullptr;
		}
#if ENGINE_EDITOR_ONLY
		{
			DebugWireframePrimitiveManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		}
#endif
	}
	RSceneRenderer::~RSceneRenderer()
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check render scene is not normally released.")), (!Scene));
	}
	void RSceneRenderer::Initialize()
	{
#if ENGINE_EDITOR_ONLY
		{
			// The debug primitive frame is process wide state owned by the window renderer.
			// A secondary renderer never draws it, so it must not build it either.
			if (bManageDebugWireframe != FALSE)
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

			WaterRefractionCopyProgramInstance = GetWaterRefractionCopyProgramInstance();
			WaterRefractionCopyProgram = (WaterRefractionCopyProgramInstance ? WaterRefractionCopyProgramInstance->GetProgram() : nullptr);

			SceneLightingProgramInstance = GetSceneLightingProgramInstance();
			SceneLightingProgram = (SceneLightingProgramInstance ? SceneLightingProgramInstance->GetProgram() : nullptr);

			ShadowMaskProgramInstance = GetShadowMaskProgramInstance();
			ShadowMaskProgram = (ShadowMaskProgramInstance ? ShadowMaskProgramInstance->GetProgram() : nullptr);

			EnvironmentSHProgramInstance = GetEnvironmentSHProgramInstance();
			EnvironmentSHProgram = (EnvironmentSHProgramInstance ? EnvironmentSHProgramInstance->GetProgram() : nullptr);

			EnvironmentPrefilterProgramInstance = GetEnvironmentPrefilterProgramInstance();
			EnvironmentPrefilterProgram = (EnvironmentPrefilterProgramInstance ? EnvironmentPrefilterProgramInstance->GetProgram() : nullptr);

			EnvironmentBRDFProgramInstance = GetEnvironmentBRDFProgramInstance();
			EnvironmentBRDFProgram = (EnvironmentBRDFProgramInstance ? EnvironmentBRDFProgramInstance->GetProgram() : nullptr);
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

		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check render scene is not normally released.")), (!Scene));
		Scene = new RScene();

		// The item pass objects and the recorder pool. The pool size leaves one core for the
		// game thread and one for the render thread; the device clamps it to what it can
		// actually record, and D3D11 answers one.
		BasePassObject = EMemory::MakeUnique<FBasePass>(this);
		ShadowPassObject = EMemory::MakeUnique<FShadowPass>(this);
		WaterSurfacePassObject = EMemory::MakeUnique<FWaterSurfacePass>(this);
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (RenderDevice)
		{
			UINT32 RecorderNum = EThread::GetHardwareThreadNum();
			RecorderNum = ((RecorderNum > 2u) ? (RecorderNum - 1u) : 1u);
			if (RecorderNum > RHI_MAX_RECORD_THREAD_NUM)
			{
				RecorderNum = RHI_MAX_RECORD_THREAD_NUM;
			}
			if (RecorderNum > RenderDevice->GetMaxRecordThreadNum())
			{
				RecorderNum = RenderDevice->GetMaxRecordThreadNum();
			}
			RecordScheduler.Initialize(RenderDevice, RecorderNum);
			FrameData.SetRenderDevice(RenderDevice);
		}
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
		WaterRefractionCopyProgram = (WaterRefractionCopyProgramInstance ? WaterRefractionCopyProgramInstance->GetProgram() : nullptr);
		SceneLightingProgram = (SceneLightingProgramInstance ? SceneLightingProgramInstance->GetProgram() : nullptr);
		ShadowMaskProgram = (ShadowMaskProgramInstance ? ShadowMaskProgramInstance->GetProgram() : nullptr);
		EnvironmentSHProgram = (EnvironmentSHProgramInstance ? EnvironmentSHProgramInstance->GetProgram() : nullptr);
		EnvironmentPrefilterProgram = (EnvironmentPrefilterProgramInstance ? EnvironmentPrefilterProgramInstance->GetProgram() : nullptr);
		EnvironmentBRDFProgram = (EnvironmentBRDFProgramInstance ? EnvironmentBRDFProgramInstance->GetProgram() : nullptr);

		// The pass states latch on those programs, and the environment bake latches its own: all of
		// them are rebuilt from the programs above on the next frame that needs them.
		for (UINT32 StateIndex = 0u; StateIndex < RPipelineStateType::PIPELINE_STATE_COUNT; StateIndex++)
		{
			PipelineStates[StateIndex] = nullptr;
		}
		bEnginePassStatesReady = FALSE;
		EnvironmentSHPipelineState = nullptr;
		EnvironmentPrefilterPipelineState = nullptr;
		EnvironmentBRDFPipelineState = nullptr;

		// The bakes remember what produced them, and a replaced environment program is a new
		// producer: keeping the latch would leave a cube or table written by the previous
		// program in place for the rest of the session. A material reload does not move these
		// programs, so the latch is only dropped for the three names that own the bakes.
		const BOOL32 bEnvironmentBakeReplaced =
			((InShaderName == EName(PE_TEXT("CS_EnvironmentSH")))
			|| (InShaderName == EName(PE_TEXT("CS_EnvironmentPrefilter")))
			|| (InShaderName == EName(PE_TEXT("CS_EnvironmentBRDF")))) ? TRUE : FALSE;
		if (bEnvironmentBakeReplaced != FALSE)
		{
			EnvironmentSHBakedSource = nullptr;
			EnvironmentPrefilterBakedSource = nullptr;
			EnvironmentPrefilterBakedMean = 0.f;
			bEnvironmentBRDFLUTBaked = FALSE;
		}

		/*
		* The blocks of the renderer's own. They latch the program their layout was read from,
		* and a block does not remember which shader that was - so they are released whenever any
		* shader is replaced. A block with no layout binds nothing and re-initializes from the
		* program it is next used with, so the cost of being wrong here is one rebuild and never a
		* wrong layout.
		*/
		GlobalParameterBlock.ReleaseRenderResources();
		IndirectLightingParameterBlock.ReleaseRenderResources();
		ShadowMaskParameterBlock.ReleaseRenderResources();
		EnvironmentPrefilterParameterBlock.ReleaseRenderResources();
		EnvironmentBRDFParameterBlock.ReleaseRenderResources();

		/*
		* The modules below cache a program of their own. Each one re-derives it from the instance
		* it holds - the instance is the stable handle - and drops the states it built from the old
		* program, exactly the way this function does for its own.
		*/
		RSkyLightSceneProxy::OnShaderAssetReplaced();
#if ENGINE_EDITOR_ONLY
		if ((bManageDebugWireframe != FALSE) && (DebugWireframePrimitiveManager != nullptr))
		{
			DebugWireframePrimitiveManager->OnShaderAssetReplaced();
		}
		// The overlay belongs to the window renderer: a preview replacing a shader must not
		// reset the main window's overlay state.
		if (IsWindowOutput() != FALSE)
		{
			CImGUIManager::GetManagerSingleton()->OnShaderAssetReplaced();
		}
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
		if (WaterRefractionCopyProgram)
		{
			EnginePassPrograms.Add(EName(PE_TEXT("WaterRefractionCopy")), WaterRefractionCopyProgram);
		}
		if (ShadowMaskProgram)
		{
			EnginePassPrograms.Add(EName(PE_TEXT("ShadowMask")), ShadowMaskProgram);
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

			RGraphicsPipelineStateDesc PipelineStateDesc;
			FRenderPassRegistry::FillPipelineStateDesc(PassDesc, PassProgram, PipelineStateDesc);
			PipelineStates[PassDesc.PipelineStateSlot] = PipelineStateCache.GetOrCreateGraphics(PipelineStateDesc);
			if (!(PipelineStates[PassDesc.PipelineStateSlot]))
			{
				bAllReady = FALSE;
			}
		}
		bEnginePassStatesReady = bAllReady;
	}
	void RSceneRenderer::ShutDown()
	{
		// The recorder pool goes first: a worker that is still parked must not observe the
		// device or the frame data after they are released.
		RecordScheduler.Shutdown();
		BasePassObject.reset();
		ShadowPassObject.reset();
		WaterSurfacePassObject.reset();
		FrameData.EndFrame();
		FrameData.ReleaseAll();

		// The specular IBL resources and their subresource views go before the device does: the
		// views hand their descriptors back and the textures queue through the device's
		// deferred delete path.
		ReleaseSpecularIBLResources();
		// The offscreen output is a device resource like the bakes above, so it goes before
		// the device does. A window renderer never built one, so this is a no-op there.
		OffscreenOutput.ReleaseRenderResource();
		OffscreenOutputSize = Vector2Int::Zero();
		// The cache invalidates the states below; a renderer that comes back up must not keep
		// the pointers from the previous device.
		EnvironmentSHPipelineState = nullptr;
		EnvironmentPrefilterPipelineState = nullptr;
		EnvironmentBRDFPipelineState = nullptr;

		// The shadow atlases and their slot buffers go with the views that own them, and before the
		// device does: the textures are released through it so the destroy lands after the frames
		// that still reference them.
		ReleaseViewShadowStates();

		if (ViewSceneTextures.Num() > 0)
		{
			for (TMap<ObjectIdentityType, RSceneTextures*>::TIterator It = ViewSceneTextures.Begin(); It != ViewSceneTextures.End(); It++)
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
			// The game side has destroyed its world by the time a renderer is shut down, so the
			// Remove* commands those actors queued are still sitting in the scene's write slot:
			// running them is what unregisters (and deletes) the proxies, and the render thread
			// that would have drained them is gone. Draining both slots here is what makes the
			// scene's own teardown check mean "a proxy nobody removed" instead of "shutdown
			// itself left every proxy registered". The main scene happens to be drained earlier,
			// through RScene::Uninit in EMainManager::Uninit; the asset previews have no such
			// call, which is why this belongs to the renderer that owns the scene.
			Scene->ClearAll();
			delete Scene;
			Scene = nullptr;
		}

#if ENGINE_EDITOR_ONLY
		{
			// A secondary renderer does not own the debug primitive frame; shutting it down
			// here would tear down state the main view still uses.
			if (bManageDebugWireframe != FALSE)
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
		for (TMap<ObjectIdentityType, RSceneTextures*>::TIterator It = ViewSceneTextures.Begin(); It != ViewSceneTextures.End(); It++)
		{
			RSceneTextures* SceneTextures = It->second;
			if (SceneTextures)
			{
				SceneTextures->InitResources(NewSize, NeedStencil);
			}
		}
	}
	void RSceneRenderer::SetOutputDescription(const FRendererOutputDescription& InOutputDescription)
	{
		OutputDescription = InOutputDescription;
		if (OutputDescription.Type == RRendererOutputType::RENDERER_OUTPUT_WINDOW)
		{
			// A renderer that goes back to the window owns no image of its own, and the
			// target it built belongs to the configuration that is gone.
			OffscreenOutput.ReleaseRenderResource();
			OffscreenOutputSize = Vector2Int::Zero();
		}
	}
	BOOL32 RSceneRenderer::IsWindowOutput() const
	{
		return ((OutputDescription.Type == RRendererOutputType::RENDERER_OUTPUT_WINDOW) ? TRUE : FALSE);
	}
	IRRHITexture* RSceneRenderer::GetOutputColorTexture()
	{
		if ((IsWindowOutput() != FALSE) || (OffscreenOutput.IsRenderResourceValid() == FALSE))
		{
			return nullptr;
		}
		return OffscreenOutput.GetResolvedRHITexture();
	}
	void RSceneRenderer::InitNewFrame()
	{
		// SwapCommandSlots is called by the GameThread at frame boundary
		// (between WaitForRenderIdle and KickRender) so that here we are
		// guaranteed the front slot is the freshly-published GameThread
		// payload and BackSlot is reserved for the next GameThread tick.

#if ENGINE_EDITOR_ONLY
		// A secondary renderer (the asset preview) does not own the debug primitive frame:
		// draining it here would consume the commands the main view is about to draw.
		if (bManageDebugWireframe != FALSE)
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
		if (!RenderDevice)
		{
			return;
		}

		// The proxies a frame draws bind their engine blocks through the renderer published
		// here. Saving and restoring the previous one is what keeps the ambient handle
		// meaningful for whichever renderer records next: a secondary renderer (the asset
		// preview) publishes itself while it records, and the main scene records after it.
		RSceneRenderer* PreviousActiveRenderer = EMainManager::GetManagerSingleton()->GetActiveSceneRenderer();
		EMainManager::GetManagerSingleton()->SetActiveSceneRenderer(this);

		// The GPU profiler's frame slot is process wide and the window renderer owns it. An
		// offscreen renderer records inside the same device frame, so its passes are not part
		// of that slice and are dropped instead of charged to it.
		EGPUProfiler::GetManagerSingleton()->SetPassRecordingEnabled(static_cast<BOOL8>(IsWindowOutput()));

		FrameData.BeginFrame(RenderDevice->GetCurrentFrameIndex());
		FrameData.SetRenderDevice(RenderDevice);
		RecordScheduler.BeginFrame(FrameData);

		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_SETUP);

			// The engine driven pass states are retried here, once per frame, because the first
			// attempt (renderer Initialize) runs before the shader device objects exist.
			EnsureEnginePassPipelineStates();

			// The per view light lists, encoded and uploaded once the pass states above are known.
			UploadLightListsForViews();

			// Per frame values. A no-op until the first draw gives the block a layout.
			UpdateGlobalParameters();
		}

		// Prepare: build the pass instances, then collect and publish every item they consume.
		PassStatePlan.Reset();
		FramePassInstances.SetNum(0);
		TArray<FPassViewInstance>& PassInstances = FramePassInstances;
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_INIT_FRAME);
			BuildPassInstances(PassInstances);
			PreparePassInstances(PassInstances);
			FrameData.EndPrepare();
		}

		// Record the simulation stage before anything is shaded. The stage has no items: its
		// sync list dispatches the water simulation bodies in order, because the SWE passes
		// depend on each other and their resource barriers are recorded there.
		{
			TArray<FRecordTask> SimulationTasks;
			RecordScheduler.ExecuteStage(ERenderPassStageType::RENDER_PASS_STAGE_SIMULATION, SimulationTasks
				, [this](IRCommandList* InList) -> void
				{
					RecordStageSync(ERenderPassStageType::RENDER_PASS_STAGE_SIMULATION, InList);
				});
		}

		// Record the shadow stage: its sync list carries the frame's GPU setup and the atlas
		// transitions, then every recorder runs its share of the caster items.
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_SHADOWS);
			TArray<FRecordTask> ShadowTasks;
			BuildStageTasks(PassInstances, ERenderPassStageType::RENDER_PASS_STAGE_SHADOW, ShadowTasks);
			RecordScheduler.ExecuteStage(ERenderPassStageType::RENDER_PASS_STAGE_SHADOW, ShadowTasks
				, [this](IRCommandList* InList) -> void
				{
					RecordStageSync(ERenderPassStageType::RENDER_PASS_STAGE_SHADOW, InList);
				});
		}

		// Record the base stage: the GBuffer transitions and clears, then the base items.
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_BASE_PASS);
			TArray<FRecordTask> BaseTasks;
			BuildStageTasks(PassInstances, ERenderPassStageType::RENDER_PASS_STAGE_BASE, BaseTasks);
			RecordScheduler.ExecuteStage(ERenderPassStageType::RENDER_PASS_STAGE_BASE, BaseTasks
				, [this](IRCommandList* InList) -> void
				{
					RecordStageSync(ERenderPassStageType::RENDER_PASS_STAGE_BASE, InList);
				});
		}

		// The engine driven tail records on its own list index and executes after every item
		// list. A separate allocator is what lets the tail list be recorded before the batch
		// that already holds the shadow and base lists is submitted. D3D11 has a single
		// immediate context, so every list there is the same wrapper and index 0 is the only
		// valid one.
		const UINT32 TailListIndex = ((RenderDevice->GetMaxRecordThreadNum() <= 1u)
			? 0u : (RHI_MAX_RECORD_LIST_NUM - 1u));
		IRCommandList* MainList = RenderDevice->AcquireCommandList(TailListIndex);
		if (MainList)
		{
			MainList->SetRecordMode(ERHIRecordModeType::RHI_RECORD_MODE_IMMEDIATE);
			CurrentCommandList = MainList;
			{
				PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_FINAL_OUTPUT);
				RecordTailPasses(MainList);
			}

			// Close the GPU profile frame before submit so the readback is queued to fill in by
			// the time the frame's fence retires.
			if (IsWindowOutput() != FALSE)
			{
				PE_GPU_PROFILER_END_FRAME(MainList);
			}
			MainList->Close();
			CurrentCommandList = nullptr;
		}

		TArray<IRCommandList*> AllLists = RecordScheduler.GetRecordedLists();
		if (MainList)
		{
			AllLists.Add(MainList);
		}
		{
			PE_PROFILE_SCOPE(PROFILER_SCOPE_RENDER_SUBMIT);
			RenderDevice->SubmitCommandLists(AllLists.GetData(), AllLists.Num<UINT32>());
		}

		RecordScheduler.EndFrame();
		FrameData.EndFrame();

		// The next renderer to record publishes itself, and the one after that restores this
		// value in turn; keeping it in step is what makes the handle describe the renderer
		// whose frame is current instead of whichever recorded last.
		EMainManager::GetManagerSingleton()->SetActiveSceneRenderer(PreviousActiveRenderer);
	}
	FBasePass::FBasePass(RSceneRenderer* InRenderer)
		: FRenderPass(FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_BASE))
		, Renderer(InRenderer)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("FBasePass: a pass needs its renderer.")), (Renderer != nullptr));
	}
	BOOL32 FBasePass::ShouldUseView(const RViewProxy& InView) const
	{
		return (((InView.GetViewKind() == EViewKindType::VIEW_KIND_CAMERA) && (InView.IsMainRenderView() != FALSE)) ? TRUE : FALSE);
	}
	void FBasePass::BeginPass(FPassRecordContext& InContext) const
	{
		Renderer->BeginItemPass(InContext);
	}
	void FBasePass::RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, const UINT32 InBegin, const UINT32 InEnd) const
	{
		Renderer->RecordItemRange(InContext, InItems, InBegin, InEnd);
	}

	FShadowPass::FShadowPass(RSceneRenderer* InRenderer)
		: FRenderPass(FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_SHADOW))
		, Renderer(InRenderer)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("FShadowPass: a pass needs its renderer.")), (Renderer != nullptr));
	}
	BOOL32 FShadowPass::ShouldUseView(const RViewProxy& InView) const
	{
		return ((InView.GetViewKind() == EViewKindType::VIEW_KIND_SHADOW) ? TRUE : FALSE);
	}
	void FShadowPass::BeginPass(FPassRecordContext& InContext) const
	{
		Renderer->BeginItemPass(InContext);
	}
	void FShadowPass::RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems, const UINT32 InBegin, const UINT32 InEnd) const
	{
		Renderer->RecordItemRange(InContext, InItems, InBegin, InEnd);
	}

	FWaterSurfacePass::FWaterSurfacePass(RSceneRenderer* InRenderer)
		: FRenderPass(FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_WATER_SURFACE))
		, Renderer(InRenderer)
	{
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("FWaterSurfacePass: a pass needs its renderer.")), (Renderer != nullptr));
	}
	BOOL32 FWaterSurfacePass::ShouldUseView(const RViewProxy& InView) const
	{
		return (((InView.GetViewKind() == EViewKindType::VIEW_KIND_CAMERA) && (InView.IsMainRenderView() != FALSE)) ? TRUE : FALSE);
	}
	void FWaterSurfacePass::CollectItems(const FPassViewContext& InContext, const RViewPrimitiveList& InPrimitiveList
		, TArray<FPassItem>& OutItems) const
	{
		FRenderPass::CollectItems(InContext, InPrimitiveList, OutItems);
		if ((!InContext.View) || (OutItems.Num<UINT32>() < 2u))
		{
			return;
		}
		// Alpha blending is not commutative, so the items go back to front. The primitive
		// bounds centre is enough for water surfaces, which are large flat patches.
		const Vector3 ViewLocation = InContext.View->GetWorldLocation();
		std::sort(OutItems.GetData(), OutItems.GetData() + OutItems.Num<UINT32>()
			, [&ViewLocation](const FPassItem& InLeft, const FPassItem& InRight) -> BOOL32
			{
				const Vector3 LeftLocation = ((InLeft.Primitive != nullptr) ? InLeft.Primitive->GetWorldLocation() : Vector3::Zero());
				const Vector3 RightLocation = ((InRight.Primitive != nullptr) ? InRight.Primitive->GetWorldLocation() : Vector3::Zero());
				const Vector3 LeftDelta = LeftLocation - ViewLocation;
				const Vector3 RightDelta = RightLocation - ViewLocation;
				const FLOAT LeftDistance = (LeftDelta.x * LeftDelta.x) + (LeftDelta.y * LeftDelta.y) + (LeftDelta.z * LeftDelta.z);
				const FLOAT RightDistance = (RightDelta.x * RightDelta.x) + (RightDelta.y * RightDelta.y) + (RightDelta.z * RightDelta.z);
				return ((LeftDistance > RightDistance) ? TRUE : FALSE);
			});
	}
	void FWaterSurfacePass::BeginPass(FPassRecordContext& InContext) const
	{
		Renderer->BeginItemPass(InContext);
	}
	void FWaterSurfacePass::EndPass(FPassRecordContext& InContext) const
	{
		Renderer->EndItemPass(InContext);
	}
	void FWaterSurfacePass::RecordItems(FPassRecordContext& InContext, const TArray<FPassItem>& InItems
		, const UINT32 InBegin, const UINT32 InEnd) const
	{
		Renderer->RecordItemRange(InContext, InItems, InBegin, InEnd);
	}

	void RSceneRenderer::BuildPassInstances(TArray<FPassViewInstance>& OutInstances)
	{
		OutInstances.SetNum(0);
		if (!Scene)
		{
			return;
		}

		// Shadow views: one pass instance per used shadow view. The atlas transition and clear
		// are stage state, declared once per atlas, not once per tile.
		for (RViewShadowStateMapType::TIterator StateIt = ViewShadowStates.Begin(); StateIt != ViewShadowStates.End(); StateIt++)
		{
			RViewShadowState* ShadowState = StateIt->second;
			if ((!ShadowState) || (ShadowState->SlotNum <= 1u) || (ShadowState->bBuiltThisFrame == FALSE))
			{
				continue;
			}
			IRRHITexture* AtlasTexture = ShadowState->Atlas.GetTexture();
			if (!AtlasTexture)
			{
				continue;
			}
			PassStatePlan.AddTransition(ERenderPassStageType::RENDER_PASS_STAGE_SHADOW, AtlasTexture
				, ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
			FRHIClear AtlasClear;
			AtlasClear.Resource		= AtlasTexture;
			AtlasClear.ClearFlags	= 0x1u;
			AtlasClear.ClearDepth	= RCommonSettings::RENDER_DEPTH_FAR;
			PassStatePlan.AddClear(ERenderPassStageType::RENDER_PASS_STAGE_SHADOW, AtlasClear);

			for (UINT32 ViewIndex = 0u, ViewNum = ShadowState->ShadowViews.Num<UINT32>(); ViewIndex < ViewNum; ViewIndex++)
			{
				RShadowViewProxy* ShadowView = ShadowState->ShadowViews[ViewIndex];
				if ((!ShadowView) || (ShadowView->IsUsedThisFrame() == FALSE))
				{
					continue;
				}
				FPassViewInstance Instance;
				Instance.Pass		= ShadowPassObject.get();
				Instance.View		= ShadowView;
				Instance.ViewData	= FrameData.EnsureView(*ShadowView);
				Instance.Targets.DepthTarget	= AtlasTexture;
				Instance.Targets.Viewport		= ShadowView->GetAtlasTile();
				const EViewport& Tile = Instance.Targets.Viewport;
				Instance.Targets.Scissor		= ERect(Tile.TopLeftX, Tile.TopLeftY, Tile.TopLeftX + Tile.Width, Tile.TopLeftY + Tile.Height);
				Instance.Targets.ClearDepth		= RCommonSettings::RENDER_DEPTH_FAR;
				OutInstances.Add(Instance);
			}
		}

		// Main camera views: one base pass instance each.
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
			if ((!ViewProxy) || (ViewProxy->IsMainRenderView() == FALSE))
			{
				continue;
			}
			RSceneTextures* SceneTextures = ViewSceneTextures[ViewProxy->GetUniqueID()];
			if (!SceneTextures)
			{
				continue;
			}
			FPassViewInstance Instance;
			Instance.Pass		= BasePassObject.get();
			Instance.View		= ViewProxy;
			Instance.ViewData	= FrameData.EnsureView(*ViewProxy);
			Instance.Targets.Viewport = ViewProxy->GetRenderViewport();
			const EViewport& Viewport = Instance.Targets.Viewport;
			Instance.Targets.Scissor = ERect(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.TopLeftX + Viewport.Width, Viewport.TopLeftY + Viewport.Height);
			Instance.Targets.ClearDepth = 1.0f;

				IRRHITexture* BaseTargets[7] =
				{
					SceneTextures->SceneColor.GetResolvedRHITexture(),
					SceneTextures->GBufferA.GetResolvedRHITexture(),
					SceneTextures->GBufferB.GetResolvedRHITexture(),
					SceneTextures->GBufferC.GetResolvedRHITexture(),
					SceneTextures->GBufferD.GetResolvedRHITexture(),
					SceneTextures->GBufferE.GetResolvedRHITexture(),
					SceneTextures->GBufferF.GetResolvedRHITexture()
				};
				BOOL32 bTargetsUsable = TRUE;
				for (UINT32 TargetIndex = 0u; TargetIndex < 7u; TargetIndex++)
				{
					if (!BaseTargets[TargetIndex])
					{
						bTargetsUsable = FALSE;
						break;
					}
				}
				if ((bTargetsUsable == FALSE) || (!SceneTextures->SceneDepthStencil.GetResolvedRHITexture()))
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("BasePass: a scene target is not a usable render target.")));
					continue;
				}
				Instance.Targets.RenderTargetCount = 7u;
			for (UINT32 TargetIndex = 0u; TargetIndex < 7u; TargetIndex++)
			{
				Instance.Targets.RenderTargets[TargetIndex] = BaseTargets[TargetIndex];
				Instance.Targets.ClearColors[TargetIndex] = Color4(0.f, 0.f, 0.f, 0.f);
				PassStatePlan.AddTransition(ERenderPassStageType::RENDER_PASS_STAGE_BASE, BaseTargets[TargetIndex]
					, ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
				FRHIClear ColorClear;
				ColorClear.Resource		= BaseTargets[TargetIndex];
				ColorClear.ClearFlags	= 0u;
				ColorClear.ClearColor	= Color4(0.f, 0.f, 0.f, 0.f);
				PassStatePlan.AddClear(ERenderPassStageType::RENDER_PASS_STAGE_BASE, ColorClear);
			}
			Instance.Targets.DepthTarget = SceneTextures->SceneDepthStencil.GetResolvedRHITexture();
			PassStatePlan.AddTransition(ERenderPassStageType::RENDER_PASS_STAGE_BASE, Instance.Targets.DepthTarget
				, ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
			FRHIClear DepthClear;
			DepthClear.Resource		= Instance.Targets.DepthTarget;
			DepthClear.ClearFlags	= 0x1u;
			DepthClear.ClearDepth	= 1.0f;
			PassStatePlan.AddClear(ERenderPassStageType::RENDER_PASS_STAGE_BASE, DepthClear);

			OutInstances.Add(Instance);

			// The water surface pass is prepared with the base pass but recorded on the tail
			// list, after lighting. Its instance carries the scene colour and depth it draws
			// into; the items are collected from the same view list and sorted back to front.
			if (WaterSurfacePassObject)
			{
				IRRHITexture* WaterColorTarget = SceneTextures->SceneColor.GetResolvedRHITexture();
				IRRHITexture* WaterDepthTarget = SceneTextures->SceneDepthStencil.GetResolvedRHITexture();
				if ((WaterColorTarget) && (WaterDepthTarget))
				{
					FPassViewInstance WaterInstance;
					WaterInstance.Pass		= WaterSurfacePassObject.get();
					WaterInstance.View		= ViewProxy;
					WaterInstance.ViewData	= FrameData.EnsureView(*ViewProxy);
					WaterInstance.Targets.Viewport = ViewProxy->GetRenderViewport();
					const EViewport& WaterViewport = WaterInstance.Targets.Viewport;
					WaterInstance.Targets.Scissor = ERect(WaterViewport.TopLeftX, WaterViewport.TopLeftY
						, WaterViewport.TopLeftX + WaterViewport.Width, WaterViewport.TopLeftY + WaterViewport.Height);
					WaterInstance.Targets.RenderTargetCount = 1u;
					WaterInstance.Targets.RenderTargets[0] = WaterColorTarget;
					WaterInstance.Targets.DepthTarget = WaterDepthTarget;
					WaterInstance.Targets.ClearDepth = 1.0f;
					OutInstances.Add(WaterInstance);
				}
			}
		}
	}

	void RSceneRenderer::PreparePassInstances(TArray<FPassViewInstance>& InOutInstances)
	{
		UINT32 ShadowItemNum = 0u;
		UINT32 BaseItemNum = 0u;
		UINT32 ShadowViewNum = 0u;
		for (UINT32 InstanceIndex = 0u, InstanceNum = InOutInstances.Num<UINT32>(); InstanceIndex < InstanceNum; InstanceIndex++)
		{
			FPassViewInstance& Instance = InOutInstances[InstanceIndex];
			if ((!Instance.Pass) || (!Instance.View) || (!Instance.ViewData))
			{
				continue;
			}
			FPassViewContext Context;
			Context.View		= Instance.View;
			Context.ViewData	= Instance.ViewData;
			Context.FrameData	= &FrameData;
			Context.PassDesc	= &(Instance.Pass->GetPassDesc());
			Context.Targets		= Instance.Targets;
			Instance.Pass->CollectItems(Context, Instance.View->GetPrimitiveList(), Instance.Items);
			Instance.Pass->PrepareItems(FrameData, Instance.Items);

			// The view and global blocks are consumed by the engine tail as well as by the item
			// draws. An empty view produces no item, so the block is ensured here against the
			// engine program that declares it. Both calls are no-ops once the layout exists.
			IRRHIDevice* RenderDevice = FrameData.GetRenderDevice();
			const EShaderProgram* FallbackProgram = ((SceneLightingProgram) ? SceneLightingProgram : FullScreenProgram);
			if ((RenderDevice) && (FallbackProgram))
			{
				EnsureGlobalParameterBlock(RenderDevice, *FallbackProgram);
				Instance.View->EnsureParameterBlock(RenderDevice, *FallbackProgram);
			}
			if (Instance.Pass->GetStage() == ERenderPassStageType::RENDER_PASS_STAGE_SHADOW)
			{
				ShadowItemNum += Instance.Items.Num<UINT32>();
				ShadowViewNum += 1u;
			}
			else if (Instance.Pass->GetStage() == ERenderPassStageType::RENDER_PASS_STAGE_BASE)
			{
				BaseItemNum += Instance.Items.Num<UINT32>();
			}
		}
		PE_PROFILE_COUNTER(PROFILER_COUNTER_SHADOW_DRAW_CALLS, ShadowItemNum);
		PE_PROFILE_COUNTER(PROFILER_COUNTER_BASE_DRAW_CALLS, BaseItemNum);
		PE_PROFILE_COUNTER(PROFILER_COUNTER_SHADOW_VIEW_NUM, ShadowViewNum);
	}

	void RSceneRenderer::BuildStageTasks(const TArray<FPassViewInstance>& InInstances, const ERenderPassStageType InStage
		, TArray<FRecordTask>& OutTasks) const
	{
		// A task is a half open item range. A view with many casters is split so one huge view
		// cannot leave every other recorder idle; a small view stays one task.
		const UINT32 ItemChunk = 256u;
		for (UINT32 InstanceIndex = 0u, InstanceNum = InInstances.Num<UINT32>(); InstanceIndex < InstanceNum; InstanceIndex++)
		{
			const FPassViewInstance& Instance = InInstances[InstanceIndex];
			if ((!Instance.Pass) || (Instance.Pass->GetStage() != InStage))
			{
				continue;
			}
			const UINT32 ItemNum = Instance.Items.Num<UINT32>();
			if (ItemNum == 0u)
			{
				continue;
			}
			if ((Instance.Pass->IsItemParallel() == FALSE) || (ItemNum <= ItemChunk))
			{
				FRecordTask Task;
				Task.Pass		= Instance.Pass;
				Task.Instance	= &InInstances[InstanceIndex];
				Task.ItemBegin	= 0u;
				Task.ItemEnd	= ItemNum;
				OutTasks.Add(Task);
				continue;
			}
			for (UINT32 ItemBegin = 0u; ItemBegin < ItemNum; ItemBegin += ItemChunk)
			{
				FRecordTask Task;
				Task.Pass		= Instance.Pass;
				Task.Instance	= &InInstances[InstanceIndex];
				Task.ItemBegin	= ItemBegin;
				Task.ItemEnd	= EMath::Min(ItemBegin + ItemChunk, ItemNum);
				OutTasks.Add(Task);
			}
		}
	}

	void RSceneRenderer::RecordStageSync(const ERenderPassStageType InStage, IRCommandList* InCommandList)
	{
		if ((!InCommandList) || (!Scene))
		{
			return;
		}

		if (InStage == ERenderPassStageType::RENDER_PASS_STAGE_SIMULATION)
		{
			// The simulation stage is executed first, so its sync list opens the GPU profile
			// frame. Only the window renderer owns that frame: an offscreen renderer records
			// inside the same device frame and must not advance the process wide slot.
			if (IsWindowOutput() != FALSE)
			{
				PE_GPU_PROFILER_BEGIN_FRAME(InCommandList);
			}
			PE_GPU_PROFILER_PASS_BEGIN(InCommandList, PE_TEXT("WaterSimulation"));
			RenderWaterSimulation(InCommandList);
			PE_GPU_PROFILER_PASS_END(InCommandList);
			return;
		}

		if (InStage == ERenderPassStageType::RENDER_PASS_STAGE_SHADOW)
		{
			// The environment bakes and the renderer wide sampler table are recorded here:
			// they must run before any draw that samples them, and this list executes before
			// the whole frame's item lists.
			CurrentCommandList = InCommandList;
			for (UINT32 SamplerIndex = 0u; SamplerIndex < RSamplerType::SAMPLER_TYPE_COUNT; SamplerIndex++)
			{
				IRRHISampler* SamplerPtr = Samplers[SamplerIndex].RHISampler
					? Samplers[SamplerIndex].RHISampler
					: static_cast<IRRHISampler*>(&Samplers[SamplerIndex]);
				InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_VERTEX, SamplerIndex, SamplerPtr);
				InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, SamplerIndex, SamplerPtr);
				InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, SamplerIndex, SamplerPtr);
			}
			RenderEnvironmentSH();
			RenderEnvironmentBRDFLUT();
			RenderEnvironmentPrefilter();
			CurrentCommandList = nullptr;
		}

		const FPassStageState& StageState = PassStatePlan.GetStages()[static_cast<UINT32>(InStage)];
		for (UINT32 TransitionIndex = 0u, TransitionNum = StageState.Transitions.Num<UINT32>(); TransitionIndex < TransitionNum; TransitionIndex++)
		{
			const FRHITransition& Transition = StageState.Transitions[TransitionIndex];
			if ((!Transition.Resource) || (Transition.Resource->GetCurrentState() == Transition.After))
			{
				continue;
			}
			InCommandList->ResourceBarrier(Transition.Resource, Transition.Resource->GetCurrentState(), Transition.After);
		}
		for (UINT32 ClearIndex = 0u, ClearNum = StageState.Clears.Num<UINT32>(); ClearIndex < ClearNum; ClearIndex++)
		{
			const FRHIClear& Clear = StageState.Clears[ClearIndex];
			IRRHITexture* Target = static_cast<IRRHITexture*>(Clear.Resource);
			if ((!Target) || (Target->GetResourceType() != ERHIResourceTypeTag::RHI_RESOURCE_TYPE_TEXTURE))
			{
				continue;
			}
			if (Clear.ClearFlags == 0u)
			{
				InCommandList->ClearRenderTarget(Target, Clear.ClearColor);
			}
			else
			{
				InCommandList->ClearDepthStencil(Target, Clear.ClearFlags, Clear.ClearDepth, Clear.ClearStencil);
			}
		}
	}

	void RSceneRenderer::RenderWaterSimulation(IRCommandList* InCommandList)
	{
		if ((!InCommandList) || (!Scene))
		{
			return;
		}
		// The body proxies are not collected into any view; they live in the scene's primitive
		// list and are dispatched here, once per frame, before the shadow and base stages.
		const FRenderPassDesc& PassDescription = FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_SWE_INTEGRATE);
		const TArray<RPrimitiveSceneProxy*>& Primitives = Scene->GetPrimitives();
		const UINT64 WaterSimulationCategory = static_cast<UINT64>(ERenderPrimitiveCategoryType::RENDER_PRIMITIVE_CATEGORY_WATER_SIMULATION);
		for (UINT32 Index = 0u, Num = Primitives.Num<UINT32>(); Index < Num; Index++)
		{
			RPrimitiveSceneProxy* Primitive = Primitives[Index];
			if ((Primitive) && ((Primitive->GetPrimitiveCategoryMask() & WaterSimulationCategory) != 0ull))
			{
				Primitive->RecordCompute(InCommandList, PassDescription);
			}
		}
	}

	void RSceneRenderer::RenderWaterSurface(IRCommandList* InCommandList, RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		if ((!InCommandList) || (!InViewProxy) || (!WaterSurfacePassObject))
		{
			return;
		}
		FPassViewInstance* WaterInstance = nullptr;
		for (UINT32 Index = 0u, Num = FramePassInstances.Num<UINT32>(); Index < Num; Index++)
		{
			FPassViewInstance& Candidate = FramePassInstances[Index];
			if ((Candidate.Pass == WaterSurfacePassObject.get()) && (Candidate.View == InViewProxy))
			{
				WaterInstance = &Candidate;
				break;
			}
		}
		if ((!WaterInstance) || (WaterInstance->Items.Num<UINT32>() == 0u))
		{
			return;
		}

		// The water material samples the scene colour behind it for refraction, and a draw cannot
		// sample the target it writes. Copy the colour into the view's linear refraction target
		// first; the copy stays in scene referred luminance so the material can blend it with the
		// water's own scattering term before the display transform.
		IRRHITexture* RefractionTarget = ((InSceneTextures)
			? InSceneTextures->WaterRefractionColor.GetResolvedRHITexture() : nullptr);
		IRRHITexture* SceneColorSource = ((InSceneTextures)
			? InSceneTextures->SceneColor.GetResolvedRHITexture() : nullptr);
		IRRHIPipelineState* CopyState = PipelineStates[PIPELINE_STATE_WATER_REFRACTION_COPY];
		if (InSceneTextures)
		{
			InSceneTextures->bWaterRefractionValid = FALSE;
		}
		if ((CopyState != nullptr) && (WaterRefractionCopyProgram != nullptr)
			&& (RefractionTarget != nullptr) && (SceneColorSource != nullptr))
		{
			const ERHIResourceState SceneColorState = SceneColorSource->GetCurrentState();
			if (SceneColorState != ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE)
			{
				InCommandList->ResourceBarrier(SceneColorSource, SceneColorState, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
			}
			const ERHIResourceState RefractionState = RefractionTarget->GetCurrentState();
			if (RefractionState != ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET)
			{
				InCommandList->ResourceBarrier(RefractionTarget, RefractionState, ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
			}

			RRHIRenderPassDesc CopyPass;
			CopyPass.RenderTargetCount		= 1u;
			CopyPass.RenderTargets[0].Target = RefractionTarget;
			CopyPass.RenderTargets[0].LoadOp = ERHILoadOp::RHI_LOAD_OP_DONT_CARE;
			CopyPass.RenderTargets[0].StoreOp = ERHIStoreOp::RHI_STORE_OP_STORE;
			CopyPass.DepthStencil.Target	= nullptr;
			CopyPass.PassName				= PE_TEXT("WaterRefractionCopy");
			// The tail list records with EXTERNAL state and the transitions above, exactly like
			// the water pass that follows on the same list.
			CopyPass.bStatePrepared			= TRUE;
			InCommandList->BeginRenderPass(CopyPass);

			InCommandList->SetPipelineState(CopyState);
			BindEngineParameters(InCommandList, *WaterRefractionCopyProgram, InViewProxy);
			FullScreenTriangle.BindPrimitiveBuffers(InCommandList, *WaterRefractionCopyProgram);
			const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*WaterRefractionCopyProgram, EShaderStageType::SHADER_STAGE_PIXEL);
			if (PixelEntry)
			{
				EShaderParameterSet Parameters;
				Parameters.SetTexture(EName(), EName(PE_TEXT("CopySourceTexture")), SceneColorSource);
				BindShaderResources(InCommandList, Parameters, PixelEntry->BindingPlan
					, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, EName(SHADER_BINDING_SCOPE_COPY));
			}
			InCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);
			InCommandList->EndRenderPass();

			const ERHIResourceState CopiedState = RefractionTarget->GetCurrentState();
			if (CopiedState != ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE)
			{
				InCommandList->ResourceBarrier(RefractionTarget, CopiedState, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);
			}
			if (InSceneTextures)
			{
				InSceneTextures->bWaterRefractionValid = TRUE;
			}
		}

		// The pass instance was prepared with the base pass, but it is recorded after the
		// forward pass, on the same immediate tail list. State the two targets it needs here;
		// BeginItemPass assumes the stage sync list already did this, and there is no sync
		// list for a pass recorded on the tail.
		IRRHITexture* ColorTarget = WaterInstance->Targets.RenderTargets[0];
		if (ColorTarget)
		{
			const ERHIResourceState ColorState = ColorTarget->GetCurrentState();
			if (ColorState != ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET)
			{
				InCommandList->ResourceBarrier(ColorTarget, ColorState, ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET);
			}
		}
		IRRHITexture* DepthTarget = WaterInstance->Targets.DepthTarget;
		if (DepthTarget)
		{
			const ERHIResourceState DepthState = DepthTarget->GetCurrentState();
			if (DepthState != ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE)
			{
				InCommandList->ResourceBarrier(DepthTarget, DepthState, ERHIResourceState::RHI_RESOURCE_STATE_DEPTH_WRITE);
			}
		}

		FPassRecordContext Context;
		Context.CommandList		= InCommandList;
		Context.Instance		= WaterInstance;
		Context.FrameData		= &FrameData;
		Context.FrameIndex		= FrameData.GetFrameIndex();
		PE_GPU_PROFILER_PASS_BEGIN(InCommandList, PE_TEXT("WaterSurface"));
		WaterInstance->Pass->BeginPass(Context);
		WaterInstance->Pass->RecordItems(Context, WaterInstance->Items, 0u, WaterInstance->Items.Num<UINT32>());
		WaterInstance->Pass->EndPass(Context);
		PE_GPU_PROFILER_PASS_END(InCommandList);
	}

	void RSceneRenderer::BeginItemPass(FPassRecordContext& InContext) const
	{
		if ((!InContext.CommandList) || (!InContext.Instance))
		{
			return;
		}
		const FPassTargets& Targets = InContext.Instance->Targets;
		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount = Targets.RenderTargetCount;
		for (UINT32 TargetIndex = 0u; TargetIndex < Targets.RenderTargetCount; TargetIndex++)
		{
			Pass.RenderTargets[TargetIndex].Target		= Targets.RenderTargets[TargetIndex];
			Pass.RenderTargets[TargetIndex].LoadOp		= ERHILoadOp::RHI_LOAD_OP_LOAD;
			Pass.RenderTargets[TargetIndex].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
			Pass.RenderTargets[TargetIndex].ClearColor	= Targets.ClearColors[TargetIndex];
		}
		Pass.DepthStencil.Target		= Targets.DepthTarget;
		Pass.DepthStencil.DepthLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
		Pass.DepthStencil.DepthStoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.StencilLoadOp	= ERHILoadOp::RHI_LOAD_OP_DONT_CARE;
		Pass.DepthStencil.StencilStoreOp = ERHIStoreOp::RHI_STORE_OP_DONT_CARE;
		Pass.DepthStencil.ClearDepth	= Targets.ClearDepth;
		Pass.DepthStencil.ClearStencil	= Targets.ClearStencil;
		// A worker list must not enter the shared GPU profiler pass stack. The stage sync list
		// carries the frame bracket; the CPU scopes carry the per pass attribution.
		Pass.PassName					= nullptr;
		Pass.bStatePrepared				= TRUE;
		InContext.CommandList->BeginRenderPass(Pass);
		InContext.CommandList->SetViewports(&Targets.Viewport, 1u);
		InContext.CommandList->SetScissorRects(&Targets.Scissor, 1u);
	}

	void RSceneRenderer::EndItemPass(FPassRecordContext& InContext) const
	{
		if (InContext.CommandList)
		{
			InContext.CommandList->EndRenderPass();
		}
	}

	void RSceneRenderer::RecordItemRange(FPassRecordContext& InContext, const TArray<FPassItem>& InItems
		, const UINT32 InBegin, const UINT32 InEnd) const
	{
		for (UINT32 ItemIndex = InBegin; ItemIndex < InEnd; ItemIndex++)
		{
			const FPassItem& Item = InItems[ItemIndex];
			if (Item.Primitive)
			{
				Item.Primitive->RecordPassItem(InContext, Item);
			}
		}
	}

	void RSceneRenderer::RecordViewTailPasses(IRCommandList* InCommandList)
	{
		if ((!InCommandList) || (!Scene))
		{
			return;
		}
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
			if ((!ViewProxy) || (ViewProxy->IsMainRenderView() == FALSE))
			{
				continue;
			}
			const EViewport& Viewport = ViewProxy->GetRenderViewport();
			const EViewport NewViewport(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.Width, Viewport.Height, Viewport.MinDepth, Viewport.MaxDepth);
			InCommandList->SetViewports(&NewViewport, 1u);
			const ERect ViewScissor(Viewport.TopLeftX, Viewport.TopLeftY, Viewport.TopLeftX + Viewport.Width, Viewport.TopLeftY + Viewport.Height);
			InCommandList->SetScissorRects(&ViewScissor, 1u);

			RSceneTextures* SceneTextures = ViewSceneTextures[ViewProxy->GetUniqueID()];
			CurrentViewProxy = ViewProxy;

			// The sky writes into the base pass MRT. In a single list the base pass left those
			// targets bound; here the base items live on other lists, so the tail binds the
			// same targets again before the sky draw. The stage sync list already transitioned
			// them, hence bStatePrepared.
			if (SceneTextures)
			{
				RRHIRenderPassDesc SkyPass;
				SkyPass.RenderTargetCount = 7u;
				IRRHITexture* SkyTargets[7] =
				{
					SceneTextures->SceneColor.GetResolvedRHITexture(),
					SceneTextures->GBufferA.GetResolvedRHITexture(),
					SceneTextures->GBufferB.GetResolvedRHITexture(),
					SceneTextures->GBufferC.GetResolvedRHITexture(),
					SceneTextures->GBufferD.GetResolvedRHITexture(),
					SceneTextures->GBufferE.GetResolvedRHITexture(),
					SceneTextures->GBufferF.GetResolvedRHITexture()
				};
				for (UINT32 TargetIndex = 0u; TargetIndex < 7u; TargetIndex++)
				{
					SkyPass.RenderTargets[TargetIndex].Target	= SkyTargets[TargetIndex];
					SkyPass.RenderTargets[TargetIndex].LoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
					SkyPass.RenderTargets[TargetIndex].StoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
				}
				SkyPass.DepthStencil.Target		= SceneTextures->SceneDepthStencil.GetResolvedRHITexture();
				SkyPass.DepthStencil.DepthLoadOp	= ERHILoadOp::RHI_LOAD_OP_LOAD;
				SkyPass.DepthStencil.DepthStoreOp	= ERHIStoreOp::RHI_STORE_OP_STORE;
				SkyPass.PassName				= nullptr;
				SkyPass.bStatePrepared			= TRUE;
				InCommandList->BeginRenderPass(SkyPass);
				RenderSky(ViewProxy);
				InCommandList->EndRenderPass();
			}

			RenderDirectionalShadowMask(ViewProxy, SceneTextures);
			RenderLighting(ViewProxy, SceneTextures);
			RenderForward(ViewProxy, SceneTextures);
			RenderWaterSurface(InCommandList, ViewProxy, SceneTextures);
		}
		CurrentViewProxy = nullptr;
	}

	void RSceneRenderer::RecordTailPasses(IRCommandList* InCommandList)
	{
		if ((!InCommandList) || (!Scene))
		{
			return;
		}

		// The shading belongs to the view; the overlay belongs to the window. An offscreen
		// renderer records the same shading and copy, and stops before the overlay.
		RecordViewTailPasses(InCommandList);

		FinalOutputPass();
#if ENGINE_EDITOR_ONLY
		if (IsWindowOutput() != FALSE)
		{
			ImGuiPass();
		}
#endif
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

		// The frame's flag word. Bit 0 says the shadow mask program has device objects this frame,
		// so the resolver runs for every view that has shadow data. Whether one particular view
		// actually produced a mask is a per view fact and travels in that view's light list: a
		// light with no shadow address is skipped before any mask is sampled, which is what keeps
		// this one frame wide bit honest for any number of views. The rest of the word is
		// reserved and written as zero.
		UINT32 RenderFlags[4] = { 0u, 0u, 0u, 0u };
		if ((ShadowMaskProgram != nullptr)
			&& (PipelineStates[RPipelineStateType::PIPELINE_STATE_DIRECTIONAL_SHADOW_MASK] != nullptr))
		{
			RenderFlags[0] |= SHADER_RENDER_FLAG_DIRECTIONAL_SHADOW_MASK_VALID;
		}
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
	void RSceneRenderer::BindEngineParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram, const RViewProxy* InView)
	{
		if (!InCommandList)
		{
			return;
		}
		// The blocks are published during Prepare. This call only replays the bind, which is
		// what keeps a worker record path free of value writes.
		BindGlobalParameters(InCommandList, InProgram);
		if (InView)
		{
			InView->BindParameters(InCommandList, InProgram);
		}
	}
	IRRHITexture* RSceneRenderer::ResolveOffscreenOutputTarget()
	{
		RViewProxy* OutputView = FindViewProxy(FinalOutputView);
		if (!OutputView)
		{
			return nullptr;
		}

		const EViewport& Viewport = OutputView->GetRenderViewport();
		const UINT32 OutputWidth  = static_cast<UINT32>(EMath::Max(Viewport.Width, 2.f));
		const UINT32 OutputHeight = static_cast<UINT32>(EMath::Max(Viewport.Height, 2.f));
		if ((OffscreenOutput.IsRenderResourceValid() != FALSE)
			&& (OffscreenOutputSize.x == static_cast<INT32>(OutputWidth))
			&& (OffscreenOutputSize.y == static_cast<INT32>(OutputHeight)))
		{
			return OffscreenOutput.GetResolvedRHITexture();
		}

		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return nullptr;
		}

		// The format comes from the final output pass row unless the owner named one: a
		// pipeline change to that row then moves the target with it, instead of leaving the
		// copy drawing into a target the state was not built for.
		const RFormatType OutputFormat = ((OutputDescription.OffscreenFormat != RFormatType::FORMAT_UNKNOWN)
			? OutputDescription.OffscreenFormat
			: FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_FINAL_OUTPUT).RenderTargetFormats[0]);

		OffscreenOutput.ReleaseRenderResource();
		if (OffscreenOutput.InitFromRHI(RenderDevice,
			RTextureDesc(OutputWidth, OutputHeight,
				RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
				OutputFormat)) == FALSE)
		{
			return nullptr;
		}

		// The recorded size only moves once the target was built: a failed creation is then
		// retried by the next frame instead of being remembered as done.
		OffscreenOutputSize = Vector2Int(static_cast<INT32>(OutputWidth), static_cast<INT32>(OutputHeight));
		return OffscreenOutput.GetResolvedRHITexture();
	}

	void RSceneRenderer::FinalOutputPass()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}

		// The final copy belongs to the output view: it reads that view's scene colour and
		// applies that view's exposure. A window renderer writes the back buffer; an
		// offscreen renderer writes the target it owns at the same viewport size.
		//
		// The pass does not clear: the copy covers every pixel of its target, and the editor
		// overlay draws over the back buffer afterwards, so a clear would only be a full
		// screen write nobody sees. It would also take the slow path on the back buffer,
		// which is a DXGI resource whose optimized clear value the engine cannot register
		// (D3D12 #820). A frame with no output view has no copy either, so the target keeps
		// its previous contents instead of a defined grey.
		IRRHITexture* OutputTarget = ((IsWindowOutput() != FALSE)
			? RenderDevice->GetCurrentBackBuffer()
			: ResolveOffscreenOutputTarget());
		if (!OutputTarget)
		{
			return;
		}

		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 1u;
		Pass.RenderTargets[0].Target		= OutputTarget;
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_DONT_CARE;
		Pass.RenderTargets[0].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target			= nullptr;
		Pass.PassName						= PE_TEXT("FinalOutput");
		CurrentCommandList->BeginRenderPass(Pass);

		// D3D12 has no implicit viewport / scissor from OMSetRenderTargets
		// (D3D11 inherits them from the bound RT). Set both explicitly to
		// the target extents so the FullScreenTriangle actually covers the
		// framebuffer; without this the draw clip-rejects and the target
		// keeps whatever it held before.
		const FLOAT OutputWidth  = static_cast<FLOAT>(OutputTarget->GetWidth());
		const FLOAT OutputHeight = static_cast<FLOAT>(OutputTarget->GetHeight());
		const EViewport FullViewport(0.f, 0.f, OutputWidth, OutputHeight, 0.f, 1.f);
		CurrentCommandList->SetViewports(&FullViewport, 1u);
		const ERect FullScissor(0.f, 0.f, OutputWidth, OutputHeight);
		CurrentCommandList->SetScissorRects(&FullScissor, 1u);

		// The blit belongs to a view: it reads that view's scene colour and it applies that
		// view's exposure, so the view block has to be published and bound here. BasePass
		// clears the current view when it finishes, so the blit cannot count on it still being
		// set - and an unbound ViewParams reads as whatever the last user of that root left
		// behind, which for exposure is a stale gain rather than a neutral one.
		CurrentViewProxy = FindViewProxy(FinalOutputView);

		// The state carries the copy program's signature, so the blocks the shader reads are
		// bound after it, never before.
		IRRHIPipelineState* FinalOutputState = PipelineStates[PIPELINE_STATE_FINAL_OUTPUT];
		if ((FinalOutputState) && (FullScreenProgram))
		{
			CurrentCommandList->SetPipelineState(FinalOutputState);
			BindEngineParameters(CurrentCommandList, *FullScreenProgram, CurrentViewProxy);

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
						BindShaderResources(CurrentCommandList, Parameters
							, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL
							, EName(SHADER_BINDING_SCOPE_COPY));
					}
				}
				else
				{
					PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("FinalOutput: the scene colour source is not a usable texture.")));
				}
			}
			CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);
		}
		// Leaving the view published would make the next frame's first draw bind a view that is
		// no longer the one being recorded.
		CurrentViewProxy = nullptr;

		CurrentCommandList->EndRenderPass();

		if (IsWindowOutput() != FALSE)
		{
#if !ENGINE_EDITOR_ONLY
			// Non-editor build: nothing follows FinalOutput, so flip the back buffer straight
			// to PRESENT here. Editor build defers this to ImGuiPass, which needs the buffer
			// in RENDER_TARGET to overlay UI.
			CurrentCommandList->ResourceBarrier(OutputTarget,
				ERHIResourceState::RHI_RESOURCE_STATE_RENDER_TARGET,
				ERHIResourceState::RHI_RESOURCE_STATE_PRESENT);
#endif
		}
		else
		{
			// The editor samples this image in the same frame's overlay, so it has to be
			// readable before the list is submitted.
			TransitionToShaderReadable(OutputTarget);
		}
	}
#if ENGINE_EDITOR_ONLY
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
		CImGUIManager::GetManagerSingleton()->RenderDrawData(CurrentCommandList);

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

		// The mask this pass reads is this view's own. A view without one is shaded unshadowed:
		// its light list carries no shadow addresses, so the shader never samples, and binding
		// nothing here leaves the plan's null fill in a register nothing reads.
		const BOOL32 bHasShadowMask = ((InViewProxy != nullptr)
			&& (IsDirectionalShadowMaskAvailable(InViewProxy->GetUniqueID()) != FALSE)) ? TRUE : FALSE;

		// The pass blends into the scene colour target the base pass filled, so that target has
		// to be usable: a pass recorded against a target it cannot write would add nothing and
		// the frame would come out unlit.
		if (!IsUsableColorTarget(InSceneTextures->SceneColor))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Lighting: the scene colour target is not a usable render target.")));
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
		BindEngineParameters(CurrentCommandList, *LightingProgram, InViewProxy);

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
				// This view's two light lists. The staging is frame wide, but the shadow address
				// each element carries indexes this view's slot table, so the encoded bytes are
				// this view's own. The counts that bound the loops travel in GlobalParams and are
				// the same for every view. Looked up rather than created: the buffers are built
				// before the frame records, and creating one here would create a device resource
				// in the middle of a command list.
				RViewLightBuffers** ViewBuffersPtr = ((InViewProxy != nullptr)
					? ViewLightBuffers.FindValueAsPtr(InViewProxy->GetUniqueID()) : nullptr);
				if ((ViewBuffersPtr != nullptr) && (*ViewBuffersPtr != nullptr))
				{
					Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("DirectionalLights")), &(*ViewBuffersPtr)->DirectionalLightBuffer);
					Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("LocalLights")), &(*ViewBuffersPtr)->LocalLightBuffer);
				}
				// The environment's coefficients, written by the bake this frame and read here as the
				// indirect term's irradiance. A pass that has no sky, or a frame where the bake could
				// not be recorded, leaves the slot to the resource set's null fill - a null view reads
				// as zero coefficients, which is the environment contributing nothing, rather than as
				// whatever a previous pass left there.
				Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("EnvironmentSH")), &EnvironmentSHBuffer);
				// The split sum's two halves. Until the prefilter and the LUT have run, the engine
				// defaults stand in: a black cube and a black table mean the specular term is zero,
				// which is a defined "no environment reflection yet" rather than a stale descriptor.
				// The cube default is the one the shader's TextureCube can sample: a 2D array
				// default would be a view whose dimension the declaration cannot use.
				IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
				if (RenderDevice)
				{
					IRRHITexture* PrefilteredTexture = RenderDevice->GetDefaultTexture(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_CUBE);
					if ((EnvironmentPrefilterBakedSource != nullptr) && (PrefilteredEnvironmentCube.IsRenderResourceValid() != FALSE))
					{
						PrefilteredTexture = PrefilteredEnvironmentCube.Texture;
					}
					IRRHITexture* BRDFLUTTexture = RenderDevice->GetDefaultTexture(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D);
					if ((bEnvironmentBRDFLUTBaked != FALSE) && (EnvironmentBRDFLUT.IsRenderResourceValid() != FALSE))
					{
						BRDFLUTTexture = EnvironmentBRDFLUT.Texture;
					}
					Parameters.SetTexture(EName(), EName(PE_TEXT("PrefilteredEnvironmentCube")), PrefilteredTexture);
					Parameters.SetTexture(EName(), EName(PE_TEXT("EnvironmentBRDFLUT")), BRDFLUTTexture);
				}
				// This view's directional shadow mask. A view without one leaves the name out:
				// its light list carries no shadow address, so the shader never samples the
				// channel and the plan's null fill lands in a register nothing reads.
				if ((bHasShadowMask != FALSE)
					&& (InSceneTextures->DirectionalShadowMask.IsRenderResourceValid() != FALSE))
				{
					Parameters.SetTexture(EName(), EName(PE_TEXT("DirectionalShadowMask0"))
						, InSceneTextures->DirectionalShadowMask.GetResolvedRHITexture());
				}
				// The atlas and the slot table the local light shadow term samples. Bound from
				// this view's state, like the mask: a slot index addresses one view's table, and
				// the atlas is the one that view's caster pass filled. A view without usable
				// shadow data leaves both unbound, and its light list has no address to sample.
				RViewShadowState** ShadowStatePtr = ((InViewProxy != nullptr)
					? ViewShadowStates.FindValueAsPtr(InViewProxy->GetUniqueID()) : nullptr);
				if ((bHasShadowMask != FALSE) && (ShadowStatePtr != nullptr) && (*ShadowStatePtr != nullptr))
				{
					IRRHITexture* AtlasTexture = (*ShadowStatePtr)->Atlas.GetTexture();
					if (AtlasTexture)
					{
						Parameters.SetTexture(EName(), EName(PE_TEXT("ShadowMapAtlas")), AtlasTexture);
						Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("ShadowSlots")), &(*ShadowStatePtr)->SlotBuffer);
					}
				}
				BindShaderResources(CurrentCommandList, Parameters
					, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL
					, EName(SHADER_BINDING_SCOPE_LIGHTING));
			}
		}

		CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);

		CurrentCommandList->EndRenderPass();
	}
	BOOL32 RSceneRenderer::IsDirectionalShadowMaskAvailable(const ObjectIdentityType InViewIdentity)
	{
		if ((!Scene) || (!ShadowMaskProgram)
			|| (!PipelineStates[RPipelineStateType::PIPELINE_STATE_DIRECTIONAL_SHADOW_MASK]))
		{
			return FALSE;
		}

		// Every input is per view, and every one of them can legitimately be missing: the view may
		// cast no shadow this frame, its atlas may not exist yet, or its mask target may have
		// failed to create. A false answer is not an error - it makes the view shade unshadowed.
		if (ViewShadowStates.ContainsKey(InViewIdentity) == FALSE)
		{
			return FALSE;
		}
		RViewShadowState* ShadowState = ViewShadowStates[InViewIdentity];
		if ((!ShadowState) || (ShadowState->bBuiltThisFrame == FALSE)
			|| (ShadowState->SlotNum <= 1u) || (ShadowState->Atlas.GetTexture() == nullptr))
		{
			return FALSE;
		}

		// The resolver reads the view's depth and normal and writes the view's mask target, so
		// all three have to be usable.
		if (ViewSceneTextures.ContainsKey(InViewIdentity) == FALSE)
		{
			return FALSE;
		}
		RSceneTextures* SceneTextures = ViewSceneTextures[InViewIdentity];
		if ((!SceneTextures) || (SceneTextures->DirectionalShadowMask.IsRenderResourceValid() == FALSE)
			|| (SceneTextures->GBufferB.IsRenderResourceValid() == FALSE)
			|| (SceneTextures->SceneDepthStencil.IsRenderResourceValid() == FALSE))
		{
			return FALSE;
		}
		return TRUE;
	}
	void RSceneRenderer::RenderDirectionalShadowMask(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		if ((!CurrentCommandList) || (!InViewProxy) || (!InSceneTextures))
		{
			return;
		}
		const ObjectIdentityType ViewIdentity = InViewProxy->GetUniqueID();
		if (IsDirectionalShadowMaskAvailable(ViewIdentity) == FALSE)
		{
			return;
		}

		const EShaderProgram* MaskProgram = ShadowMaskProgram;
		IRRHIPipelineState* MaskState = PipelineStates[RPipelineStateType::PIPELINE_STATE_DIRECTIONAL_SHADOW_MASK];
		if ((!MaskProgram) || (!MaskState))
		{
			return;
		}
		RViewShadowState* ShadowState = ViewShadowStates[ViewIdentity];
		if (!ShadowState)
		{
			return;
		}

		IRRHITexture* AtlasTexture	= ShadowState->Atlas.GetTexture();
		IRRHITexture* MaskTarget	= InSceneTextures->DirectionalShadowMask.GetResolvedRHITexture();
		IRRHITexture* NormalTarget	= InSceneTextures->GBufferB.GetResolvedRHITexture();
		IRRHITexture* DepthTarget	= InSceneTextures->SceneDepthStencil.GetResolvedRHITexture();
		if ((!AtlasTexture) || (!MaskTarget) || (!NormalTarget) || (!DepthTarget))
		{
			return;
		}

		RRHIRenderPassDesc Pass;
		Pass.RenderTargetCount				= 1u;
		Pass.RenderTargets[0].Target		= MaskTarget;
		Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_CLEAR;
		// Cleared to one so a channel no directional light claims reads as fully lit. The pass
		// writes every pixel it covers, so the clear is only what a skipped or partial frame
		// leaves behind - and that value has to be the identity of the shadow term.
		Pass.RenderTargets[0].ClearColor	= Color4(1.f, 1.f, 1.f, 1.f);
		Pass.RenderTargets[0].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
		Pass.DepthStencil.Target			= nullptr;
		Pass.PassName						= PE_TEXT("DirectionalShadowMask");
		CurrentCommandList->BeginRenderPass(Pass);

		CurrentCommandList->SetPipelineState(MaskState);

		// The shared blocks belong to the pass that draws it: the state above brought its own
		// signature, which voided anything an earlier pass had left bound.
		BindEngineParameters(CurrentCommandList, *MaskProgram, InViewProxy);

		// The mask block: the reciprocal of the mask's pixel size, which is what the shader
		// converts its own pixel coordinate with. The size is the scene textures' own, because
		// the mask is created at the view's resolution.
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((RenderDevice) && (ShadowMaskParameterBlock.EnsureInitialized(RenderDevice, *MaskProgram) != FALSE))
		{
			const Vector2Int& MaskSize = InSceneTextures->GetTextureSize();
			const FLOAT InvMaskSize[2] =
			{
				1.f / static_cast<FLOAT>(EMath::Max(MaskSize.x, 2)),
				1.f / static_cast<FLOAT>(EMath::Max(MaskSize.y, 2)),
			};
			WriteShaderFloatArray(ShadowMaskParameterBlock.GetParameters()
				, EName(SHADER_BLOCK_SHADOW_MASK_PARAMS), EName(SHADER_FIELD_INV_MASK_SIZE), InvMaskSize, 2u);
			ShadowMaskParameterBlock.Upload(RenderDevice);
			ShadowMaskParameterBlock.Bind(CurrentCommandList, *MaskProgram);
		}

		FullScreenTriangle.BindPrimitiveBuffers(CurrentCommandList, *MaskProgram);

		// The depth and the normal were render targets until now, so the pass has to put them
		// into a state a pixel shader may read before it binds them.
		TransitionToShaderReadable(NormalTarget);
		TransitionDepthToShaderReadable(DepthTarget);

		// Every name lands on the register the mask program's own reflection put it at. The atlas
		// and the slot table are the shadow state's; the light list is the frame's, like every
		// other pass that reads it.
		const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*MaskProgram, EShaderStageType::SHADER_STAGE_PIXEL);
		if (PixelEntry)
		{
			EShaderParameterSet Parameters;
			Parameters.SetTexture(EName(), EName(PE_TEXT("DepthTexture")), DepthTarget);
			Parameters.SetTexture(EName(), EName(PE_TEXT("GBufferBTexture")), NormalTarget);
			Parameters.SetTexture(EName(), EName(PE_TEXT("ShadowMapAtlas")), AtlasTexture);
			// This view's light list: the shadow addresses the resolver follows are the ones this
			// view's slot table was built with. The buffers are built before the frame records,
			// so this is a lookup and not a creation.
			RViewLightBuffers** ViewBuffersPtr = ViewLightBuffers.FindValueAsPtr(ViewIdentity);
			if ((ViewBuffersPtr != nullptr) && (*ViewBuffersPtr != nullptr))
			{
				Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("DirectionalLights")), &(*ViewBuffersPtr)->DirectionalLightBuffer);
			}
			Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("ShadowSlots")), &ShadowState->SlotBuffer);
			BindShaderResources(CurrentCommandList, Parameters
				, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL
				, EName(SHADER_BINDING_SCOPE_SHADOW_MASK));
		}

		CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);

		CurrentCommandList->EndRenderPass();

		// The lighting pass samples the mask next, so it leaves the pass readable rather than
		// leaving each consumer to state the same transition.
		TransitionToShaderReadable(MaskTarget);
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
		PE_GPU_PROFILER_PASS_BEGIN(CurrentCommandList, PE_TEXT("Sky"));

		for (UINT32 SkyLightIndex = 0u; SkyLightIndex < SkyLightNum; SkyLightIndex++)
		{
			RSkyLightSceneProxy* SkyLight = SkyLights.SceneProxies[SkyLightIndex];
#if ENGINE_EDITOR_ONLY
			if (!SkyLight)
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Exist a null sky light proxy.")));
				continue;
			}
			if (!(SkyLight->IsRenderValid()))
			{
				PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Exist a render invalid sky light proxy.")));
				continue;
			}
#endif
			SkyLight->BindRenderResource();
			SkyLight->Draw(CurrentCommandList, InViewProxy);
#if ENGINE_EDITOR_ONLY
			break;
#endif
		}

		PE_GPU_PROFILER_PASS_END(CurrentCommandList);
	}
	void RSceneRenderer::RenderForward(const RViewProxy* InViewProxy, RSceneTextures* InSceneTextures)
	{
		// Same requirement as the lighting pass: the forward geometry lands on top of the scene
		// colour it reads back, so a target it cannot write would drop the whole pass.
		if (!IsUsableColorTarget(InSceneTextures->SceneColor))
		{
			PE_FAILED((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Forward: the scene colour target is not a usable render target.")));
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
		// and leaves the pipeline state to them. A renderer that does not own the debug
		// primitive frame leaves the pass empty instead of drawing another view's overlay.
#if ENGINE_EDITOR_ONLY
		if (bRenderDebugWireframe != FALSE)
		{
			CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_LINELIST);
			DebugWireframePrimitiveManager->RenderPrimitives_RenderThread(CurrentCommandList, InViewProxy);
			CurrentCommandList->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		}
#endif

		CurrentCommandList->EndRenderPass();
	}
	// Defined with the shadow views below. The base pass list uses the same grouping, and the
	// definition sits there because both callers share it.
	static void SortViewPrimitiveListByShaderInstance(RViewPrimitiveList& InOutDrawList);

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
		// The light lists are re-encoded from scratch in InitLights, so the per view addresses they
		// read have to be this frame's. Clearing them here rather than at the end of the frame
		// leaves the encoding reading a table that only ever holds views built for the frame.
		ViewShadowAddresses.Empty();
		// A view that stopped being a main view keeps its shadow state, and InitShadowViews never
		// reaches its BeginFrame again. Clearing the flag here is what keeps RenderShadowViews from
		// recording that stale atlas for a view nothing shades.
		for (RViewShadowStateMapType::TIterator StateIt = ViewShadowStates.Begin(); StateIt != ViewShadowStates.End(); StateIt++)
		{
			if (StateIt->second)
			{
				StateIt->second->bBuiltThisFrame = FALSE;
			}
		}
		// The frame's caster union, before the shadow views that consume it. The scene commands
		// above are what make it this frame's set of proxies.
		CollectSceneCasterBounds();
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		{
			for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
			{
				RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if ENGINE_EDITOR_ONLY
				PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that view proxy can not be null")), (!!ViewProxy));
				if (!ViewProxy)
				{
					continue;
				}
#endif
				ViewProxy->ResetPrimitiveList();
				if (ViewProxy->IsMainRenderView())
				{
					FinalOutputView = ViewProxy->GetUniqueID();
				}
			}
		}

		for (INT32 ViewIndex = 0, ViewNum = ViewProxies.Num(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
#if ENGINE_EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that view proxy can not be null")), (!!ViewProxy));
			if (!ViewProxy)
			{
				continue;
			}
#endif
			// This view's shadow views. Built before the view's own culling because a shadow view
			// carries its own frustum and its own caster list, and before InitLights because the
			// light lists carry the slot each light landed in.
			{
				InitShadowViews(ViewProxy);
			}

			// The view's own list. A secondary view - an editor camera, a preview - is not shaded
			// through the deferred pipeline, so its list would be built and never read.
			if (ViewProxy->IsMainRenderView() != FALSE)
			{
				CollectViewPrimitiveList(*ViewProxy, ViewProxy->GetPrimitiveList());
				// Opaque geometry is order independent, so the base pass list is grouped the same
				// way a shadow view groups its casters: one material run costs one material bind,
				// and on D3D12 the root / descriptor work then scales with the material count
				// rather than with the draw count. The shadow path has always done this; the main
				// view list was the one still arriving in scene order.
				SortViewPrimitiveListByShaderInstance(ViewProxy->GetPrimitiveList());
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

		// Views that no longer exist take their atlas and slot buffer with them. Doing it after the
		// loop is what makes "the scene still has this view" a single lookup per state rather than
		// a flag kept in step with the loop.
		PruneViewShadowStates();
	}
	void RSceneRenderer::InitLights()
	{
		if (!Scene)
		{
			return;
		}
		LightLists.ReserveStaging();

		GatherDirectionalLights();
		GatherLocalLights();

		// The buffers travel with the counts that address them: a count is only meaningful
		// together with the list it indexes, so the two are produced and published together.
		// The per view upload happens at the start of Render, after the pass states are built:
		// whether a view's shadow data is usable depends on them, and encoding addresses the
		// resolver cannot honour would make the frame inconsistent.

#if ENGINE_EDITOR_ONLY
		ValidateLightBufferLayout();
#endif
	}
	void RSceneRenderer::UploadLightListsForViews()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!RenderDevice) || (!Scene))
		{
			return;
		}

		// One upload per main view, into that view's own buffers. A view whose shadow data is not
		// usable gets the empty address table, which encodes every light as "casts no shadow" -
		// the per view half of the frame wide flag the shader reads.
		const RShadowLightAddressMapType EmptyAddresses;
		TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
		for (UINT32 ViewIndex = 0u, ViewNum = ViewProxies.Num<UINT32>(); ViewIndex < ViewNum; ViewIndex++)
		{
			RViewProxy* ViewProxy = ViewProxies[ViewIndex];
			if ((ViewProxy == nullptr) || (ViewProxy->IsMainRenderView() == FALSE))
			{
				continue;
			}
			const ObjectIdentityType ViewIdentity = ViewProxy->GetUniqueID();
			RViewLightBuffers* ViewBuffers = EnsureViewLightBuffers(ViewIdentity);
			if (!ViewBuffers)
			{
				continue;
			}
			const RShadowLightAddressMapType* ViewAddresses = nullptr;
			if (IsDirectionalShadowMaskAvailable(ViewIdentity) != FALSE)
			{
				ViewAddresses = ViewShadowAddresses.FindValueAsPtr(ViewIdentity);
			}
			LightLists.UploadForView(RenderDevice, *ViewBuffers
				, ((ViewAddresses != nullptr) ? *ViewAddresses : EmptyAddresses));
		}
	}
	RSceneRenderer::RViewLightBuffers* RSceneRenderer::EnsureViewLightBuffers(const ObjectIdentityType InViewIdentity)
	{
		if (ViewLightBuffers.ContainsKey(InViewIdentity) == FALSE)
		{
			ViewLightBuffers.Add(InViewIdentity, nullptr);
		}
		RViewLightBuffers* ViewBuffers = ViewLightBuffers[InViewIdentity];
		if (!ViewBuffers)
		{
			ViewBuffers = new RViewLightBuffers();
			IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
			ViewBuffers->EnsureBuffers(RenderDevice);
			ViewLightBuffers[InViewIdentity] = ViewBuffers;
		}
		return ViewBuffers;
	}
	void RSceneRenderer::GatherDirectionalLights()
	{
		LightLists.DirectionalLightStaging.Empty();
		LightLists.DirectionalLightIdentities.Empty();
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
#if ENGINE_EDITOR_ONLY
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that light proxy can not be null")), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			EDirectionalLightsGPU& Element = LightLists.DirectionalLightStaging.AddDefaultGetRef();
			// No shadow address here. The address indexes one view's slot table, so it is filled
			// per view by UploadLightListsForViews rather than once for the frame; the identity is
			// recorded beside the element so that upload can find it again.
			EncodeDirectionalLight(LightProxy->GetLightData(), LightProxy->GetWorldRotation(), 0u, 0u, Element);
			LightLists.DirectionalLightIdentities.Add(LightProxy->GetUniqueID());
			LightLists.DirectionalLightCount += 1u;
		}
	}
	void RSceneRenderer::GatherLocalLights()
	{
		LightLists.LocalLightStaging.Empty();
		LightLists.LocalLightIdentities.Empty();
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
#if ENGINE_EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), (PE_TEXT("Check renderer failed that local light proxy can not be null")), (!!LightProxy));
			if (!LightProxy)
			{
				continue;
			}
#endif
			ELocalLightsGPU& Element = LightLists.LocalLightStaging.AddDefaultGetRef();
			// No shadow address, for the same reason the directional gather leaves it out: the
			// first slot of a light's views belongs to one view's slot table, and it is written
			// per view before that view records.
			EncodeLocalLight(LightProxy->GetLightData(), LightProxy->GetWorldLocation(), LightProxy->GetWorldRotation()
				, 0u, Element);
			LightLists.LocalLightIdentities.Add(LightProxy->GetUniqueID());
			LightLists.LocalLightCount += 1u;
		}
	}
	void RSceneRenderer::RenderEnvironmentSH()
	{
		if ((!Scene) || (!CurrentCommandList))
		{
			return;
		}
		if (!EnvironmentSHProgram)
		{
			// The whole indirect term is missing when this happens, and an absence is the hardest
			// thing to attribute from the picture alone, so it is reported rather than skipped
			// quietly. The report is deduplicated, so a session without the program costs one line.
			//
			// Asked for again before that report: the synchronous resolution at start up is the
			// only other caller, and if it ran before the shader's device objects existed the
			// program would stay missing for the session. RequireInstance is the any-thread form -
			// a cache hit once assembled, and a recorded request otherwise.
			if (const EShaderInstance* SHInstance = RequireEnvironmentSHProgramInstance())
			{
				EnvironmentSHProgramInstance = SHInstance;
				EnvironmentSHProgram = SHInstance->GetProgram();
			}
			if (!EnvironmentSHProgram)
			{
				ReportSkippedDraw(EString(PE_TEXT("the environment SH bake")), EString(PE_TEXT("its compute program did not assemble")));
				return;
			}
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
			ReportSkippedDraw(EString(PE_TEXT("the environment SH bake")), EString(PE_TEXT("its compute pipeline state could not be built for program '"))
				+ EnvironmentSHProgram->GetShaderName().ToString() + EString(PE_TEXT("'")));
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
			ReportSkippedDraw(EString(PE_TEXT("the environment SH bake")), EString(PE_TEXT("its coefficient buffer has no render resource")));
			return;
		}
		const EShaderStageEntry* ComputeEntry = ResolveShaderStageEntry(*EnvironmentSHProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
		if (!ComputeEntry)
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment SH bake")), EString(PE_TEXT("its compute program has no compute stage entry")));
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

		// A bake is a one-shot compute pass; it gets a profiler bracket of its
		// own so its cost is not folded into whichever frame happens to trigger
		// it. Every early out above leaves before this point.
		PE_GPU_PROFILER_PASS_BEGIN(CurrentCommandList, PE_TEXT("EnvironmentSH"));

		CurrentCommandList->SetPipelineState(EnvironmentSHPipelineState);

		// Bound by name through the compute stage's own plan, so neither a register nor a slot is
		// written down here - the same rule the light lists follow on the pixel stage.
		EShaderParameterSet Parameters;
		Parameters.SetTexture(EName(), EName(PE_TEXT("EnvironmentCube")), SourceTexture);
		Parameters.SetStructuredBuffer(EName(), EName(PE_TEXT("EnvironmentSHOutput")), &EnvironmentSHBuffer, TRUE);
		const BOOL32 bShaderResourcesBound = BindShaderResources(CurrentCommandList, Parameters, ComputeEntry->BindingPlan
			, ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_SH));

		// One threadgroup: the kernel hands one coefficient to each of nine lanes and lets each
		// scan the whole sample set itself, so there is no reduction, no group shared memory and
		// nothing here to size against the cubemap's resolution.
		if (bShaderResourcesBound != FALSE)
		{
			CurrentCommandList->Dispatch(1u, 1u, 1u);
		}

		PE_GPU_PROFILER_PASS_END(CurrentCommandList);

		CurrentCommandList->ResourceBarrier(CoefficientBuffer, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);

		if (bShaderResourcesBound == FALSE)
		{
			// The dispatch would have read an unbound cube and written zero coefficients, and the
			// bake would then be marked done - the indirect term would stay black for the session.
			ReportSkippedDraw(EString(PE_TEXT("the environment SH bake")), EString(PE_TEXT("its bindings could not be published")));
			return;
		}
		EnvironmentSHBakedSource = SourceTexture;

		// One line per bake and a bake only happens when the cube behind the sky changes, so this
		// is the record that the dispatch ran rather than that the pass was skipped.
#if ENGINE_EDITOR_ONLY
		PE_LOG_LOG(EString(PE_TEXT("[SceneRenderer] Baked the environment SH from the sky's cube.")));
#endif	// ENGINE_EDITOR_ONLY
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
	BOOL32 RSceneRenderer::EnsureEnvironmentPrefilterPipelineState()
	{
		if (EnvironmentPrefilterPipelineState)
		{
			return TRUE;
		}
		if (!EnvironmentPrefilterProgram)
		{
			return FALSE;
		}
		EnvironmentPrefilterPipelineState = PipelineStateCache.GetOrCreateCompute(*EnvironmentPrefilterProgram);
		return ((EnvironmentPrefilterPipelineState != nullptr) ? TRUE : FALSE);
	}
	BOOL32 RSceneRenderer::EnsureEnvironmentBRDFPipelineState()
	{
		if (EnvironmentBRDFPipelineState)
		{
			return TRUE;
		}
		if (!EnvironmentBRDFProgram)
		{
			return FALSE;
		}
		EnvironmentBRDFPipelineState = PipelineStateCache.GetOrCreateCompute(*EnvironmentBRDFProgram);
		return ((EnvironmentBRDFPipelineState != nullptr) ? TRUE : FALSE);
	}
	void RSceneRenderer::ReleaseSpecularIBLResources()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();

		if (EnvironmentBRDFLUTUAVView)
		{
			if (RenderDevice)
			{
				RenderDevice->DestroyTextureUAVView(EnvironmentBRDFLUTUAVView);
			}
			EnvironmentBRDFLUTUAVView = nullptr;
		}
		for (UINT32 ViewIndex = 0u; ViewIndex < PrefilteredEnvironmentUAVViews.Num<UINT32>(); ViewIndex++)
		{
			if ((RenderDevice) && (PrefilteredEnvironmentUAVViews[ViewIndex]))
			{
				RenderDevice->DestroyTextureUAVView(PrefilteredEnvironmentUAVViews[ViewIndex]);
			}
		}
		PrefilteredEnvironmentUAVViews.Empty();

		if (RenderDevice)
		{
			PrefilteredEnvironmentCube.Release(RenderDevice);
			EnvironmentBRDFLUT.Release(RenderDevice);
		}
		else
		{
			PrefilteredEnvironmentCube.ReleaseRenderResource();
			EnvironmentBRDFLUT.ReleaseRenderResource();
		}

		PrefilteredEnvironmentMipCount = 0u;
		PrefilteredEnvironmentResolutionExponent = 0u;
		EnvironmentPrefilterBakedSource = nullptr;
		EnvironmentPrefilterBakedMean = 0.f;
		bEnvironmentBRDFLUTBaked = FALSE;
	}
	BOOL32 RSceneRenderer::EnsurePrefilteredEnvironmentResources(IRRHIDevice* InRenderDevice, const UINT32 InResolutionExponent)
	{
		if ((!InRenderDevice) || (InResolutionExponent < 4u) || (InResolutionExponent > 10u))
		{
			return FALSE;
		}
		if ((PrefilteredEnvironmentResolutionExponent == InResolutionExponent)
			&& (PrefilteredEnvironmentCube.IsRenderResourceValid() != FALSE))
		{
			return TRUE;
		}

		// The exponent defines the base level; the chain runs down to 1 x 1, so a 256 base has
		// nine levels. Roughness maps linearly onto that chain at sample time.
		const UINT32 BaseResolution = (1u << InResolutionExponent);
		const UINT32 MipCount = (InResolutionExponent + 1u);

		for (UINT32 ViewIndex = 0u; ViewIndex < PrefilteredEnvironmentUAVViews.Num<UINT32>(); ViewIndex++)
		{
			if (PrefilteredEnvironmentUAVViews[ViewIndex])
			{
				InRenderDevice->DestroyTextureUAVView(PrefilteredEnvironmentUAVViews[ViewIndex]);
			}
		}
		PrefilteredEnvironmentUAVViews.Empty();
		PrefilteredEnvironmentCube.Release(InRenderDevice);
		EnvironmentPrefilterBakedSource = nullptr;
		EnvironmentPrefilterBakedMean = 0.f;

		RTextureDesc TextureDesc(BaseResolution, BaseResolution
			, static_cast<UINT8>(static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE) | static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS))
			, RFormatType::FORMAT_R16G16B16A16_FLOAT);
		TextureDesc.MipLevels = MipCount;
		// Six faces carrying the cube flag: the resource is written one mip at a time through
		// 2D array UAV views, since no D3D version has a cube UAV dimension, and it is read back
		// through a cube SRV. That read side is the reason for the flag: a cube view gives the
		// sampling side hardware direction lookup and the seamless filtering across face borders
		// that a 2D array view cannot express. D3D11 needs the flag on the resource; D3D12 takes
		// cube-ness from the view alone, so the same desc serves both.
		TextureDesc.ArraySize = 6u;
		TextureDesc.MiscFlags = static_cast<UINT8>(RResourceMiscFlagType::RESOURCE_MISC_TEXTURECUBE);
		if (InRenderDevice->CreateTexture2DResource(TextureDesc, Color4::Black(), RRHISubresourceData(), PrefilteredEnvironmentCube) == FALSE)
		{
			PrefilteredEnvironmentResolutionExponent = 0u;
			return FALSE;
		}

		for (UINT32 MipIndex = 0u; MipIndex < MipCount; MipIndex++)
		{
			RRHITextureUAVDesc UAVDesc;
			UAVDesc.MipSlice = MipIndex;
			UAVDesc.FirstArraySlice = 0u;
			UAVDesc.ArraySize = 6u;
			IRRHITextureUAVView* View = nullptr;
			if ((InRenderDevice->CreateTextureUAVView(PrefilteredEnvironmentCube.Texture, UAVDesc, &View) == FALSE) || (!View))
			{
				for (UINT32 CreatedIndex = 0u; CreatedIndex < PrefilteredEnvironmentUAVViews.Num<UINT32>(); CreatedIndex++)
				{
					if (PrefilteredEnvironmentUAVViews[CreatedIndex])
					{
						InRenderDevice->DestroyTextureUAVView(PrefilteredEnvironmentUAVViews[CreatedIndex]);
					}
				}
				PrefilteredEnvironmentUAVViews.Empty();
				PrefilteredEnvironmentCube.Release(InRenderDevice);
				PrefilteredEnvironmentResolutionExponent = 0u;
				return FALSE;
			}
			PrefilteredEnvironmentUAVViews.Add(View);
		}

		PrefilteredEnvironmentMipCount = MipCount;
		PrefilteredEnvironmentResolutionExponent = InResolutionExponent;
		return TRUE;
	}
	BOOL32 RSceneRenderer::EnsureBRDFLUTResources(IRRHIDevice* InRenderDevice)
	{
		if (!InRenderDevice)
		{
			return FALSE;
		}
		if ((EnvironmentBRDFLUT.IsRenderResourceValid() != FALSE) && (EnvironmentBRDFLUTUAVView != nullptr))
		{
			return TRUE;
		}
		if (EnvironmentBRDFLUTUAVView)
		{
			InRenderDevice->DestroyTextureUAVView(EnvironmentBRDFLUTUAVView);
			EnvironmentBRDFLUTUAVView = nullptr;
		}
		EnvironmentBRDFLUT.Release(InRenderDevice);
		bEnvironmentBRDFLUTBaked = FALSE;

		// 256 x 256 is the usual size for a DFG table: the function is smooth, and the table is
		// generated once. R16G16_FLOAT keeps the pair exact enough that the fit error is gone.
		RTextureDesc TextureDesc(256u, 256u
			, static_cast<UINT8>(static_cast<UINT8>(RBindFlagType::BIND_SHADER_RESOURCE) | static_cast<UINT8>(RBindFlagType::BIND_UNORDERED_ACCESS))
			, RFormatType::FORMAT_R16G16_FLOAT);
		if (InRenderDevice->CreateTexture2DResource(TextureDesc, Color4::Black(), RRHISubresourceData(), EnvironmentBRDFLUT) == FALSE)
		{
			return FALSE;
		}

		RRHITextureUAVDesc UAVDesc;
		UAVDesc.MipSlice = 0u;
		UAVDesc.FirstArraySlice = 0u;
		UAVDesc.ArraySize = 1u;
		if ((InRenderDevice->CreateTextureUAVView(EnvironmentBRDFLUT.Texture, UAVDesc, &EnvironmentBRDFLUTUAVView) == FALSE)
			|| (!EnvironmentBRDFLUTUAVView))
		{
			EnvironmentBRDFLUT.Release(InRenderDevice);
			return FALSE;
		}
		return TRUE;
	}
	void RSceneRenderer::RenderEnvironmentBRDFLUT()
	{
		if ((!CurrentCommandList) || (bEnvironmentBRDFLUTBaked != FALSE))
		{
			return;
		}
		if (!EnvironmentBRDFProgram)
		{
			// Same retry as the SH bake: the start up resolution is sync, and a program that was
			// not assembled then would never be asked for again, because this bake is one shot.
			if (const EShaderInstance* BRDFInstance = RequireEnvironmentBRDFProgramInstance())
			{
				EnvironmentBRDFProgramInstance = BRDFInstance;
				EnvironmentBRDFProgram = BRDFInstance->GetProgram();
			}
			if (!EnvironmentBRDFProgram)
			{
				ReportSkippedDraw(EString(PE_TEXT("the environment BRDF LUT")), EString(PE_TEXT("its compute program did not assemble")));
				return;
			}
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}
		if ((EnsureEnvironmentBRDFPipelineState() == FALSE) || (EnsureBRDFLUTResources(RenderDevice) == FALSE))
		{
			// This bake runs once and leaves no other trace, so a failure that only skipped it
			// would look from the picture exactly like a feature nobody implemented: the specular
			// term would sample a black table and contribute zero.
			ReportSkippedDraw(EString(PE_TEXT("the environment BRDF LUT")), EString(PE_TEXT("its pipeline state or lookup table could not be created")));
			return;
		}
		const EShaderStageEntry* ComputeEntry = ResolveShaderStageEntry(*EnvironmentBRDFProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
		if (!ComputeEntry)
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment BRDF LUT")), EString(PE_TEXT("its compute program has no compute stage entry")));
			return;
		}
		if (EnvironmentBRDFParameterBlock.EnsureInitialized(RenderDevice, *EnvironmentBRDFProgram) == FALSE)
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment BRDF LUT")), EString(PE_TEXT("its parameter block is not declared by its own program")));
			return;
		}
		IRRHITexture* LUTTexture = EnvironmentBRDFLUT.Texture;
		if (!LUTTexture)
		{
			return;
		}

		const UINT32 Resolution = 256u;
		const UINT32 SampleCount = 512u;
		const EName BlockName(PE_TEXT("BRDFLUTParams"));
		EShaderParameterSet& BlockParameters = EnvironmentBRDFParameterBlock.GetParameters();
		WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("Resolution")), &Resolution, 1u);
		WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("SampleCount")), &SampleCount, 1u);
		EnvironmentBRDFParameterBlock.Upload(RenderDevice);

		const ERHIResourceState StateBefore = LUTTexture->GetCurrentState();
		if (StateBefore != ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			CurrentCommandList->ResourceBarrier(LUTTexture, StateBefore, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		// One-shot bake, bracketed like the SH bake. Every early out above
		// leaves before this point.
		PE_GPU_PROFILER_PASS_BEGIN(CurrentCommandList, PE_TEXT("EnvironmentBRDF"));

		CurrentCommandList->SetPipelineState(EnvironmentBRDFPipelineState);
		EnvironmentBRDFParameterBlock.Bind(CurrentCommandList, *EnvironmentBRDFProgram);

		EShaderParameterSet Parameters;
		Parameters.SetTextureUAV(EName(), EName(PE_TEXT("EnvironmentBRDFLUT")), EnvironmentBRDFLUTUAVView);
		const BOOL32 bShaderResourcesBound = BindShaderResources(CurrentCommandList, Parameters, ComputeEntry->BindingPlan
			, ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_BRDF));

		const UINT32 GroupNum = ((Resolution + 7u) / 8u);
		if (bShaderResourcesBound != FALSE)
		{
			CurrentCommandList->Dispatch(GroupNum, GroupNum, 1u);
		}

		PE_GPU_PROFILER_PASS_END(CurrentCommandList);

		CurrentCommandList->ResourceBarrier(LUTTexture
			, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);

		if (bShaderResourcesBound == FALSE)
		{
			// Same failure the prefilter guards against: a one-shot bake that dispatched against
			// an unbound table would write nothing and then be marked done.
			ReportSkippedDraw(EString(PE_TEXT("the environment BRDF LUT")), EString(PE_TEXT("its bindings could not be published")));
			return;
		}
		bEnvironmentBRDFLUTBaked = TRUE;

#if ENGINE_EDITOR_ONLY
		// The counterpart of the prefilter's line: the two halves of the split sum each say they
		// were produced, so a session that shows no reflection can be told from a session that
		// never baked one.
		PE_LOG_LOG(EString(PE_TEXT("[SceneRenderer] Baked the environment BRDF LUT at "))
			+ ToString(Resolution) + EString(PE_TEXT(" with ")) + ToString(SampleCount) + EString(PE_TEXT(" samples.")));
#endif	// ENGINE_EDITOR_ONLY
	}
	void RSceneRenderer::RenderEnvironmentPrefilter()
	{
		if ((!Scene) || (!CurrentCommandList))
		{
			return;
		}
		if (!EnvironmentPrefilterProgram)
		{
			// Same retry as the SH bake. Without this, a program the start up resolution missed
			// stays null for the session and the prefilter never records its dispatch - which
			// leaves the prefiltered cube at its creation contents and reads as "no environment
			// reflection" for every material.
			if (const EShaderInstance* PrefilterInstance = RequireEnvironmentPrefilterProgramInstance())
			{
				EnvironmentPrefilterProgramInstance = PrefilterInstance;
				EnvironmentPrefilterProgram = PrefilterInstance->GetProgram();
			}
			if (!EnvironmentPrefilterProgram)
			{
				ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its compute program did not assemble")));
				return;
			}
		}
		const RSkyLightSceneProxy* SkyLight = FindPrimarySkyLight();
		const IRRHITexture* SourceTexture = ((SkyLight != nullptr) ? SkyLight->GetEnvironmentTexture() : nullptr);
		if (!SourceTexture)
		{
			// A scene without a sky is legal, but it is also the first gate that produces a black
			// prefiltered cube, so it is named once per session rather than left to the picture.
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("the scene has no sky light with an environment cube")));
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}

		const UINT32 ResolutionExponent = SkyLight->GetPrefilterResolutionExponent();
		const FLOAT CubeMeanLuminance = ((SkyLight->GetCubeMeanLuminance() > 0.0001f)
			? SkyLight->GetCubeMeanLuminance() : 1.f);
		// One comparison for "nothing moved", a swapped cube, a changed mean and a changed
		// resolution alike: each of them is a different bake.
		if ((SourceTexture == EnvironmentPrefilterBakedSource)
			&& (ResolutionExponent == PrefilteredEnvironmentResolutionExponent)
			&& (CubeMeanLuminance == EnvironmentPrefilterBakedMean))
		{
			return;
		}

		if ((EnsureEnvironmentPrefilterPipelineState() == FALSE) || (EnsurePrefilteredEnvironmentResources(RenderDevice, ResolutionExponent) == FALSE))
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its pipeline state or prefiltered cube could not be created")));
			return;
		}
		const EShaderStageEntry* ComputeEntry = ResolveShaderStageEntry(*EnvironmentPrefilterProgram, EShaderStageType::SHADER_STAGE_COMPUTE);
		if (!ComputeEntry)
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its compute program has no compute stage entry")));
			return;
		}
		if (EnvironmentPrefilterParameterBlock.EnsureInitialized(RenderDevice, *EnvironmentPrefilterProgram) == FALSE)
		{
			// Silent before: a block the program does not declare means the dispatch would run
			// with every MipResolution/SampleCount at zero, which writes at most one texel per
			// face and then marks the bake as done. Naming it here is what keeps that from
			// reading as "the prefilter produced nothing".
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its parameter block is not declared by its own program")));
			return;
		}
		IRRHITexture* PrefilteredTexture = PrefilteredEnvironmentCube.Texture;
		if (!PrefilteredTexture)
		{
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its prefiltered cube has no render texture")));
			return;
		}

		const ERHIResourceState StateBefore = PrefilteredTexture->GetCurrentState();
		if (StateBefore != ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS)
		{
			CurrentCommandList->ResourceBarrier(PrefilteredTexture, StateBefore, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS);
		}

		// One pass covers every mip: the bracket stays outside the loop so the
		// frame shows a single "EnvironmentPrefilter" cost rather than a
		// same-named row per mip. The loop only breaks, never returns, so the
		// End below is always reached.
		PE_GPU_PROFILER_PASS_BEGIN(CurrentCommandList, PE_TEXT("EnvironmentPrefilter"));

		CurrentCommandList->SetPipelineState(EnvironmentPrefilterPipelineState);

		const EName BlockName(PE_TEXT("PrefilterParams"));
		const UINT32 BaseResolution = (1u << ResolutionExponent);
		const UINT32 SourceCubeResolution = EMath::Max(SourceTexture->GetWidth(), 1u);
		EShaderParameterSet Parameters;
		Parameters.SetTexture(EName(), EName(PE_TEXT("EnvironmentCube")), SourceTexture);
		BOOL32 bAllMipsDispatched = TRUE;
		for (UINT32 MipIndex = 0u; MipIndex < PrefilteredEnvironmentMipCount; MipIndex++)
		{
			if ((MipIndex >= PrefilteredEnvironmentUAVViews.Num<UINT32>()) || (!PrefilteredEnvironmentUAVViews[MipIndex]))
			{
				bAllMipsDispatched = FALSE;
				break;
			}
			const UINT32 MipResolution = EMath::Max(BaseResolution >> MipIndex, 1u);
			// Rough mips have fewer pixels but need more samples to smooth the kernel; the cap
			// keeps a one-shot bake bounded.
			const UINT32 SampleCount = EMath::Min(128u + (MipIndex * 64u), 512u);

			EShaderParameterSet& BlockParameters = EnvironmentPrefilterParameterBlock.GetParameters();
			WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("MipIndex")), &MipIndex, 1u);
			WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("MipCount")), &PrefilteredEnvironmentMipCount, 1u);
			WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("MipResolution")), &MipResolution, 1u);
			WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("SourceCubeResolution")), &SourceCubeResolution, 1u);
			WriteShaderUIntArray(BlockParameters, BlockName, EName(PE_TEXT("SampleCount")), &SampleCount, 1u);
			WriteShaderScalar(BlockParameters, BlockName, EName(PE_TEXT("CubeMeanLuminance")), CubeMeanLuminance);
			EnvironmentPrefilterParameterBlock.Upload(RenderDevice);
			EnvironmentPrefilterParameterBlock.Bind(CurrentCommandList, *EnvironmentPrefilterProgram);

			Parameters.SetTextureUAV(EName(), EName(PE_TEXT("PrefilteredEnvironmentCube")), PrefilteredEnvironmentUAVViews[MipIndex]);
			if (BindShaderResources(CurrentCommandList, Parameters, ComputeEntry->BindingPlan
				, ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, EName(SHADER_BINDING_SCOPE_ENVIRONMENT_PREFILTER)) == FALSE)
			{
				// The binding side refuses a set whose index driven resources have no index
				// fields. Dispatching anyway would write nothing and then mark the bake as done,
				// which is exactly the "prefiltered cube is black" state.
				bAllMipsDispatched = FALSE;
				break;
			}

			const UINT32 GroupNum = ((MipResolution + 7u) / 8u);
			CurrentCommandList->Dispatch(GroupNum, GroupNum, 6u);
		}

		PE_GPU_PROFILER_PASS_END(CurrentCommandList);

		CurrentCommandList->ResourceBarrier(PrefilteredTexture
			, ERHIResourceState::RHI_RESOURCE_STATE_UNORDERED_ACCESS, ERHIResourceState::RHI_RESOURCE_STATE_SHADER_RESOURCE);

		if ((bAllMipsDispatched == FALSE) || (PrefilteredEnvironmentMipCount == 0u))
		{
			// Every mip has to be written before the cube is readable: a partial chain would
			// leave the levels the sampling side reaches at their creation contents. The bake
			// is not marked done, so the next frame tries the whole chain again.
			ReportSkippedDraw(EString(PE_TEXT("the environment prefilter")), EString(PE_TEXT("its mip chain could not be dispatched in full")));
			return;
		}
		EnvironmentPrefilterBakedSource = SourceTexture;
		EnvironmentPrefilterBakedMean = CubeMeanLuminance;

#if ENGINE_EDITOR_ONLY
		PE_LOG_LOG(EString(PE_TEXT("[SceneRenderer] Prefiltered the environment cube at "))
			+ ToString(BaseResolution) + EString(PE_TEXT(" base with ")) + ToString(PrefilteredEnvironmentMipCount)
			+ EString(PE_TEXT(" mips, cube mean ")) + ToString(CubeMeanLuminance) + EString(PE_TEXT(" (divided out).")));
#endif	// ENGINE_EDITOR_ONLY
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
		// The normalisation divisor and the prefiltered chain's level count. The first is what
		// makes AdjustIntensity a true mean; the second is what lets the specular term map
		// perceptual roughness onto the chain without querying the texture.
		const FLOAT CubeMeanLuminance = ((SkyLight != nullptr)
			? ((SkyLight->GetCubeMeanLuminance() > 0.0001f) ? SkyLight->GetCubeMeanLuminance() : 1.f) : 1.f);
		const FLOAT PrefilterMipCount = ((PrefilteredEnvironmentMipCount > 0u)
			? static_cast<FLOAT>(PrefilteredEnvironmentMipCount) : 1.f);

		// Said once, because this pair is the number the whole indirect term is multiplied by: an
		// environment that shades nothing is either zero coefficients (the bake wrote nothing) or a
		// zero scale here, and the two are indistinguishable from the picture.
		if ((bIndirectLightingParamsReported == FALSE) && (SkyLight != nullptr))
		{
			bIndirectLightingParamsReported = TRUE;
			PE_LOG_LOG(EString(PE_TEXT("[SceneRenderer] The indirect term is scaled by the sky light's tint ("))
				+ ToString(AdjustColor.r) + EString(PE_TEXT(", ")) + ToString(AdjustColor.g) + EString(PE_TEXT(", "))
				+ ToString(AdjustColor.b) + EString(PE_TEXT(") times intensity ")) + ToString(AdjustIntensity)
				+ EString(PE_TEXT(".")));
		}

		// Written every frame and uploaded only when something moved: the block reports which
		// mirrors are stale, so a still sky costs a comparison rather than a copy.
		const EName BlockName(SHADER_BLOCK_INDIRECT_LIGHTING_PARAMS);
		EShaderParameterSet& Parameters = IndirectLightingParameterBlock.GetParameters();
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_SKY_LIGHT_COLOR_LUMINANCE)
			, Vector4(AdjustColor.r, AdjustColor.g, AdjustColor.b, AdjustIntensity));
		WriteShaderScalar(Parameters, BlockName, EName(SHADER_FIELD_SKY_CUBE_MEAN_LUMINANCE), CubeMeanLuminance);
		WriteShaderScalar(Parameters, BlockName, EName(SHADER_FIELD_PREFILTER_MIP_COUNT), PrefilterMipCount);
		IndirectLightingParameterBlock.Upload(RenderDevice);
		IndirectLightingParameterBlock.Bind(InCommandList, InProgram);
	}
	void RSceneRenderer::AddWaterLightingParameters(EShaderParameterSet& InOutOverrides, const RViewProxy* InView)
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if (!RenderDevice)
		{
			return;
		}

		// The light lists are this view's; the counts that bound the loops travel in GlobalParams
		// and were bound with the rest of the engine block before this call.
		if (InView)
		{
			RViewLightBuffers** ViewBuffersPtr = ViewLightBuffers.FindValueAsPtr(InView->GetUniqueID());
			if ((ViewBuffersPtr != nullptr) && (*ViewBuffersPtr != nullptr))
			{
				InOutOverrides.SetStructuredBuffer(EName(), EName(PE_TEXT("DirectionalLights"))
					, &(*ViewBuffersPtr)->DirectionalLightBuffer);
				InOutOverrides.SetStructuredBuffer(EName(), EName(PE_TEXT("LocalLights"))
					, &(*ViewBuffersPtr)->LocalLightBuffer);
			}
		}
		InOutOverrides.SetStructuredBuffer(EName(), EName(PE_TEXT("EnvironmentSH")), &EnvironmentSHBuffer);

		// The split sum's two halves, with the same "not baked yet" defaults the lighting pass
		// uses: a black cube and a black table mean zero reflection, not a stale descriptor.
		IRRHITexture* PrefilteredTexture = RenderDevice->GetDefaultTexture(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_CUBE);
		if ((EnvironmentPrefilterBakedSource != nullptr) && (PrefilteredEnvironmentCube.IsRenderResourceValid() != FALSE))
		{
			PrefilteredTexture = PrefilteredEnvironmentCube.Texture;
		}
		IRRHITexture* BRDFLUTTexture = RenderDevice->GetDefaultTexture(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D);
		if ((bEnvironmentBRDFLUTBaked != FALSE) && (EnvironmentBRDFLUT.IsRenderResourceValid() != FALSE))
		{
			BRDFLUTTexture = EnvironmentBRDFLUT.Texture;
		}
		InOutOverrides.SetTexture(EName(), EName(PE_TEXT("PrefilteredEnvironmentCube")), PrefilteredTexture);
		InOutOverrides.SetTexture(EName(), EName(PE_TEXT("EnvironmentBRDFLUT")), BRDFLUTTexture);

		// The refraction copy this view's water pass samples. It was recorded immediately before
		// the pass, so the contents are this frame's pre-water scene colour.
		if (InView)
		{
			// Black when this frame did not record the copy: the material's luma test then emits
			// no refraction correction instead of adding the previous frame's scene.
			IRRHITexture* RefractionTexture = RenderDevice->GetDefaultTexture(RDefaultBindingTextureType::DEFAULT_BINDING_TEXTURE_2D);
			RSceneTextures** SceneTexturesPtr = ViewSceneTextures.FindValueAsPtr(InView->GetUniqueID());
			if ((SceneTexturesPtr != nullptr) && (*SceneTexturesPtr != nullptr)
				&& ((*SceneTexturesPtr)->bWaterRefractionValid != FALSE))
			{
				IRRHITexture* CopiedTexture = (*SceneTexturesPtr)->WaterRefractionColor.GetResolvedRHITexture();
				if (CopiedTexture != nullptr)
				{
					RefractionTexture = CopiedTexture;
				}
			}
			InOutOverrides.SetTexture(EName(), EName(PE_TEXT("WaterRefractionColor")), RefractionTexture);
		}
	}
	void RSceneRenderer::BindWaterIndirectLightingParameters(IRCommandList* InCommandList, const EShaderProgram& InProgram)
	{
		// The indirect block belongs to the lighting pass but the values are frame wide, and the
		// water program declares the same block at its own slot. Publishing here uploads only the
		// mirrors that moved, so calling it per water draw is a comparison rather than a copy.
		BindIndirectLightingParameters(InCommandList, InProgram);
	}
	void RSceneRenderer::ValidateLightBufferLayout()
	{
#if ENGINE_EDITOR_ONLY
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
		// Every view has its own pair, built from the same descriptors, so one of them stands for
		// all: the first entry that exists is the one compared. A frame with no view has no pair
		// to check, and the stride comparison is skipped rather than reported as a mismatch.
		const RViewLightBuffers* SampledBuffers = nullptr;
		for (RViewLightBuffersMapType::TConstIterator BufferIt = ViewLightBuffers.Begin(); BufferIt != ViewLightBuffers.End(); BufferIt++)
		{
			if (BufferIt->second)
			{
				SampledBuffers = BufferIt->second;
				break;
			}
		}
		const UINT32 GpuStrides[] =
		{
			((SampledBuffers != nullptr) && (SampledBuffers->DirectionalLightBuffer.IsRenderResourceValid() != FALSE))
				? SampledBuffers->DirectionalLightBuffer.GetStructureByteStride() : 0u,
			((SampledBuffers != nullptr) && (SampledBuffers->LocalLightBuffer.IsRenderResourceValid() != FALSE))
				? SampledBuffers->LocalLightBuffer.GetStructureByteStride() : 0u
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

		// The shadow slot table is the same contract in another file: Components/C_Shadows.json
		// declares the element and RenderProxy/ShadowViewProxy.h mirrors it. Checked here because
		// this is the one place that already holds a program reflection and reports drift as a
		// warning; a mismatched slot table would read as a shadow in the wrong place rather than
		// as a failure.
		if (ShadowMaskProgram)
		{
			const EShaderStageEntry* MaskPixelEntry = ResolveShaderStageEntry(*ShadowMaskProgram, EShaderStageType::SHADER_STAGE_PIXEL);
			const EShaderReflectionResource* ShadowSlotResource = (MaskPixelEntry
				? MaskPixelEntry->Reflection.FindResource(EName(PE_TEXT("ShadowSlots")), FALSE) : nullptr);
			if (ShadowSlotResource)
			{
				UINT32 ShaderStride = 0u;
				for (UINT32 FieldIndex = 0u; FieldIndex < ShadowSlotResource->StructFields.Num<UINT32>(); FieldIndex++)
				{
					const EShaderReflectionField& Field = ShadowSlotResource->StructFields[FieldIndex];
					ShaderStride += (Field.Format.GetElementSlotBytes() * Field.GetElementNum());
				}
				const UINT32 CppStride = static_cast<UINT32>(sizeof(EShadowSlotGPU));
				if (ShaderStride != CppStride)
				{
					PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] 'ShadowSlots': the shader declares an element of "))
						+ ToString(ShaderStride) + EString(PE_TEXT(" bytes but the c++ mirror is "))
						+ ToString(CppStride)
						+ EString(PE_TEXT(" bytes. Components/C_Shadows.json and ShadowViewProxy.h have drifted apart.")));
				}
			}
		}
#endif	// ENGINE_EDITOR_ONLY
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
	// ---------------------------------------------------------------------------
	// Shadow views
	// ---------------------------------------------------------------------------

	/*
	* One shadow view the frame is about to build: a light's request, the tile the atlas gave it,
	* and the light it came from.
	*
	* The array of these is built in slot order - each light's requests consecutive - because that
	* is what lets one light reach all of its views from a single address in the light list.
	*/
	struct FShadowViewBuild
	{
		FShadowViewRequest		Request;
		const RBaseSceneProxy*	LightProxy		= nullptr;
		ObjectIdentityType		LightIdentity	= 0u;
		FShadowTile				Tile;
		UINT32					TileSize		= 0u;
		BOOL32					bIsAllocated	= FALSE;
	};

	/*
	* How many builds one light contributed, and where its block starts.
	*
	* Allocation and drop decisions are made per block rather than per request because a light is
	* all or nothing: a point light's six faces are addressed as one range, so keeping five of them
	* would hand the sixth face's projection to whichever light took that slot.
	*/
	struct FShadowLightBlock
	{
		UINT32	StartIndex	= 0u;
		UINT32	ViewNum		= 0u;
	};

	// A local light that may keep a shadow this frame, with the number that ranks it.
	struct FLocalShadowCandidate
	{
		RLocalLightSceneProxy*	LightProxy	= nullptr;
		ObjectIdentityType		Identity	= 0u;
		FLOAT					Coverage	= 0.f;
	};

	/*
	* Identity of a shadow view across frames, which is what lets the renderer rebuild a view
	* instead of allocating a new one (and with it a new ViewParams block) every frame.
	*
	* A frame's state is already per camera view, so the light and the view it asked for identify
	* it completely. The widths are what those values can use: at most four cascades and six point
	* faces, and the kinds are the three the enum states.
	*/
	static UINT64 MakeShadowViewKey(const ObjectIdentityType InLightIdentity, const EShadowViewKindType InKind, const UINT32 InIndex)
	{
		return ((InLightIdentity & 0x000000FFFFFFFFFFull)
			| (static_cast<UINT64>(InIndex & 0xFFu) << 40u)
			| (static_cast<UINT64>(InKind) << 48u));
	}
	// Places one tile, halving the request until the atlas accepts it. A refusal is a budget
	// signal rather than an error: the ladder is 2048, 1024, 512, 256 before a view is dropped.
	static BOOL32 TryAllocateShadowTile(RShadowAtlas& InAtlas, const UINT32 InRequestedSize
		, FShadowTile& OutTile, UINT32& OutSize)
	{
		UINT32 Size = EMath::Max(InRequestedSize, 256u);
		while (Size >= 256u)
		{
			if (InAtlas.AllocateTile(Size, OutTile) != FALSE)
			{
				OutSize = Size;
				return TRUE;
			}
			Size >>= 1u;
		}
		return FALSE;
	}
	/*
	* Fills one view's primitive list.
	*
	* The list is the cull's output and every pass that draws the view consumes it. A shadow
	* view adds the caster filter, because a primitive that does not cast is not part of that
	* view's work at all. A proxy that cannot state a bound is kept: a bound that cannot be
	* computed is not evidence of invisibility, and a caster dropped on a guess is a shadow that
	* silently disappears from the frame.
	*/
	void RSceneRenderer::CollectViewPrimitiveList(const RViewProxy& InView, RViewPrimitiveList& OutPrimitiveList
		, RViewPrimitiveBounds* OutBounds) const
	{
		if (!Scene)
		{
			return;
		}
		const EFrustum& InViewFrustum = InView.GetViewFrustum();
		// The frustum's own world AABB, used only to clip what a kept primitive contributes to
		// the union. A caster larger than the shadow view (the ground plane is the usual one)
		// would otherwise dominate the fit with geometry no receiver in this view can see.
		Vector3 FrustumWorldMin(PE_FLOAT32_MAX, PE_FLOAT32_MAX, PE_FLOAT32_MAX);
		Vector3 FrustumWorldMax(-PE_FLOAT32_MAX, -PE_FLOAT32_MAX, -PE_FLOAT32_MAX);
		if (OutBounds)
		{
			for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
			{
				FrustumWorldMin = MinVector3(FrustumWorldMin, InViewFrustum.FarNearPlanePoint[CornerIndex]);
				FrustumWorldMax = MaxVector3(FrustumWorldMax, InViewFrustum.FarNearPlanePoint[CornerIndex]);
			}
		}
		const TArray<RPrimitiveSceneProxy*>& Primitives = Scene->GetPrimitives();
		for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = Primitives.Num<UINT32>(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
		{
			RPrimitiveSceneProxy* Primitive = Primitives[PrimitiveIndex];
			if (!Primitive)
			{
				continue;
			}
			// The view's kind, the view's masks, the primitive's own hooks and the global
			// policy are one decision, made in the central filter. The frustum test below is
			// the only part the filter leaves to the caller.
			FRenderFilterContext FilterContext;
			FilterContext.View		= &InView;
			FilterContext.Primitive	= Primitive;
			FilterContext.FrameIndex = FrameData.GetFrameIndex();
			if (FRenderFilterRegistry::ShouldCollect(FilterContext) == FALSE)
			{
				continue;
			}
			Vector3 BoundMin;
			Vector3 BoundMax;
			const BOOL32 bHasBound = Primitive->GetWorldBound(BoundMin, BoundMax);
			if ((bHasBound != FALSE)
				&& (InViewFrustum.IntersectsWorldAABB(BoundMin, BoundMax) == FALSE))
			{
				continue;
			}
			OutPrimitiveList.Add(Primitive);
			if (OutBounds)
			{
				if (bHasBound != FALSE)
				{
					// The intersection of the primitive's bound and the frustum's own AABB. It
					// contains every part of the primitive the view can reach, and it cannot be
					// larger than the view itself.
					const Vector3 ClippedMin = MaxVector3(BoundMin, FrustumWorldMin);
					const Vector3 ClippedMax = MinVector3(BoundMax, FrustumWorldMax);
					OutBounds->Add(ClippedMin, ClippedMax);
				}
				else
				{
					OutBounds->MarkUnknown();
				}
			}
		}
	}
	/*
	* The world space union of this frame's casters, and whether it covers all of them.
	*
	* One pass over the scene's primitives, before any shadow view is built: the union is what a
	* directional cascade's depth range has to reach so an occluder between the light and the
	* slice is in the caster list. The completeness flag is separate from the bound because a
	* primitive that cannot state a bound still casts; the renderer then falls back to the
	* camera's reach rather than trusting a union that does not contain it.
	*/
	void RSceneRenderer::CollectSceneCasterBounds()
	{
		SceneCasterBounds			= EBoundAABB(Vector3::Zero(), Vector3::Zero(), FALSE);
		bSceneCasterBoundsComplete	= TRUE;
		if (!Scene)
		{
			return;
		}
		const TArray<RPrimitiveSceneProxy*>& Primitives = Scene->GetPrimitives();
		for (UINT32 PrimitiveIndex = 0u, PrimitiveNum = Primitives.Num<UINT32>(); PrimitiveIndex < PrimitiveNum; PrimitiveIndex++)
		{
			RPrimitiveSceneProxy* Primitive = Primitives[PrimitiveIndex];
			if ((!Primitive) || (Primitive->IsSceneProxyCastShadow() == FALSE))
			{
				continue;
			}
			Vector3 BoundMin;
			Vector3 BoundMax;
			if (Primitive->GetWorldBound(BoundMin, BoundMax) != FALSE)
			{
				SceneCasterBounds += EBoundAABB(BoundMin, BoundMax, TRUE);
			}
			else
			{
				bSceneCasterBoundsComplete = FALSE;
			}
		}
	}
	/*
	* Orders a view's list so a run of primitives that share a material stays together. A depth
	* only pass does not depend on draw order, so the reorder is free and is what lets the draw
	* path keep one material binding for the run.
	*/
	static void SortViewPrimitiveListByShaderInstance(RViewPrimitiveList& InOutDrawList)
	{
		TArray<RViewPrimitiveItem>& Items = InOutDrawList.GetItems();
		if (Items.Num<UINT32>() <= 1u)
		{
			return;
		}
		std::sort(Items.GetData(), Items.GetData() + Items.Num<UINT32>()
			, [](const RViewPrimitiveItem& InLeft, const RViewPrimitiveItem& InRight) -> bool
			{
				const UINT64 LeftKey = static_cast<UINT64>(reinterpret_cast<SIZE_T>(InLeft.SortKey));
				const UINT64 RightKey = static_cast<UINT64>(reinterpret_cast<SIZE_T>(InRight.SortKey));
				if (LeftKey != RightKey)
				{
					return (LeftKey < RightKey);
				}
				return (InLeft.Identity < InRight.Identity);
			});
	}
	/*
	* Whether a point light's cube face can contain a receiver the camera sees.
	*
	* The face covers the half space in front of its own direction. If all eight corners of the
	* camera frustum are behind that plane, the whole frustum is behind it (it is convex), so no
	* visible surface lies in that face and the casters for it do not have to be drawn. The slot
	* and its cleared tile stay in the table: the shader maps a point light's face to
	* (first slot + face), and dropping a request would renumber the faces after it.
	*/
	static BOOL32 DoesViewFrustumReachPointFace(const RViewProxy& InView, const Vector3& InLightLocation
		, const EViewDomainInfo& InFaceDomain)
	{
		const Matrix4x4& InverseView = InFaceDomain.ViewMatrix.ViewPart.InverseViewMatrix;
		const Vector3 FaceForward(InverseView(2u, 0u), InverseView(2u, 1u), InverseView(2u, 2u));
		const EFrustum& CameraFrustum = InView.GetViewFrustum();
		for (UINT32 CornerIndex = 0u; CornerIndex < 8u; CornerIndex++)
		{
			if (Vector3::Dot(CameraFrustum.FarNearPlanePoint[CornerIndex] - InLightLocation, FaceForward) > 0.f)
			{
				return TRUE;
			}
		}
		return FALSE;
	}
	/*
	* World size of one shadow texel, which is what turns the receiver's normal offset into metres.
	*
	* An orthographic cascade has one size everywhere, so its near plane's width answers exactly. A
	* perspective view's texel grows with distance, so one number can only be a reference: the
	* midpoint between the two planes is used, which makes the offset right at the middle of the
	* light's range and slightly conservative at either end of it.
	*/
	static FLOAT ComputeShadowWorldTexelSize(const EShadowViewKindType InKind, const EViewDomainInfo& InDomain
		, const Vector3& InViewAnchor, const UINT32 InTileSize)
	{
		const FLOAT TileSize = static_cast<FLOAT>(EMath::Max(InTileSize, 1u));
		const EFrustum& Frustum = InDomain.ViewFrustum;
		if (InKind == EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE)
		{
			const FLOAT OrthographicWidth = Vector3::Length(Frustum.NearPlaneTopLeft - Frustum.NearPlaneTopRight);
			return (OrthographicWidth / TileSize);
		}
		const FLOAT FarWidth		= Vector3::Length(Frustum.FarPlaneTopLeft - Frustum.FarPlaneTopRight);
		const FLOAT FarDistance		= Vector3::Length(Frustum.FarPlaneTopLeft - InViewAnchor);
		const FLOAT NearDistance	= Vector3::Length(Frustum.NearPlaneTopLeft - InViewAnchor);
		if (FarDistance < 1e-4f)
		{
			return 0.f;
		}
		// The frustum's far corners are the ones that carry the field of view, so the width there
		// scaled by the reference distance is the width the reference plane sees.
		const FLOAT ReferenceDistance = 0.5f * (NearDistance + FarDistance);
		return ((FarWidth * (ReferenceDistance / FarDistance)) / TileSize);
	}
	/*
	* Writes one element of the slot table.
	*
	* The matrix travels as its four rows because a parameter cannot carry sixteen scalars in one
	* field; the shader reassembles float4x4(Row0..Row3) and applies mul(Position, ViewProjection),
	* which is the same product the cpu-side matrix states.
	*/
	static void FillShadowSlot(const RShadowViewProxy& InView, const FShadowViewBuild& InBuild
		, const UINT32 InAtlasSize, const RShadowSettings& InSettings, EShadowSlotGPU& OutSlot)
	{
		const Matrix4x4& ViewProjection = InView.GetViewMatrix().ViewProjectionMatrix;
		OutSlot.ViewProjectionRow0 = Vector4(ViewProjection(0u, 0u), ViewProjection(0u, 1u), ViewProjection(0u, 2u), ViewProjection(0u, 3u));
		OutSlot.ViewProjectionRow1 = Vector4(ViewProjection(1u, 0u), ViewProjection(1u, 1u), ViewProjection(1u, 2u), ViewProjection(1u, 3u));
		OutSlot.ViewProjectionRow2 = Vector4(ViewProjection(2u, 0u), ViewProjection(2u, 1u), ViewProjection(2u, 2u), ViewProjection(2u, 3u));
		OutSlot.ViewProjectionRow3 = Vector4(ViewProjection(3u, 0u), ViewProjection(3u, 1u), ViewProjection(3u, 2u), ViewProjection(3u, 3u));

		const FLOAT AtlasSize = static_cast<FLOAT>(EMath::Max(InAtlasSize, 1u));
		const FLOAT TileUV = static_cast<FLOAT>(InBuild.Tile.Size) / AtlasSize;
		OutSlot.AtlasTile = Vector4(TileUV, TileUV
			, InBuild.Tile.Rect.TopLeftX / AtlasSize, InBuild.Tile.Rect.TopLeftY / AtlasSize);

		// The valid flag is what a receiver tests before it reads the projection: a slot whose view
		// could not be built keeps the zeroed staging element and must not be sampled with it.
		const FLOAT TileSize = static_cast<FLOAT>(EMath::Max(InBuild.TileSize, 1u));
		OutSlot.ShadowParams = Vector4(InBuild.Request.SplitDistance, 1.f / TileSize
			, static_cast<FLOAT>(InBuild.Request.Index), 1.f);

		// Only a cascade that has a next one blends; the last cascade has nothing to blend into,
		// and a local light's view has no neighbour at all.
		const BOOL32 bBlends = ((InBuild.Request.Kind == EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE)
			&& ((InBuild.Request.Index + 1u) < InBuild.Request.Count)) ? TRUE : FALSE;
		const FLOAT BlendFraction = ((bBlends != FALSE) ? InSettings.CascadeBlendFraction : 0.f);
		const FLOAT WorldTexelSize = ComputeShadowWorldTexelSize(InBuild.Request.Kind
			, InView.GetShadowDomainInfo(), InView.GetWorldLocation(), InBuild.TileSize);
		OutSlot.BiasParams = Vector4(InSettings.ConstantDepthBias, InSettings.SlopeScaledDepthBias
			, InSettings.NormalOffsetTexels * WorldTexelSize, BlendFraction);
	}
	// Appends one light's requests as a block of builds. The block is what keeps the light's views
	// consecutive, which is the property the light list address relies on.
	static void AppendShadowBuilds(const RBaseSceneProxy& InLightProxy, const TArray<FShadowViewRequest>& InRequests
		, TArray<FShadowViewBuild>& OutBuilds, TArray<FShadowLightBlock>& OutBlocks)
	{
		if (InRequests.Num<UINT32>() == 0u)
		{
			return;
		}
		FShadowLightBlock Block;
		Block.StartIndex	= OutBuilds.Num<UINT32>();
		Block.ViewNum		= InRequests.Num<UINT32>();
		for (UINT32 RequestIndex = 0u, RequestNum = InRequests.Num<UINT32>(); RequestIndex < RequestNum; RequestIndex++)
		{
			FShadowViewBuild Build;
			Build.Request		= InRequests[RequestIndex];
			Build.LightProxy	= &InLightProxy;
			Build.LightIdentity	= InLightProxy.GetUniqueID();
			OutBuilds.Add(Build);
		}
		OutBlocks.Add(Block);
	}
	// Inserts a candidate in rank order: coverage descending, then identity ascending. Insertion
	// rather than a sort call because the tie break has to be explicit and stable - two equally
	// covered lights that swap places between frames are a shadow that flickers on and off.
	static void InsertShadowCandidate(TArray<FLocalShadowCandidate>& InOutCandidates, const FLocalShadowCandidate& InCandidate)
	{
		const UINT32 CandidateNum = InOutCandidates.Num<UINT32>();
		UINT32 InsertIndex = CandidateNum;
		for (UINT32 CandidateIndex = 0u; CandidateIndex < CandidateNum; CandidateIndex++)
		{
			const FLocalShadowCandidate& Existing = InOutCandidates[CandidateIndex];
			const BOOL32 bRanksBefore = ((InCandidate.Coverage > Existing.Coverage)
				|| ((InCandidate.Coverage == Existing.Coverage) && (InCandidate.Identity < Existing.Identity))) ? TRUE : FALSE;
			if (bRanksBefore != FALSE)
			{
				InsertIndex = CandidateIndex;
				break;
			}
		}
		InOutCandidates.Add(InCandidate);
		for (UINT32 MoveIndex = CandidateNum; MoveIndex > InsertIndex; MoveIndex--)
		{
			InOutCandidates[MoveIndex] = InOutCandidates[MoveIndex - 1u];
		}
		InOutCandidates[InsertIndex] = InCandidate;
	}
	// Asks a light for its views and appends them. A light that answers FALSE is one the frame
	// does not shadow - disabled, not a caster, or of a type without a shadow projection.
	static void AppendLocalShadowBuilds(const TArray<RLocalLightSceneProxy*>& InCandidates, const RViewProxy& InView
		, TArray<FShadowViewBuild>& OutBuilds, TArray<FShadowLightBlock>& OutBlocks)
	{
		for (UINT32 CandidateIndex = 0u, CandidateNum = InCandidates.Num<UINT32>(); CandidateIndex < CandidateNum; CandidateIndex++)
		{
			RLocalLightSceneProxy* LightProxy = InCandidates[CandidateIndex];
			if (!LightProxy)
			{
				continue;
			}
			TArray<FShadowViewRequest> Requests;
			if ((LightProxy->CollectShadowViews(InView, Requests) == FALSE) || (Requests.Num<UINT32>() == 0u))
			{
				continue;
			}
			AppendShadowBuilds(*LightProxy, Requests, OutBuilds, OutBlocks);
		}
	}
	void RSceneRenderer::SelectLocalShadowCandidates(const RViewProxy& InView
		, TArray<RLocalLightSceneProxy*>& OutSpotCandidates, TArray<RLocalLightSceneProxy*>& OutPointCandidates) const
	{
		OutSpotCandidates.Empty();
		OutPointCandidates.Empty();
		if (!Scene)
		{
			return;
		}
		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		if (Settings->bEnableLocalShadow == FALSE)
		{
			return;
		}

		TArray<FLocalShadowCandidate> SpotCandidates;
		TArray<FLocalShadowCandidate> PointCandidates;
		const EFrustum& ViewFrustum = InView.GetViewFrustum();
		RSceneProxyMapping<RLocalLightSceneProxy>& LocalLights = Scene->GetLocalLightSceneProxies();
		for (UINT32 LightIndex = 0u, LightNum = LocalLights.GetSceneProxyCount(); LightIndex < LightNum; LightIndex++)
		{
			RLocalLightSceneProxy* LightProxy = LocalLights.SceneProxies[LightIndex];
			if (!LightProxy)
			{
				continue;
			}
			const ELightData& LightData = LightProxy->GetLightData();
			if (LightData.CastShadow == FALSE)
			{
				continue;
			}
			// Only the two types with a projection of their own. A directional light is not in this
			// list, and anything else has no shadow view to build.
			const BOOL32 bIsPoint = ((LightData.LightType == ELightType::LIGHT_TYPE_POINT) ? TRUE : FALSE);
			if ((bIsPoint == FALSE) && (LightData.LightType != ELightType::LIGHT_TYPE_SPOT))
			{
				continue;
			}
			// A light whose attenuation sphere misses the camera cannot shadow anything this view
			// sees. Rejecting it here is what keeps it from costing a tile and six caster passes.
			const FLOAT Radius = EMath::Max(LightData.AttenuationRadiusMeters, 1.f);
			if (ViewFrustum.IntersectsSphere(LightProxy->GetWorldLocation(), Radius) == FALSE)
			{
				continue;
			}
			FLocalShadowCandidate Candidate;
			Candidate.LightProxy	= LightProxy;
			Candidate.Identity		= LightProxy->GetUniqueID();
			Candidate.Coverage		= LightProxy->ComputeScreenCoverage(InView);
			if (bIsPoint != FALSE)
			{
				InsertShadowCandidate(PointCandidates, Candidate);
			}
			else
			{
				InsertShadowCandidate(SpotCandidates, Candidate);
			}
		}

		for (UINT32 CandidateIndex = 0u, CandidateNum = SpotCandidates.Num<UINT32>(); CandidateIndex < CandidateNum; CandidateIndex++)
		{
			OutSpotCandidates.Add(SpotCandidates[CandidateIndex].LightProxy);
		}
		for (UINT32 CandidateIndex = 0u, CandidateNum = PointCandidates.Num<UINT32>(); CandidateIndex < CandidateNum; CandidateIndex++)
		{
			OutPointCandidates.Add(PointCandidates[CandidateIndex].LightProxy);
		}
	}
	void RSceneRenderer::InitShadowViews(RViewProxy* InViewProxy)
	{
		if ((!Scene) || (!InViewProxy))
		{
			return;
		}
		// Only a main render view is shaded through the deferred pipeline, so only it has a pass
		// that samples a shadow. A secondary view - an editor camera, a preview - would otherwise
		// pay for a full atlas and a caster pass that nothing in the frame reads.
		if (InViewProxy->IsMainRenderView() == FALSE)
		{
			return;
		}

		const RShadowSettings* Settings = RShadowSettings::GetSingleton();
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		RViewShadowState* ShadowState = EnsureViewShadowState(InViewProxy->GetUniqueID());
		if (!ShadowState)
		{
			return;
		}
		ShadowState->BeginFrame();
		// Marked before anything can fail: a view that is a main view this frame owns this
		// frame's state even if the budget later refuses every tile, and RenderShadowViews reads
		// this to tell "rebuilt and empty" apart from "not rebuilt at all".
		ShadowState->bBuiltThisFrame = TRUE;

		// The requests, in slot order: directional cascades, then spot lights, then point lights,
		// each local group ranked by screen coverage. Slot order is also allocation order, so the
		// fixed budget is spent in the order the design states.
		TArray<FShadowViewBuild> Builds;
		TArray<FShadowLightBlock> Blocks;
		{
			RSceneProxyMapping<RDirectionalLightSceneProxy>& DirectionalLights = Scene->GetDirectionalLightSceneProxies();
			for (UINT32 LightIndex = 0u, LightNum = DirectionalLights.GetSceneProxyCount(); LightIndex < LightNum; LightIndex++)
			{
				RDirectionalLightSceneProxy* LightProxy = DirectionalLights.SceneProxies[LightIndex];
				if (!LightProxy)
				{
					continue;
				}
				TArray<FShadowViewRequest> Requests;
				if ((LightProxy->CollectShadowViews(*InViewProxy, Requests) == FALSE) || (Requests.Num<UINT32>() == 0u))
				{
					continue;
				}
				AppendShadowBuilds(*LightProxy, Requests, Builds, Blocks);
			}
		}
		// The per type caps and the drop count are applied here, where the two numbers are in
		// scope: the candidate lists are ranked but not yet trimmed, so the overflow is exactly
		// the difference between the two.
		UINT32 CappedLocalNum = 0u;
		{
			TArray<RLocalLightSceneProxy*> SpotCandidates;
			TArray<RLocalLightSceneProxy*> PointCandidates;
			SelectLocalShadowCandidates(*InViewProxy, SpotCandidates, PointCandidates);
			const UINT32 SpotNum	= EMath::Min(SpotCandidates.Num<UINT32>(), Settings->LocalSpotShadowMaxCount);
			const UINT32 PointNum	= EMath::Min(PointCandidates.Num<UINT32>(), Settings->LocalPointShadowMaxCount);
			CappedLocalNum			= (SpotCandidates.Num<UINT32>() - SpotNum) + (PointCandidates.Num<UINT32>() - PointNum);

			TArray<RLocalLightSceneProxy*> SelectedSpots;
			TArray<RLocalLightSceneProxy*> SelectedPoints;
			for (UINT32 CandidateIndex = 0u; CandidateIndex < SpotNum; CandidateIndex++)
			{
				SelectedSpots.Add(SpotCandidates[CandidateIndex]);
			}
			for (UINT32 CandidateIndex = 0u; CandidateIndex < PointNum; CandidateIndex++)
			{
				SelectedPoints.Add(PointCandidates[CandidateIndex]);
			}
			AppendLocalShadowBuilds(SelectedSpots, *InViewProxy, Builds, Blocks);
			AppendLocalShadowBuilds(SelectedPoints, *InViewProxy, Builds, Blocks);
		}

		if (Builds.Num<UINT32>() == 0u)
		{
			// Nothing to render this frame. The views of the previous frame are pruned rather than
			// kept: a light that was deleted must not leave its depth behind for a slot to point at.
			ShadowState->PruneShadowViews();
			return;
		}

		const UINT32 AtlasSize = Settings->GetAtlasSize();
		if ((!RenderDevice)
			|| (ShadowState->Atlas.EnsureResources(RenderDevice, AtlasSize, Settings->ShadowDepthBits) == FALSE))
		{
			ReportSkippedDraw(EString(PE_TEXT("the shadow atlas")), EString(PE_TEXT("its depth texture could not be created")));
			ShadowState->PruneShadowViews();
			return;
		}

		// Tiles, one block at a time. A block that cannot be placed in full is dropped in full.
		UINT32 DroppedDirectionalNum	= 0u;
		UINT32 DroppedLocalNum			= 0u;
		for (UINT32 BlockIndex = 0u, BlockNum = Blocks.Num<UINT32>(); BlockIndex < BlockNum; BlockIndex++)
		{
			const FShadowLightBlock& Block = Blocks[BlockIndex];
			BOOL32 bBlockFits = TRUE;
			for (UINT32 ViewIndex = 0u; ViewIndex < Block.ViewNum; ViewIndex++)
			{
				FShadowViewBuild& Build = Builds[Block.StartIndex + ViewIndex];
				if (TryAllocateShadowTile(ShadowState->Atlas, Build.Request.RequestedSize
					, Build.Tile, Build.TileSize) == FALSE)
				{
					bBlockFits = FALSE;
					break;
				}
			}
			if (bBlockFits != FALSE)
			{
				for (UINT32 ViewIndex = 0u; ViewIndex < Block.ViewNum; ViewIndex++)
				{
					Builds[Block.StartIndex + ViewIndex].bIsAllocated = TRUE;
				}
				continue;
			}
			// The tiles this block already placed stay reserved: the shelf has no free, and the only
			// frame that reaches this line is one whose budget is exhausted already. The whole block
			// is dropped so a light's slots stay one contiguous range.
			if (Builds[Block.StartIndex].Request.Kind == EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE)
			{
				DroppedDirectionalNum += Block.ViewNum;
			}
			else
			{
				DroppedLocalNum += Block.ViewNum;
			}
		}

		// The slot table is sized from what the atlas accepted, before any light is told where its
		// views live: a frame whose table cannot be created must hand every light the "no shadow"
		// address rather than an index into a buffer that does not exist.
		UINT32 TotalSlotNum = 1u;
		for (UINT32 BlockIndex = 0u, BlockNum = Blocks.Num<UINT32>(); BlockIndex < BlockNum; BlockIndex++)
		{
			if (Builds[Blocks[BlockIndex].StartIndex].bIsAllocated != FALSE)
			{
				TotalSlotNum += Blocks[BlockIndex].ViewNum;
			}
		}
		if ((TotalSlotNum > 1u) && (ShadowState->EnsureSlotBuffer(RenderDevice, TotalSlotNum) == FALSE))
		{
			ReportSkippedDraw(EString(PE_TEXT("the shadow slot table")), EString(PE_TEXT("its structured buffer could not be created")));
			ShadowState->SlotNum = 0u;
			ShadowState->PruneShadowViews();
			return;
		}

		// The views and the slot table. Slot zero is the "casts no shadow" sentinel, so the first
		// real slot is one; a block only consumes slots when its tiles were placed.
		ShadowState->SlotStaging.AddDefaultGetRef();
		UINT32 NextSlot = 1u;
		// This view's address table. Filled as the slots are handed out and published at the end,
		// so a view that cannot get a usable atlas leaves no half built table behind: an absent
		// table is what makes its light list say "casts no shadow" everywhere.
		RShadowLightAddressMapType LightAddresses;
		for (UINT32 BlockIndex = 0u, BlockNum = Blocks.Num<UINT32>(); BlockIndex < BlockNum; BlockIndex++)
		{
			const FShadowLightBlock& Block = Blocks[BlockIndex];
			if (Builds[Block.StartIndex].bIsAllocated == FALSE)
			{
				continue;
			}
			FShadowLightAddress ShadowAddress;
			ShadowAddress.FirstSlot	= NextSlot;
			ShadowAddress.ViewNum	= Block.ViewNum;
			for (UINT32 ViewIndex = 0u; ViewIndex < Block.ViewNum; ViewIndex++)
			{
				FShadowViewBuild& Build = Builds[Block.StartIndex + ViewIndex];
				EShadowSlotGPU& Slot = ShadowState->SlotStaging.AddDefaultGetRef();

				const UINT64 ViewKey = MakeShadowViewKey(Build.LightIdentity, Build.Request.Kind, Build.Request.Index);
				RShadowViewProxy* ShadowView = ShadowState->FindShadowView(ViewKey);
				if (!ShadowView)
				{
					ShadowView = new RShadowViewProxy();
					ShadowView->SetShadowViewKey(ViewKey);
					ShadowState->AddShadowView(ShadowView);
				}
				EViewDomainInfo DomainInfo;
				Vector3 ViewAnchor;
				Quaternion ViewRotation;
				if ((!Build.LightProxy)
					|| (Build.LightProxy->BuildShadowViewDomain(*InViewProxy, Build.Request, Build.TileSize
						, SceneCasterBounds, bSceneCasterBoundsComplete
						, DomainInfo, ViewAnchor, ViewRotation) == FALSE))
				{
					// The slot was already counted, so it keeps its zeroed element and reads as
					// invalid; renumbering here would move every later light's address.
					ShadowView->ClearUsed();
					NextSlot += 1u;
					continue;
				}
				// The domain's viewport is the atlas tile, filled by the renderer rather than the
				// light: the light knows the projection, the atlas knows where it may be written.
				DomainInfo.RenderViewport = Build.Tile.Rect;
				ShadowView->SetupShadowView(Build.Request.Kind, Build.LightProxy, NextSlot
					, Build.Request.Index, Build.Request.Count, DomainInfo, ViewAnchor, ViewRotation);
				// After SetupShadowView, which resets the list: the cull fills the list this
				// view's draws will read. A point face the camera cannot reach keeps that list
				// empty: the slot is still valid with a cleared tile, so a receiver that somehow
				// selected it reads "lit" rather than a stale caster.
				const BOOL32 bFaceReachesCamera = ((Build.Request.Kind != EShadowViewKindType::SHADOW_VIEW_KIND_POINT_FACE)
					|| (Build.LightProxy == nullptr)
					|| (DoesViewFrustumReachPointFace(*InViewProxy, Build.LightProxy->GetWorldLocation(), DomainInfo) != FALSE)) ? TRUE : FALSE;
				if (bFaceReachesCamera != FALSE)
				{
					// The list is filled here, once for this view, instead of being scanned
					// again by the pass that records its draws. Only a directional cascade asks
					// for the drawn bounds: an orthographic light is the one whose tile can be
					// fitted to the casters without changing which shadows reach the slice.
					const BOOL32 bIsDirectionalCascade =
						((Build.Request.Kind == EShadowViewKindType::SHADOW_VIEW_KIND_DIRECTIONAL_CASCADE) ? TRUE : FALSE);
					RViewPrimitiveBounds DrawnBounds;
					CollectViewPrimitiveList(*ShadowView, ShadowView->GetPrimitiveList(), &DrawnBounds);
					if ((bIsDirectionalCascade != FALSE) && (DrawnBounds.Bounds.IsValid) && (DrawnBounds.bComplete))
					{
						// The conservative box reached every caster; the cull kept the ones that
						// can reach this slice. Fitting the projection to them spends the tile on
						// casters instead of on empty space. The view keeps the domain otherwise:
						// an incomplete union could clip a primitive the cull deliberately kept.
						if (Build.LightProxy->RefitShadowViewDomain(*InViewProxy, Build.Request, Build.TileSize
							, DrawnBounds.Bounds, DomainInfo) != FALSE)
						{
							ShadowView->UpdateShadowViewDomain(DomainInfo);
						}
					}
					SortViewPrimitiveListByShaderInstance(ShadowView->GetPrimitiveList());
				}
				FillShadowSlot(*ShadowView, Build, AtlasSize, *Settings, Slot);
				NextSlot += 1u;
			}
			LightAddresses.Add(Builds[Block.StartIndex].LightIdentity, ShadowAddress);
		}
		ViewShadowAddresses.Add(InViewProxy->GetUniqueID(), LightAddresses);
		ShadowState->SlotNum = NextSlot;
		if (NextSlot > 1u)
		{
			ShadowState->UploadSlots(RenderDevice);
		}

		const UINT32 DroppedViewNum = DroppedDirectionalNum + DroppedLocalNum;
		ShadowState->DroppedShadowNum = DroppedViewNum;
		if (((DroppedViewNum > 0u) || (CappedLocalNum > 0u)) && (bShadowDropReported == FALSE))
		{
			// Once per session rather than once per frame: a scene that is permanently over budget
			// would otherwise write this line sixty times a second, which buries the first one.
			bShadowDropReported = TRUE;
			PE_LOG_WARN(EString(PE_TEXT("[SceneRenderer] Shadows were dropped this frame: "))
				+ ToString(DroppedViewNum) + EString(PE_TEXT(" view(s) over budget (directional "))
				+ ToString(DroppedDirectionalNum) + EString(PE_TEXT(", local "))
				+ ToString(DroppedLocalNum) + EString(PE_TEXT(")) and "))
				+ ToString(CappedLocalNum) + EString(PE_TEXT(" local light(s) over the per type caps.")));
		}

		ShadowState->PruneShadowViews();
	}
	RViewShadowState* RSceneRenderer::EnsureViewShadowState(const ObjectIdentityType InViewIdentity)
	{
		if (!ViewShadowStates.ContainsKey(InViewIdentity))
		{
			ViewShadowStates.Add(InViewIdentity, nullptr);
		}
		RViewShadowState* ShadowState = ViewShadowStates[InViewIdentity];
		if (!ShadowState)
		{
			ShadowState = new RViewShadowState();
			ViewShadowStates[InViewIdentity] = ShadowState;
		}
		return ShadowState;
	}
	void RSceneRenderer::PruneViewShadowStates()
	{
		// Collected first, because a map cannot be modified while it is being walked.
		TArray<ObjectIdentityType> StaleViewIdentities;
		for (RViewShadowStateMapType::TConstIterator StateIt = ViewShadowStates.Begin(); StateIt != ViewShadowStates.End(); StateIt++)
		{
			if (FindViewProxy(StateIt->first) == nullptr)
			{
				StaleViewIdentities.Add(StateIt->first);
			}
		}
		if (StaleViewIdentities.Num<UINT32>() == 0u)
		{
			return;
		}
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		for (UINT32 IdentityIndex = 0u, IdentityNum = StaleViewIdentities.Num<UINT32>(); IdentityIndex < IdentityNum; IdentityIndex++)
		{
			RViewShadowState* ShadowState = ViewShadowStates[StaleViewIdentities[IdentityIndex]];
			if (ShadowState)
			{
				ShadowState->Release(RenderDevice);
				delete ShadowState;
			}
			ViewShadowStates.Remove(StaleViewIdentities[IdentityIndex]);
			// The view's address table dies with its state: an address is an index into that
			// state's slot table, and keeping it for a view that is gone would let a later view
			// with a recycled identity read another view's slots.
			ViewShadowAddresses.Remove(StaleViewIdentities[IdentityIndex]);
			// Its light list buffers go with it, for the same reason.
			RViewLightBuffers* ViewBuffers = ViewLightBuffers[StaleViewIdentities[IdentityIndex]];
			if (ViewBuffers)
			{
				ViewBuffers->DirectionalLightBuffer.ReleaseRenderResource();
				ViewBuffers->LocalLightBuffer.ReleaseRenderResource();
				delete ViewBuffers;
			}
			ViewLightBuffers.Remove(StaleViewIdentities[IdentityIndex]);
		}
	}
	void RSceneRenderer::ReleaseViewShadowStates()
	{
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		for (RViewShadowStateMapType::TIterator StateIt = ViewShadowStates.Begin(); StateIt != ViewShadowStates.End(); StateIt++)
		{
			RViewShadowState* ShadowState = StateIt->second;
			if (ShadowState)
			{
				ShadowState->Release(RenderDevice);
				delete ShadowState;
			}
		}
		ViewShadowStates.Empty();
		ViewShadowAddresses.Empty();
		for (RViewLightBuffersMapType::TIterator BufferIt = ViewLightBuffers.Begin(); BufferIt != ViewLightBuffers.End(); BufferIt++)
		{
			RViewLightBuffers* ViewBuffers = BufferIt->second;
			if (ViewBuffers)
			{
				ViewBuffers->DirectionalLightBuffer.ReleaseRenderResource();
				ViewBuffers->LocalLightBuffer.ReleaseRenderResource();
				delete ViewBuffers;
			}
		}
		ViewLightBuffers.Empty();
		// A renderer that comes back up has allocated nothing yet, so the first drop is worth
		// reporting again rather than being swallowed by the previous session's report.
		bShadowDropReported = FALSE;
	}
	void RSceneRenderer::InitRendererSettings()
	{
		NeedStencil = IsNeedStencil();
	}
	BOOL32 RSceneRenderer::IsNeedStencil() const
	{
		//TODO
		return FALSE;
	}

};
