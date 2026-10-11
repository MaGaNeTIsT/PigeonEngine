#include "SkyLightProxy.h"
#include <ShaderAsset/ShaderAsset.h>
#include <ShaderAsset/ShaderInstance.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/EngineShaderInstanceCache.h>
#include <ShaderAsset/ShaderJobBuilder.h>
#include <MeshAsset/MeshAsset.h>
#include <TextureAsset/TextureAsset.h>
#include <RHI/IRCommandList.h>
#include <Pipeline/RPipelineStateCache.h>
#include <Renderer/SceneRenderer.h>
#include <Renderer/ScenePass.h>
#include <Main/MainManager.h>
#include "MeshSceneProxy.h"
#include <Development/Profiler.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RSkyLightSceneProxy, RBaseSceneProxy>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	const EShaderProgram*		RSkyLightSceneProxy::SkyProgram	= nullptr;

	/*
	* The engine sky shader. Shared process wide through the instance cache; the proxy keeps only
	* the resolved program, so no sky light carries per instance state.
	*
	* The request is built once: resolving it is pure computation, but the two callers below have
	* to name the same assembly, and one shared request is what keeps them from drifting apart.
	*/
	static const EShaderAssemblyRequest& GetSkyShaderRequest()
	{
		static const EShaderAssemblyRequest Request = []() -> EShaderAssemblyRequest
		{
			// The sky pass states its own source: the Sky program, no material (it shades
			// nothing), the Unlit domain, the fullscreen vertex axis and both stages.
			EShaderAssemblyRequest Built;
			Built.ShaderName = EName(PE_TEXT("M_EngineSkyLight"));
			Built.ProgramName = EName(PE_TEXT("Sky"));
			Built.DomainName = EString(PE_TEXT("Unlit"));
			Built.VertexFactory = EShaderVertexFactoryType::FULLSCREEN;
			Built.Profile.StreamProfile = SHADER_VERTEX_ATTRIBUTE_UV0;
			Built.StageMask = ENGINE_SHADER_STAGE_MASK_GRAPHICS;
			return Built;
		}();
		return Request;
	}
	// Load thread: resolves the sky shader for the caller that creates a sky proxy.
	static const EShaderInstance* GetSkyShaderInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->FindOrCreate(GetSkyShaderRequest());
	}
	// Any thread: the same instance, asked for rather than resolved - null while it is not
	// assembled yet, and a cache hit once it is. This is what the reload refresh uses, because it
	// runs on the render thread where a load may not happen.
	static const EShaderInstance* RequireSkyShaderInstance()
	{
		return REngineShaderInstanceCache::GetManagerSingleton()->RequireInstance(GetSkyShaderRequest());
	}

	void RSkyLightSceneProxy::CreateSkyAssets()
	{
		// Only the program: the geometry is the engine's fullscreen triangle, which the shared
		// draw path owns, so a sky light carries no mesh of its own any more.
		//
		// Re-derived on every construction rather than only when the cached pointer is empty: the
		// static outlives one engine lifetime inside a process that restarts the engine, and a
		// program left over from the previous lifetime is a pointer into released assets.
		const EShaderInstance* SkyInstance = GetSkyShaderInstance();
		RSkyLightSceneProxy::SkyProgram = (SkyInstance ? SkyInstance->GetProgram() : nullptr);
	}

	void RSkyLightSceneProxy::OnShaderAssetReplaced()
	{
		// The instance is the stable handle and was re-pointed to the new object before this ran,
		// so re-reading its program is what makes the process wide pointer current. The proxies
		// themselves drop what they built from the previous program - each one notices the swap by
		// comparing its own PipelineStateProgram against this pointer.
		const EShaderInstance* SkyInstance = RequireSkyShaderInstance();
		RSkyLightSceneProxy::SkyProgram = (SkyInstance ? SkyInstance->GetProgram() : nullptr);
	}

	void RSkyLightSceneProxy::DropStateForReplacedProgram()
	{
		if (PipelineStateProgram == RSkyLightSceneProxy::SkyProgram)
		{
			return;
		}
		// The pipeline state carries the root signature of the program it was created for, and the
		// parameter block had its layout read from that program's reflection. Both are re-derived
		// from the current program by their next user, so dropping is all this has to do.
		PipelineState = nullptr;
		PipelineStateProgram = nullptr;
		SkyLightParameterBlock.ReleaseRenderResources();
	}

	RSkyLightSceneProxy::RSkyLightSceneProxy(PSkyLightComponent* InComponent)
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f)
		, PipelineState(nullptr), Component(InComponent)
	{
		CreateSkyAssets();
		PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Create directional light scene proxy failed"), (!!Component));
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy()
		: TextureCubeAsset(nullptr), AdjustColor(Color3::White()), AdjustIntensity(1.f)
		, PipelineState(nullptr), Component(nullptr)
	{
		CreateSkyAssets();
	}
	RSkyLightSceneProxy::RSkyLightSceneProxy(const RSkyLightSceneProxy& Other)
		: RBaseSceneProxy(Other), TextureCubeAsset(Other.TextureCubeAsset), AdjustColor(Other.AdjustColor)
		, AdjustIntensity(Other.AdjustIntensity), PipelineState(Other.PipelineState), Component(Other.Component)
	{
		CreateSkyAssets();
	}
	RSkyLightSceneProxy::~RSkyLightSceneProxy()
	{
	}
	void RSkyLightSceneProxy::SetupProxy(const ERenderSkyLightMatrices& InMatrices, const ERenderSkyLightParams& InSkyLightParams, const ETextureCubeAsset* InTextureCubeAsset)
	{
		UpdateTextureCubeAsset(InTextureCubeAsset);
		UpdateSkyLightParams(InSkyLightParams);
		UpdateMatrices(InMatrices);

		UpdateRenderResource();
	}
	void RSkyLightSceneProxy::UpdateTextureCubeAsset(const ETextureCubeAsset* InTextureCubeAsset)
	{
		TextureCubeAsset = InTextureCubeAsset;
	}
	const IRRHITexture* RSkyLightSceneProxy::GetEnvironmentTexture() const noexcept
	{
		if (!TextureCubeAsset)
		{
			return nullptr;
		}
		const RTextureCubeResource* CubeRenderResource = TextureCubeAsset->GetRenderResource();
		if (!CubeRenderResource)
		{
			return nullptr;
		}
		return CubeRenderResource->GetResolvedRHITexture();
	}
	const Color3& RSkyLightSceneProxy::GetAdjustColor() const noexcept
	{
		return AdjustColor;
	}
	FLOAT RSkyLightSceneProxy::GetAdjustIntensity() const noexcept
	{
		return AdjustIntensity;
	}
	void RSkyLightSceneProxy::UpdateSkyLightParams(const ERenderSkyLightParams& InSkyLightParams)
	{
		AdjustColor		= InSkyLightParams.LightAdjustColor;
		AdjustIntensity	= InSkyLightParams.LightAdjustIntensity;
	}
	void RSkyLightSceneProxy::UpdateMatrices(const ERenderSkyLightMatrices& InMatrices)
	{
		SetProxyWorldTransform(InMatrices.WorldLocation, InMatrices.WorldRotation, InMatrices.WorldScaling);
	}
	void RSkyLightSceneProxy::UpdateRenderResource()
	{
		// Before anything reads the program: a replaced one leaves the block below holding a layout
		// that came from it.
		DropStateForReplacedProgram();

		// The values the shader reads go through the block the sky program's own source declares,
		// so they land on the register the plan reports rather than on a slot written down here.
		IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
		if ((!SkyProgram) || (!RenderDevice))
		{
			return;
		}
		if (!SkyLightParameterBlock.EnsureInitialized(RenderDevice, *SkyProgram))
		{
			return;
		}
		const EName BlockName(SHADER_BLOCK_SKY_LIGHT_PARAMS);
		EShaderParameterSet& Parameters = SkyLightParameterBlock.GetParameters();
		WriteShaderVector4(Parameters, BlockName, EName(SHADER_FIELD_LIGHT_ADJUST_COLOR)
			, Vector4(AdjustColor.r, AdjustColor.g, AdjustColor.b, 1.f));
		WriteShaderScalar(Parameters, BlockName, EName(SHADER_FIELD_LIGHT_ADJUST_INTENSITY), AdjustIntensity);
		SkyLightParameterBlock.Upload(RenderDevice);
	}
	BOOL32 RSkyLightSceneProxy::IsRenderValid()const
	{
		// No mesh in this test any more: the geometry is the engine's fullscreen triangle.
		//
		// Each term says why it failed before returning, because this is the predicate RenderSky
		// pauses on: "render invalid" on its own does not say whether the program, one of its
		// stages, the cube asset or the cube's device object is what is missing - and on a back-end
		// whose shaders have not been built yet it is the first of them. The report is deduplicated
		// per reason, so a sky that stays broken costs one line rather than one per frame.
#if _EDITOR_ONLY
		if (!RSkyLightSceneProxy::SkyProgram)
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its sky program is not resolved")));
			return FALSE;
		}
		if (!ResolveShaderStageResource(*RSkyLightSceneProxy::SkyProgram, EShaderStageType::SHADER_STAGE_VERTEX))
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("the vertex stage of its sky program has no render resource")));
			return FALSE;
		}
		if (!ResolveShaderStageResource(*RSkyLightSceneProxy::SkyProgram, EShaderStageType::SHADER_STAGE_PIXEL))
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("the pixel stage of its sky program has no render resource")));
			return FALSE;
		}
#endif
		if (!TextureCubeAsset)
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("it has no environment cube")));
			return FALSE;
		}
		if (!TextureCubeAsset->IsRenderResourceValid())
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its environment cube has no render resource")));
			return FALSE;
		}
		if (AdjustIntensity < 0.001f)
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its adjust intensity is zero")));
			return FALSE;
		}
		return TRUE;
	}
	void RSkyLightSceneProxy::BindRenderResource()const
	{
		// Everything the sky binds is bound inside Draw, right after the pipeline state that
		// would otherwise void it. The hook stays because the renderer calls it first.
	}
	BOOL32 RSkyLightSceneProxy::EnsurePipelineState()
	{
		/*
		* A program that is missing is asked for again, which is what makes a sky whose first
		* resolution failed recover instead of staying without one for the session.
		*
		* The first resolution runs where the proxy is created, and that call is the load thread's
		* synchronous form: if the sky shader could not be assembled then, nothing else would ever
		* ask again, because the reload refresh only runs when a shader asset is replaced.
		* RequireInstance is the any-thread form - it answers from the cache and otherwise records
		* one request for the load thread, which is what every draw path does with a material that
		* is not assembled yet. The load manager deduplicates the request, so asking every frame
		* costs a table lookup for as long as the state lasts.
		*
		* It goes before the drop below on purpose: a program that arrives here is a different one
		* than the pipeline state was built from, and the drop is what notices that.
		*/
		if (!SkyProgram)
		{
			const EShaderInstance* SkyInstance = RequireSkyShaderInstance();
			if (SkyInstance)
			{
				RSkyLightSceneProxy::SkyProgram = SkyInstance->GetProgram();
			}
		}
		DropStateForReplacedProgram();
		if (PipelineState)
		{
			return TRUE;
		}
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		if ((!Renderer) || (!SkyProgram))
		{
			return FALSE;
		}
		// The pass the sky draws in is engine driven, so it carries no shaders of its own:
		// the fixed function state comes from the pass, the two stages and the roots they
		// read come from the program. The cache keys the result on both, which is what lets
		// every sky light in the frame share one state.
		RGraphicsPipelineStateDesc Desc;
		FRenderPassRegistry::FillPipelineStateDesc(
			FRenderPassRegistry::GetDesc(ERenderPassType::RENDER_PASS_TYPE_SKY), SkyProgram, Desc);
		PipelineState = Renderer->GetPipelineStateCache().GetOrCreateGraphics(Desc);
		PipelineStateProgram = (PipelineState ? SkyProgram : nullptr);
		if (PipelineState)
		{
			// The drop above took the parameter block with the state it was derived beside, and
			// nothing else is coming to rebuild it: a component update is not sent when only the
			// program moved, and the cube's arrival sends a texture update, which does not refresh
			// the render resources either. A block that is never rebuilt binds nothing at all -
			// not zeroes - so the sky's own values would read as zero and the pass would multiply
			// its radiance away, which is why the draw path rebuilds what it dropped here.
			UpdateRenderResource();
		}
		return (PipelineState != nullptr) ? TRUE : FALSE;
	}
	void RSkyLightSceneProxy::Draw()
	{
		IRCommandList* CmdList = EMainManager::GetManagerSingleton()->GetActiveCommandList();
		if (!CmdList) { return; }
		if (!EnsurePipelineState())
		{
			// The sky has one draw and no substitute: a state that cannot be built means the pass
			// renders nothing, which reads as a missing sky rather than as a broken shader. It is
			// expected for the first frames of a cold start, while the program is still assembling.
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its pipeline state could not be built for program '"))
				+ ((SkyProgram) ? SkyProgram->GetShaderName().ToString() : EString(PE_TEXT("<none>")))
				+ EString(PE_TEXT("'")));
			return;
		}

		PE_GPU_EVENT_SCOPE(CmdList, EString(PE_TEXT("SkyLight")));

		// Order matters: the pipeline state voids every root binding made before it, so it goes
		// first, then the blocks the shader reads, and the geometry last.
		CmdList->SetPipelineState(PipelineState);

		// The shared blocks belong to this draw: the state above switched the signature, which
		// voided whatever the pass before had bound.
		RSceneRenderer* Renderer = EMainManager::GetManagerSingleton()->GetSceneRenderer();
		if (Renderer)
		{
			Renderer->BindEngineParameters(CmdList, *SkyProgram);
		}

		// The sky's own block is bound here rather than by the renderer because it belongs to
		// this pass and no other. Same rule as the shared ones: after the pipeline state, which
		// would have voided an earlier binding.
		SkyLightParameterBlock.Bind(CmdList, *SkyProgram);

		BindEnvironmentCube(CmdList);

		// The geometry is the fullscreen triangle: the program's vertex stage is the fullscreen
		// factory, which builds its clip position from the triangle's UV coordinate and reads no
		// mesh stream. It sits on the far plane (see Sky_VS.tmpl.hlsl), so the depth test against
		// the base pass's depth buffer is what keeps the sky behind everything else.
		FullScreenTriangle.BindPrimitiveBuffers(CmdList, *SkyProgram);
		CmdList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);
	}
	void RSkyLightSceneProxy::BindEnvironmentCube(IRCommandList* InCommandList)const
	{
		if ((!InCommandList) || (!SkyProgram))
		{
			return;
		}
		if (!TextureCubeAsset)
		{
			// Quiet: the cube is asked for when the sky light appears and may simply not be there
			// yet, which is what every sky looks like for its first frames.
			return;
		}
		const RTextureCubeResource* CubeRenderResource = TextureCubeAsset->GetRenderResource();
		if (!CubeRenderResource)
		{
			// Not quiet, for the same reason the mesh path is not: the asset is held and its device
			// side is missing, so the sky draws without its cube rather than waiting for one.
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its environment cube has no render resource")));
			return;
		}
		const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*SkyProgram, EShaderStageType::SHADER_STAGE_PIXEL);
		if (!PixelEntry)
		{
			ReportSkippedDraw(EString(PE_TEXT("the sky dome")), EString(PE_TEXT("its program has no pixel stage to bind the environment cube through")));
			return;
		}
		// The environment cube is a reflected resource of the pixel stage (the lighting
		// component declares it), so it is bound through that stage's own plan and lands on
		// the register the compiler assigned rather than on a literal slot.
		EShaderParameterSet Parameters;
		Parameters.SetTexture(EName(), EName(PE_TEXT("EnvironmentCube")), CubeRenderResource->GetResolvedRHITexture());
		BindShaderParameterResources(InCommandList, Parameters, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL);
	}
};
