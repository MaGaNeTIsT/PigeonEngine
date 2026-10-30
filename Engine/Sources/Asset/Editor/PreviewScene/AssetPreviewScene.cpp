#include "AssetPreviewScene.h"

#if _EDITOR_ONLY

#include <Animation/AnimationSampler.h>

#include <cmath>

#include <AnimationAsset/AnimationAsset.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/World/PreviewWorld.h>
#include <Main/MainManager.h>
#include <Development/GPUProfiler.h>
#include <RHI/IRRHIDevice.h>
#include <RHI/IRCommandList.h>
#include <RenderCore/Renderer/SceneRenderer.h>
#include <RenderCore/RenderProxy/ViewProxy.h>
#include <ShaderAsset/ShaderAsset.h>
#include <Shader/ShaderAssetBinding.h>
#include <Shader/ShaderParameter.h>
#include <imGUI/Headers/imGUIManager.h>

namespace PigeonEngine
{
	namespace
	{
		// Skeletal assets are authored Y-up; the preview turns them into the engine's Z-up
		// world, matching what the runtime skeletal mesh actors do.
		PE_CONSTEXPR_STATIC const FLOAT PreviewSkeletalMeshPitchDegrees = 90.0f;
		// Framing: the distance is a multiple of the largest world extent, so every asset fills
		// the same portion of the viewport regardless of its authored scale. The camera's clip
		// planes scale with that distance as well: the world default near plane (5) is far
		// outside a metre-scale character and would clip it completely.
		PE_CONSTEXPR_STATIC const FLOAT PreviewCameraDistanceExtentScale = 3.0f;
		PE_CONSTEXPR_STATIC const FLOAT PreviewCameraNearRatio = 0.01f;
		PE_CONSTEXPR_STATIC const FLOAT PreviewCameraFarRatio = 10.0f;
		PE_CONSTEXPR_STATIC const FLOAT PreviewCameraHeightScale = 0.35f;

		class RAssetPreviewSceneRenderer final : public RSceneRenderer
		{
		public:
			// Records the preview's frame on the render thread.
			//
			// The main renderer owns the window, the final blit, the ImGui overlay and the
			// frame's GPU profiling slice; this renderer owns one offscreen view. So it repeats
			// the parts of RSceneRenderer::Render that build a view - drain the scene commands,
			// build the targets, bind the frame wide state, record the base pass - and
			// deliberately records none of the parts that belong to the window.
			void RenderPreview_RenderThread()
			{
				IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
				if (!RenderDevice)
				{
					return;
				}

				CurrentCommandList = RenderDevice->AcquireCommandList();
				EMainManager::GetManagerSingleton()->SetActiveCommandList(CurrentCommandList);
				// The primitives this pass draws reach the engine blocks through whichever
				// renderer publishes itself here, and they must reach this one: their
				// BindEngineParameters binds the view of the renderer it finds, and this pass
				// is the only place the preview's view is current. Falling back to the main
				// renderer would read its current view, which is null until the main scene
				// records - so the preview's ViewParams would never be bound, every vertex
				// would collapse into one point, and the panel would be black with the
				// geometry drawing all along.
				EMainManager::GetManagerSingleton()->SetActiveSceneRenderer(this);

				// The frame's timing slice was opened and closed by the main renderer, so this
				// renderer's passes are not part of it and are dropped instead of charged to it.
				EGPUProfiler::GetManagerSingleton()->SetPassRecordingEnabled(FALSE);

				// Publishes this frame's commands, culls against the preview camera, builds the
				// per-view targets. A viewport change is picked up here.
				InitNewFrame();
				EnsureEnginePassPipelineStates();

				BindSamplerTable(CurrentCommandList);
				// A preview world has no sky, which is exactly what this publishes: no
				// environment, so the indirect term contributes nothing.
				RenderEnvironmentSH();
				UpdateGlobalParameters();
				BasePass();

				EGPUProfiler::GetManagerSingleton()->SetPassRecordingEnabled(TRUE);

				// The base pass leaves the scene colour as a render target. The display copy
				// reads it, so it has to be readable first - the main renderer makes the same
				// transition in its final output pass.
				if (RViewProxy* PreviewView = FindViewProxy(FinalOutputView))
				{
					RSceneTextures** SceneTextures = ViewSceneTextures.FindValueAsPtr(PreviewView->GetUniqueID());
					if (SceneTextures && (*SceneTextures))
					{
						TransitionToShaderReadable((*SceneTextures)->SceneColor.GetResolvedRHITexture());
					}
				}

				// Scene colour is scene referred and its alpha is not a display alpha: the base
				// pass writes the emissive with alpha 0 and the lighting pass preserves the
				// destination alpha. The engine's final output pass is what turns that into
				// something an image can show - exposure, tone mapping, alpha 1 - so the preview
				// runs it into an offscreen target instead of into the window.
				RenderDisplayTarget();

				// Resolved once per recorded frame and read by the editor on the next one.
				UpdateTextureId();

				RenderDevice->SubmitCommandList(CurrentCommandList);
				CurrentCommandList = nullptr;
				EMainManager::GetManagerSingleton()->SetActiveCommandList(nullptr);
				EMainManager::GetManagerSingleton()->SetActiveSceneRenderer(nullptr);
			}

			ImTextureID GetTextureId() const
			{
				return PreviewTextureId;
			}

		private:
			// The renderer wide sampler table, bound once per frame for the same reason the main
			// renderer binds it there: the register is the array index and no draw changes it.
			// This is the one piece of state the two renderers must agree on, because their
			// shaders are the same shaders.
			void BindSamplerTable(IRCommandList* InCommandList)
			{
				for (UINT32 SamplerIndex = 0u; SamplerIndex < RSamplerType::SAMPLER_TYPE_COUNT; SamplerIndex++)
				{
					IRRHISampler* SamplerPtr = Samplers[SamplerIndex].RHISampler
						? Samplers[SamplerIndex].RHISampler
						: static_cast<IRRHISampler*>(&Samplers[SamplerIndex]);
					InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_VERTEX, SamplerIndex, SamplerPtr);
					InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_PIXEL, SamplerIndex, SamplerPtr);
					InCommandList->SetSampler(ERHIShaderStage::RHI_SHADER_STAGE_COMPUTE, SamplerIndex, SamplerPtr);
				}
			}

			// The display-referred copy of the view: what ImGui can actually show.
			//
			// This is the engine's final output pass, recorded into an offscreen target instead
			// of into the window: the same program (Copy), the same pipeline state, the same view
			// exposure block, and therefore the same image the main window shows. It is not
			// optional for a preview - scene colour is scene referred (cd/m2, 16F) and its alpha
			// is not a display alpha, so handing it to ImGui directly is what made the panel
			// look black while the geometry was drawing all along.
			void RenderDisplayTarget()
			{
				IRRHIDevice* RenderDevice = EMainManager::GetManagerSingleton()->GetRenderDevice();
				IRRHIPipelineState* DisplayState = PipelineStates[PIPELINE_STATE_FINAL_OUTPUT];
				if ((!RenderDevice) || (!DisplayState) || (!FullScreenProgram))
				{
					return;
				}

				RViewProxy* PreviewView = FindViewProxy(FinalOutputView);
				if (!PreviewView)
				{
					return;
				}
				RSceneTextures** SceneTextures = ViewSceneTextures.FindValueAsPtr(PreviewView->GetUniqueID());
				if ((!SceneTextures) || (!(*SceneTextures)) || (!(*SceneTextures)->SceneColor.IsRenderResourceValid()))
				{
					return;
				}

				const EViewport& Viewport = PreviewView->GetRenderViewport();
				const UINT32 DisplayWidth = static_cast<UINT32>(EMath::Max(Viewport.Width, 2.f));
				const UINT32 DisplayHeight = static_cast<UINT32>(EMath::Max(Viewport.Height, 2.f));
				if ((PreviewDisplay.IsRenderResourceValid() == FALSE)
					|| (PreviewDisplaySize.x != static_cast<INT32>(DisplayWidth))
					|| (PreviewDisplaySize.y != static_cast<INT32>(DisplayHeight)))
				{
					PreviewDisplay.ReleaseRenderResource();
					if (PreviewDisplay.InitFromRHI(RenderDevice,
						RTextureDesc(DisplayWidth, DisplayHeight,
							RBindFlagType::BIND_RENDER_TARGET | RBindFlagType::BIND_SHADER_RESOURCE,
							RFormatType::FORMAT_R8G8B8A8_UNORM)) == FALSE)
					{
						return;
					}
					PreviewDisplaySize = Vector2Int(static_cast<INT32>(DisplayWidth), static_cast<INT32>(DisplayHeight));
				}

				// The pass table's final output row states R8G8B8A8_UNORM, so the target above is
				// created with exactly that format and the pipeline state's render target matches.
				RRHIRenderPassDesc Pass;
				Pass.RenderTargetCount				= 1u;
				Pass.RenderTargets[0].Target		= PreviewDisplay.GetResolvedRHITexture();
				Pass.RenderTargets[0].LoadOp		= ERHILoadOp::RHI_LOAD_OP_DONT_CARE;
				Pass.RenderTargets[0].StoreOp		= ERHIStoreOp::RHI_STORE_OP_STORE;
				Pass.RenderTargets[0].ClearColor	= Color4(0.f, 0.f, 0.f, 1.f);
				Pass.DepthStencil.Target			= nullptr;
				Pass.PassName						= PE_TEXT("PreviewDisplay");
				CurrentCommandList->BeginRenderPass(Pass);

				// D3D12 has no implicit viewport or scissor from the render targets, and for a
				// full target the two are simply its size.
				const EViewport DisplayViewport(0.f, 0.f, static_cast<FLOAT>(DisplayWidth), static_cast<FLOAT>(DisplayHeight), 0.f, 1.f);
				CurrentCommandList->SetViewports(&DisplayViewport, 1u);
				const ERect DisplayScissor(0.f, 0.f, static_cast<FLOAT>(DisplayWidth), static_cast<FLOAT>(DisplayHeight));
				CurrentCommandList->SetScissorRects(&DisplayScissor, 1u);

				CurrentCommandList->SetPipelineState(DisplayState);

				// The copy program reads the view's exposure, so the view block is published and
				// bound after the state - the same order the main renderer uses, because switching
				// a signature voids the roots.
				CurrentViewProxy = PreviewView;
				CurrentViewProxy->UpdateRenderResource();
				BindEngineParameters(CurrentCommandList, *FullScreenProgram);
				FullScreenTriangle.BindPrimitiveBuffers(CurrentCommandList, *FullScreenProgram);

				const EShaderStageEntry* PixelEntry = ResolveShaderStageEntry(*FullScreenProgram, EShaderStageType::SHADER_STAGE_PIXEL);
				if (PixelEntry)
				{
					EShaderParameterSet Parameters;
					Parameters.SetTexture(EName(), EName(PE_TEXT("CopySourceTexture")), (*SceneTextures)->SceneColor.GetResolvedRHITexture());
					BindShaderParameterResources(CurrentCommandList, Parameters
						, PixelEntry->BindingPlan, ERHIShaderStage::RHI_SHADER_STAGE_PIXEL);
				}

				CurrentCommandList->DrawIndexed(FullScreenTriangle.GetIndexCount(), 1u, 0u, 0, 0u);
				CurrentViewProxy = nullptr;

				CurrentCommandList->EndRenderPass();

				// ImGui samples it later in this frame, so it has to be readable before the list
				// is submitted.
				TransitionToShaderReadable(PreviewDisplay.GetResolvedRHITexture());
			}

			// The id handed to ImGui is the bindless index the overlay resolves, not a native
			// view: that is what keeps this code indifferent to the active back-end. It is
			// recomputed when the display target is rebuilt, at which point the texture object
			// (and with it the index) can be a new one.
			void UpdateTextureId()
			{
				PreviewTextureId = ImTextureID_Invalid;
				if (PreviewDisplay.IsRenderResourceValid())
				{
					PreviewTextureId = EngineTextureToImgui(PreviewDisplay.GetResolvedRHITexture());
				}
			}

		private:
			ImTextureID				PreviewTextureId = ImTextureID_Invalid;
			// Display-referred copy of the view's scene colour, and the size it was built for.
			RRenderTexture2D		PreviewDisplay;
			Vector2Int				PreviewDisplaySize = Vector2Int::Zero();
		};

	}

	EAssetPreviewScene::~EAssetPreviewScene()
	{
		Shutdown();
	}

	void EAssetPreviewScene::Initialize(const EString& InDebugLabel, const Vector3& InZoneOrigin)
	{
		m_DebugLabel = InDebugLabel;
		m_ZoneOrigin = InZoneOrigin;
		if (!m_PreviewRenderer)
		{
			RAssetPreviewSceneRenderer* PreviewRenderer = new RAssetPreviewSceneRenderer();
			PreviewRenderer->SetManageDebugWireframe(FALSE);
			PreviewRenderer->SetRenderDebugWireframe(FALSE);
			PreviewRenderer->Initialize();
			m_PreviewRenderer = PreviewRenderer;
		}
		if (!m_PreviewWorld)
		{
			m_PreviewWorld = new PPreviewWorld();
			m_PreviewWorld->BindRenderScene(m_PreviewRenderer->GetRenderScene());
			m_PreviewWorld->Init();
		}
		m_Initialized = TRUE;
		EnsureCameraActor();
		// From here on the render thread records this preview's frame. Registration is what
		// puts it in the frame at all: nothing else knows the preview world exists.
		EMainManager::GetManagerSingleton()->RegisterFrameRenderer(this);
	}

	void EAssetPreviewScene::Shutdown()
	{
		// Unregistered first, so the frame callbacks can never reach a renderer that is about
		// to be deleted. The calls run either between frames (editor closing) or after the
		// render thread stopped (engine shutdown), so neither can race this.
		EMainManager::GetManagerSingleton()->UnregisterFrameRenderer(this);
		DestroyPreviewActor();
		if (m_CameraActor)
		{
			m_CameraActor->Destroy();
			m_CameraActor = nullptr;
			m_CameraComponent = nullptr;
		}
		if (m_PreviewWorld)
		{
			m_PreviewWorld->Destroy();
			m_PreviewWorld = nullptr;
		}
		if (m_PreviewRenderer)
		{
			m_PreviewRenderer->InitNewFrame();
			m_PreviewRenderer->ShutDown();
			delete m_PreviewRenderer;
			m_PreviewRenderer = nullptr;
		}
		m_StaticMeshAsset = nullptr;
		m_SkinnedMeshAsset = nullptr;
		m_SkeletonAsset = nullptr;
		m_AnimationClip = nullptr;
		m_Initialized = FALSE;
	}

	void EAssetPreviewScene::UpdateViewportSize(UINT32 InWidth, UINT32 InHeight)
	{
		m_ViewportWidth = EMath::Max(InWidth, 64u);
		m_ViewportHeight = EMath::Max(InHeight, 64u);
		EnsureCameraActor();
		if (m_CameraComponent)
		{
			m_CameraComponent->SetViewport(0.0f, 0.0f, static_cast<FLOAT>(m_ViewportWidth), static_cast<FLOAT>(m_ViewportHeight));
		}
	}

	void EAssetPreviewScene::Tick(FLOAT InDeltaSeconds)
	{
		if (!m_Initialized)
		{
			return;
		}

		EnsureCameraActor();
		if (m_AutoRotate)
		{
			m_OrbitRadians += InDeltaSeconds * 0.6f;
		}
		UpdateCameraTransform();
		ApplyAnimation(InDeltaSeconds);
		if (m_PreviewWorld)
		{
			m_PreviewWorld->Tick(InDeltaSeconds);
		}
		// The recording itself happens on the render thread; this only publishes that this
		// frame has something to record. A panel that stops ticking (closed) costs one more
		// frame, which RenderFrame consumes.
		m_bRenderRequested = ((m_PreviewRenderer != nullptr) && (GetPreviewMeshResource() != nullptr)) ? TRUE : FALSE;
	}

	void EAssetPreviewScene::PublishFrameCommands()
	{
		if ((!m_bRenderRequested) || (!m_PreviewRenderer))
		{
			return;
		}
		// The preview scene is double buffered like the main one, and this is the only reader
		// it has: flipping at the same boundary is what makes what Tick queued visible to the
		// render thread in this frame.
		if (RScene* PreviewScene = m_PreviewRenderer->GetRenderScene())
		{
			PreviewScene->SwapCommandSlots();
		}
	}

	void EAssetPreviewScene::RenderFrame()
	{
		if ((!m_bRenderRequested) || (!m_PreviewRenderer))
		{
			return;
		}
		// Cleared on this side: one published frame is one recorded frame.
		m_bRenderRequested = FALSE;
		static_cast<RAssetPreviewSceneRenderer*>(m_PreviewRenderer)->RenderPreview_RenderThread();
	}

	void EAssetPreviewScene::SetStaticMesh(const EStaticMeshAsset* InStaticMeshAsset)
	{
		if ((m_StaticMeshAsset == InStaticMeshAsset) && (!m_SkinnedMeshAsset))
		{
			return;
		}
		m_StaticMeshAsset = InStaticMeshAsset;
		m_SkinnedMeshAsset = nullptr;
		m_SkeletonAsset = nullptr;
		m_AnimationClip = nullptr;
		ResetPlayback();
		RecreatePreviewActor();
	}

	void EAssetPreviewScene::SetSkinnedMesh(const ESkinnedMeshAsset* InSkinnedMeshAsset, const ESkeletonAsset* InSkeletonAsset)
	{
		if ((m_SkinnedMeshAsset == InSkinnedMeshAsset) && (m_SkeletonAsset == InSkeletonAsset) && (!m_StaticMeshAsset))
		{
			return;
		}
		m_StaticMeshAsset = nullptr;
		m_SkinnedMeshAsset = InSkinnedMeshAsset;
		m_SkeletonAsset = InSkeletonAsset;
		ResetPlayback();
		RecreatePreviewActor();
	}

	void EAssetPreviewScene::SetAnimationClip(const ESkeletonAnimationClip* InAnimationClip)
	{
		if (m_AnimationClip == InAnimationClip)
		{
			return;
		}
		m_AnimationClip = InAnimationClip;
		ResetPlayback();
	}

	void EAssetPreviewScene::SetAutoRotate(BOOL8 InAutoRotate)
	{
		m_AutoRotate = InAutoRotate;
	}

	void EAssetPreviewScene::SetPlayAnimation(BOOL8 InPlayAnimation)
	{
		m_PlayAnimation = InPlayAnimation;
	}

	void EAssetPreviewScene::SetPlaybackRate(FLOAT InPlaybackRate)
	{
		m_PlaybackRate = EMath::Max(InPlaybackRate, 0.0f);
	}

	void EAssetPreviewScene::ResetPlayback()
	{
		m_PlaybackTimeSeconds = 0.0f;
	}

	void EAssetPreviewScene::SetPlaybackTimeSeconds(FLOAT InTimeSeconds)
	{
		const FLOAT DurationSeconds = GetClipDurationSeconds();
		if (DurationSeconds <= 0.0f)
		{
			m_PlaybackTimeSeconds = 0.0f;
			return;
		}
		m_PlaybackTimeSeconds = EMath::Clamp(InTimeSeconds, 0.0f, DurationSeconds);
	}

	FLOAT EAssetPreviewScene::GetPlaybackTimeSeconds()const
	{
		return m_PlaybackTimeSeconds;
	}

	FLOAT EAssetPreviewScene::GetClipDurationSeconds()const
	{
		if ((!m_AnimationClip) || (m_AnimationClip->Duration <= 0.0f))
		{
			return 0.0f;
		}
		const FLOAT TicksPerSecond = (m_AnimationClip->TicksPerSecond > 1e-3f)
			? m_AnimationClip->TicksPerSecond
			: EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
		return ((TicksPerSecond > 0.0f) ? (m_AnimationClip->Duration / TicksPerSecond) : 0.0f);
	}

	void EAssetPreviewScene::ClearPreview()
	{
		m_StaticMeshAsset = nullptr;
		m_SkinnedMeshAsset = nullptr;
		m_SkeletonAsset = nullptr;
		m_AnimationClip = nullptr;
		DestroyPreviewActor();
	}

	ImTextureID EAssetPreviewScene::GetTextureId() const
	{
		// Nothing published means nothing recorded: no mesh selected, or the first frame after
		// one was, whose image only exists once the render thread has recorded it.
		if ((!m_PreviewRenderer) || (!m_bRenderRequested))
		{
			return ImTextureID_Invalid;
		}
		return static_cast<const RAssetPreviewSceneRenderer*>(m_PreviewRenderer)->GetTextureId();
	}

	BOOL8 EAssetPreviewScene::HasPreviewMesh() const
	{
		return (GetPreviewMeshResource() != nullptr);
	}

	const ESkinnedMeshAsset* EAssetPreviewScene::GetPreviewSkinnedMeshAsset() const
	{
		return m_SkinnedMeshAsset;
	}

	const ESkeletonAsset* EAssetPreviewScene::GetPreviewSkeletonAsset() const
	{
		return m_SkeletonAsset;
	}

	void EAssetPreviewScene::EnsureCameraActor()
	{
		if ((!m_Initialized) || m_CameraActor)
		{
			return;
		}

		if (!m_PreviewWorld)
		{
			return;
		}

		PActor* NewCameraActor = new PActor();
		NewCameraActor->SetIsTickable(TRUE);
		m_CameraComponent = new PCameraComponent(0.0f, 0.0f, static_cast<FLOAT>(m_ViewportWidth), static_cast<FLOAT>(m_ViewportHeight));
		// BasePass records only the view that is the main one: it exists to skip secondary
		// editor cameras. This camera is the only view of the preview world, so it has to be
		// marked before the actor is added - the flag is read when the proxy is created.
		m_CameraComponent->SetIsMainCamera(TRUE);
		NewCameraActor->SetRootComponent(m_CameraComponent);
		m_PreviewWorld->AddActor(NewCameraActor);
		m_CameraActor = NewCameraActor;
		UpdateCameraTransform();
	}

	void EAssetPreviewScene::RecreatePreviewActor()
	{
		DestroyPreviewActor();

		if ((!m_PreviewWorld) || ((!m_StaticMeshAsset) && ((!m_SkinnedMeshAsset) || (!m_SkeletonAsset))))
		{
			return;
		}

		PActor* NewPreviewActor = new PActor();
		NewPreviewActor->SetIsTickable(TRUE);
		m_PreviewWorld->AddActor(NewPreviewActor);
		m_PreviewActor = NewPreviewActor;

		if (m_StaticMeshAsset)
		{
			m_StaticMeshComponent = new PStaticMeshComponent();
			m_PreviewActor->AddComponent(m_StaticMeshComponent);
			m_StaticMeshComponent->SetMeshAsset(m_StaticMeshAsset);
		}
		else if ((m_SkinnedMeshAsset) && (m_SkeletonAsset))
		{
			m_SkeletalMeshComponent = new PSkeletalMeshComponent();
			m_PreviewActor->AddComponent(m_SkeletalMeshComponent);
			m_SkeletalMeshComponent->SetMeshAsset(m_SkinnedMeshAsset);
			m_SkeletalMeshComponent->SetSkeletonAsset(m_SkeletonAsset);
			m_SkeletalMeshComponent->SetComponentRotation(GetPreviewMeshRotation());
		}

		UpdatePreviewPlacement();
	}

	void EAssetPreviewScene::DestroyPreviewActor()
	{
		if (m_PreviewActor)
		{
			m_PreviewActor->Destroy();
			m_PreviewActor = nullptr;
		}
		m_StaticMeshComponent = nullptr;
		m_SkeletalMeshComponent = nullptr;
	}

	void EAssetPreviewScene::UpdatePreviewPlacement()
	{
		const EMesh* PreviewMesh = GetPreviewMeshResource();
		if ((!m_PreviewActor) || (!PreviewMesh))
		{
			return;
		}

		// Placement works in the same rotated space as the component. Bounds are transformed
		// corner by corner: rotating the centre alone leaves the extent axes unswapped, which
		// is what made 90 degree skeletal previews look offset by mesh.
		const Quaternion PreviewRotation = GetPreviewMeshRotation();
		// CalculateBoundWithSpace writes into the bound, so the asset's bound is copied first.
		EBoundAABB LocalBound = PreviewMesh->GetBoundAABB();
		TArray<Vector3> BoundCorners;
		LocalBound.CalculateBoundWithSpace(Vector3::Zero(), PreviewRotation, Vector3::One(), BoundCorners);

		EBoundAABB WorldBound(BoundCorners[0], BoundCorners[0], TRUE);
		for (INT32 CornerIndex = 1; CornerIndex < BoundCorners.Num(); CornerIndex++)
		{
			WorldBound += EBoundAABB(BoundCorners[CornerIndex], BoundCorners[CornerIndex], TRUE);
		}

		const Vector3 LocalBoundsCenter = (LocalBound.AABBMin + LocalBound.AABBMax) * 0.5f;
		const Vector3 WorldBoundsCenter = (WorldBound.AABBMin + WorldBound.AABBMax) * 0.5f;
		const Vector3 WorldExtent = (WorldBound.AABBMax - WorldBound.AABBMin) * 0.5f;
		const FLOAT MaxExtent = EMath::Max(
			EMath::Max(WorldExtent.x, EMath::Max(WorldExtent.y, WorldExtent.z)),
			EEngineSettings::ENGINE_BOUND_MINIMUM_HALF);

		// Pin the pivot (skeleton root for skinned meshes, bounds centre for static ones) to the
		// zone origin. Auto Rotate turns the camera around the vertical line through that pivot:
		// using the bounds centre as the orbit axis lets weapon geometry shift the axis sideways,
		// so the body sweeps around the view and reads as a spin in the wrong direction. Only the
		// vertical centre is kept for framing.
		const Vector3 PreviewPivot = GetPreviewPivot(LocalBoundsCenter);
		const Vector3 RotatedPivot = QuaternionTransformVector(PreviewRotation, PreviewPivot);
		m_CameraFocusOffset = Vector3(0.0f, WorldBoundsCenter.y - RotatedPivot.y, 0.0f);
		m_CameraDistance = MaxExtent * PreviewCameraDistanceExtentScale;

		m_PreviewActor->SetActorLocation(m_ZoneOrigin - RotatedPivot);
		m_PreviewActor->SetActorScale(Vector3::One());
		UpdateCameraProjection();
		UpdateCameraTransform();
	}

	void EAssetPreviewScene::UpdateCameraProjection()
	{
		if (!m_CameraComponent)
		{
			return;
		}

		// The preview world contains only the mesh and this camera, so the clip planes can track
		// the framing distance instead of the world defaults. The 1:1000 near/far ratio keeps
		// depth precision sane for both centimetre characters and tiny props.
		const FLOAT NearDist = EMath::Max(m_CameraDistance * PreviewCameraNearRatio, 1e-4f);
		const FLOAT FarDist = EMath::Max(m_CameraDistance * PreviewCameraFarRatio, NearDist * 100.0f);
		const FLOAT FovAngleY = m_CameraComponent->GetCameraViewInfo().FovAngleY;
		m_CameraComponent->SetCameraViewInfo(0.0f, 0.0f
			, static_cast<FLOAT>(m_ViewportWidth), static_cast<FLOAT>(m_ViewportHeight)
			, FovAngleY, FarDist, NearDist);
	}

	Quaternion EAssetPreviewScene::GetPreviewMeshRotation() const
	{
		if ((m_SkeletalMeshComponent) && (m_SkeletonAsset))
		{
			return MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(PreviewSkeletalMeshPitchDegrees));
		}
		return Quaternion::Identity();
	}

	Vector3 EAssetPreviewScene::GetPreviewPivot(const Vector3& InBoundsCenter) const
	{
		if ((!m_SkeletalMeshComponent) || (!m_SkeletonAsset))
		{
			return InBoundsCenter;
		}

		const ESkeleton* SkeletonResource = m_SkeletonAsset->GetStoragedResource();
		if (!SkeletonResource)
		{
			return InBoundsCenter;
		}

		const ESkeleton::EBonePart& BonePart = SkeletonResource->GetBonePart();
		for (INT32 BoneIndex = 0; BoneIndex < BonePart.Num(); BoneIndex++)
		{
			// The root is the bone without a parent. Its bind position is already in mesh
			// space, so it needs no parent chain composition.
			if (BonePart[BoneIndex].Parent < 0)
			{
				return BonePart[BoneIndex].DefaultPosition;
			}
		}
		return InBoundsCenter;
	}

	void EAssetPreviewScene::UpdateCameraTransform()
	{
		EnsureCameraActor();
		if (!m_CameraActor)
		{
			return;
		}

		const Vector3 FocusPoint = m_ZoneOrigin + m_CameraFocusOffset;
		const FLOAT CameraHeight = m_CameraDistance * PreviewCameraHeightScale;
		const Vector3 CameraPosition(
			FocusPoint.x + std::cos(m_OrbitRadians) * m_CameraDistance,
			FocusPoint.y + CameraHeight,
			FocusPoint.z + std::sin(m_OrbitRadians) * m_CameraDistance);

		m_CameraActor->SetActorLocation(CameraPosition);
		m_CameraActor->SetActorRotation(LookAtTargetQuaternionRotation(FocusPoint, CameraPosition, Vector3::YVector()));
	}

	void EAssetPreviewScene::ApplyAnimation(FLOAT InDeltaSeconds)
	{
		if ((!m_SkeletalMeshComponent) || (!m_SkeletonAsset))
		{
			return;
		}

		const ESkeleton* SkeletonResource = m_SkeletonAsset->GetStoragedResource();
		if (!SkeletonResource)
		{
			return;
		}

		ESkeletonBoneMemoryPool& BoneMemoryPool = m_SkeletalMeshComponent->GetBoneMemoryPool();
		BoneMemoryPool.GenerateFromSkeleton(SkeletonResource);

		if (m_AnimationClip)
		{
			if (m_PlayAnimation)
			{
				m_PlaybackTimeSeconds += InDeltaSeconds * m_PlaybackRate;

				// Wrap the editor clock too, not just the sampled key time: otherwise the
				// Position slider sticks at the clip end while the mesh keeps looping, and the
				// accumulated seconds slowly lose float precision.
				const FLOAT DurationSeconds = GetClipDurationSeconds();
				if (DurationSeconds > 0.0f)
				{
					m_PlaybackTimeSeconds = std::fmod(m_PlaybackTimeSeconds, DurationSeconds);
				}
			}

			const FLOAT TicksPerSecond = (m_AnimationClip->TicksPerSecond > 1e-3f) ? m_AnimationClip->TicksPerSecond : EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
			const FLOAT KeyTime = EAnimationSampler::WrapKeyTime(m_PlaybackTimeSeconds * TicksPerSecond, m_AnimationClip->Duration);
			auto VectorLerp = [](const Vector3& InValue0, const Vector3& InValue1, FLOAT InAlpha)->Vector3
			{
				return Vector3::Lerp(InValue0, InValue1, InAlpha);
			};
			auto QuaternionLerp = [](const Quaternion& InValue0, const Quaternion& InValue1, FLOAT InAlpha)->Quaternion
			{
				return Quaternion::SLerp(InValue0, InValue1, InAlpha);
			};

			const TMap<EString, USHORT>& BoneMapping = SkeletonResource->GetBoneMapping();
			for (INT32 CurveIndex = 0, CurveNum = m_AnimationClip->AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
			{
				const EBoneAnimationCurve& Curve = m_AnimationClip->AnimationCurves[CurveIndex];
				if (!(BoneMapping.ContainsKey(Curve.BoneName)))
				{
					continue;
				}

				EBoneTransform BoneTransform = BoneMemoryPool.GetBoneRelativeTransform(Curve.BoneName);
				EAnimationSampler::SampleKey(KeyTime, Curve.PositionKeys, BoneTransform.Position, VectorLerp);
				EAnimationSampler::SampleKey(KeyTime, Curve.RotationKeys, BoneTransform.Rotation, QuaternionLerp);
				EAnimationSampler::SampleKey(KeyTime, Curve.ScalingKeys, BoneTransform.Scaling, VectorLerp);
				BoneMemoryPool.SetBoneRelativeTransform(Curve.BoneName, BoneTransform);
			}
		}

		BoneMemoryPool.RecursionToRootTransforms();
		m_SkeletalMeshComponent->MarkBoneMemoryPoolDirty();
	}

	const EMesh* EAssetPreviewScene::GetPreviewMeshResource() const
	{
		if (m_StaticMeshAsset)
		{
			return m_StaticMeshAsset->GetStoragedResource();
		}
		if (m_SkinnedMeshAsset)
		{
			return m_SkinnedMeshAsset->GetStoragedResource();
		}
		return nullptr;
	}
}

#endif