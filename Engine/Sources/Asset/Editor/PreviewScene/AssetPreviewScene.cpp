#include "AssetPreviewScene.h"

#if ENGINE_EDITOR_ONLY

#include <Animation/AnimationSampler.h>

#include <AnimationAsset/AnimationAsset.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/World/PreviewWorld.h>
#include <Main/MainManager.h>
#include <RenderCore/Renderer/SceneRenderer.h>
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

	}

	EAssetPreviewScene::~EAssetPreviewScene()
	{
		Shutdown();
	}

	void EAssetPreviewScene::Initialize(const EString& InDebugLabel, const Vector3& InZoneOrigin)
	{
		DebugLabel = InDebugLabel;
		ZoneOrigin = InZoneOrigin;
		if (!PreviewRenderer)
		{
			// The preview is an ordinary scene renderer whose final image lands in a target
			// the renderer owns instead of the back buffer. It records the same pipeline as
			// the main window - every stage, every pass, the same shaders - so a pipeline
			// change reaches the preview without an edit here.
			RSceneRenderer* NewPreviewRenderer = new RSceneRenderer();
			FRendererOutputDescription OutputDescription;
			OutputDescription.Type = RRendererOutputType::RENDERER_OUTPUT_OFFSCREEN;
			NewPreviewRenderer->SetOutputDescription(OutputDescription);
			// The preview does not own the process wide debug primitive frame or the editor
			// overlay, so it neither drains that frame nor draws it.
			NewPreviewRenderer->SetManageDebugWireframe(FALSE);
			NewPreviewRenderer->SetRenderDebugWireframe(FALSE);
			NewPreviewRenderer->Initialize();
			PreviewRenderer = NewPreviewRenderer;
		}
		if (!PreviewWorld)
		{
			PreviewWorld = new PPreviewWorld();
			PreviewWorld->BindRenderScene(PreviewRenderer->GetRenderScene());
			PreviewWorld->Init();
		}
		bInitialized = TRUE;
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
		if (CameraActor)
		{
			CameraActor->Destroy();
			CameraActor = nullptr;
			CameraComponent = nullptr;
		}
		if (PreviewWorld)
		{
			PreviewWorld->Destroy();
			PreviewWorld = nullptr;
		}
		if (PreviewRenderer)
		{
			PreviewRenderer->InitNewFrame();
			PreviewRenderer->ShutDown();
			delete PreviewRenderer;
			PreviewRenderer = nullptr;
		}
		StaticMeshAsset = nullptr;
		SkinnedMeshAsset = nullptr;
		SkeletonAsset = nullptr;
		AnimationClip = nullptr;
		PreviewTextureId = ImTextureID_Invalid;
		bInitialized = FALSE;
	}

	void EAssetPreviewScene::UpdateViewportSize(UINT32 InWidth, UINT32 InHeight)
	{
		ViewportWidth = EMath::Max(InWidth, 64u);
		ViewportHeight = EMath::Max(InHeight, 64u);
		EnsureCameraActor();
		if (CameraComponent)
		{
			CameraComponent->SetViewport(0.0f, 0.0f, static_cast<FLOAT>(ViewportWidth), static_cast<FLOAT>(ViewportHeight));
		}
	}

	void EAssetPreviewScene::Tick(FLOAT InDeltaSeconds)
	{
		if (!bInitialized)
		{
			return;
		}

		EnsureCameraActor();
		if (bAutoRotate)
		{
			OrbitRadians += InDeltaSeconds * 0.6f;
		}
		UpdateCameraTransform();
		ApplyAnimation(InDeltaSeconds);
		if (PreviewWorld)
		{
			PreviewWorld->Tick(InDeltaSeconds);
		}
		// The recording itself happens on the render thread; this only publishes that this
		// frame has something to record. A panel that stops ticking (closed) costs one more
		// frame, which RenderFrame consumes.
		bRenderRequested = ((PreviewRenderer != nullptr) && (GetPreviewMeshResource() != nullptr)) ? TRUE : FALSE;
	}

	void EAssetPreviewScene::PublishFrameCommands()
	{
		if ((!bRenderRequested) || (!PreviewRenderer))
		{
			return;
		}
		// The preview scene is double buffered like the main one, and this is the only reader
		// it has: flipping at the same boundary is what makes what Tick queued visible to the
		// render thread in this frame.
		if (RScene* PreviewScene = PreviewRenderer->GetRenderScene())
		{
			PreviewScene->SwapCommandSlots();
		}
	}

	void EAssetPreviewScene::RenderFrame()
	{
		if ((!bRenderRequested) || (!PreviewRenderer))
		{
			return;
		}
		// Cleared on this side: one published frame is one recorded frame.
		bRenderRequested = FALSE;
		// The renderer records the same frame the main scene records; only its output target
		// differs, which is what keeps the preview on the current pipeline.
		PreviewRenderer->InitNewFrame();
		PreviewRenderer->Render();

		// The image exists now. Converting it here keeps the bindless registration on the
		// render thread, which is where the device is allowed to be touched.
		IRRHITexture* OutputTexture = PreviewRenderer->GetOutputColorTexture();
		PreviewTextureId = (OutputTexture ? EngineTextureToImgui(OutputTexture) : ImTextureID_Invalid);
	}

	void EAssetPreviewScene::OnShaderAssetReplaced(const EName& InShaderName)
	{
		if (PreviewRenderer)
		{
			PreviewRenderer->OnShaderAssetReplaced(InShaderName);
		}
	}

	void EAssetPreviewScene::SetStaticMesh(const EStaticMeshAsset* InStaticMeshAsset)
	{
		if ((StaticMeshAsset == InStaticMeshAsset) && (!SkinnedMeshAsset))
		{
			return;
		}
		StaticMeshAsset = InStaticMeshAsset;
		SkinnedMeshAsset = nullptr;
		SkeletonAsset = nullptr;
		AnimationClip = nullptr;
		ResetPlayback();
		RecreatePreviewActor();
	}

	void EAssetPreviewScene::SetSkinnedMesh(const ESkinnedMeshAsset* InSkinnedMeshAsset, const ESkeletonAsset* InSkeletonAsset)
	{
		if ((SkinnedMeshAsset == InSkinnedMeshAsset) && (SkeletonAsset == InSkeletonAsset) && (!StaticMeshAsset))
		{
			return;
		}
		StaticMeshAsset = nullptr;
		SkinnedMeshAsset = InSkinnedMeshAsset;
		SkeletonAsset = InSkeletonAsset;
		ResetPlayback();
		RecreatePreviewActor();
	}

	void EAssetPreviewScene::SetAnimationClip(const ESkeletonAnimationClip* InAnimationClip)
	{
		if (AnimationClip == InAnimationClip)
		{
			return;
		}
		AnimationClip = InAnimationClip;
		ResetPlayback();
	}

	void EAssetPreviewScene::SetAutoRotate(BOOL8 InAutoRotate)
	{
		bAutoRotate = InAutoRotate;
	}

	void EAssetPreviewScene::SetPlayAnimation(BOOL8 InPlayAnimation)
	{
		bPlayAnimation = InPlayAnimation;
	}

	void EAssetPreviewScene::SetPlaybackRate(FLOAT InPlaybackRate)
	{
		PlaybackRate = EMath::Max(InPlaybackRate, 0.0f);
	}

	void EAssetPreviewScene::ResetPlayback()
	{
		PlaybackTimeSeconds = 0.0f;
	}

	void EAssetPreviewScene::SetPlaybackTimeSeconds(FLOAT InTimeSeconds)
	{
		const FLOAT DurationSeconds = GetClipDurationSeconds();
		if (DurationSeconds <= 0.0f)
		{
			PlaybackTimeSeconds = 0.0f;
			return;
		}
		PlaybackTimeSeconds = EMath::Clamp(InTimeSeconds, 0.0f, DurationSeconds);
	}

	FLOAT EAssetPreviewScene::GetPlaybackTimeSeconds() const
	{
		return PlaybackTimeSeconds;
	}

	FLOAT EAssetPreviewScene::GetClipDurationSeconds() const
	{
		if ((!AnimationClip) || (AnimationClip->Duration <= 0.0f))
		{
			return 0.0f;
		}
		const FLOAT TicksPerSecond = (AnimationClip->TicksPerSecond > 1e-3f)
			? AnimationClip->TicksPerSecond
			: EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
		return ((TicksPerSecond > 0.0f) ? (AnimationClip->Duration / TicksPerSecond) : 0.0f);
	}

	void EAssetPreviewScene::ClearPreview()
	{
		StaticMeshAsset = nullptr;
		SkinnedMeshAsset = nullptr;
		SkeletonAsset = nullptr;
		AnimationClip = nullptr;
		DestroyPreviewActor();
	}

	ImTextureID EAssetPreviewScene::GetTextureId() const
	{
		// Nothing published means nothing recorded: no mesh selected, or the first frame after
		// one was, whose image only exists once the render thread has recorded it.
		if ((!PreviewRenderer) || (!bRenderRequested))
		{
			return ImTextureID_Invalid;
		}
		return PreviewTextureId;
	}

	BOOL8 EAssetPreviewScene::HasPreviewMesh() const
	{
		return (GetPreviewMeshResource() != nullptr);
	}

	const ESkinnedMeshAsset* EAssetPreviewScene::GetPreviewSkinnedMeshAsset() const
	{
		return SkinnedMeshAsset;
	}

	const ESkeletonAsset* EAssetPreviewScene::GetPreviewSkeletonAsset() const
	{
		return SkeletonAsset;
	}

	void EAssetPreviewScene::EnsureCameraActor()
	{
		if ((!bInitialized) || CameraActor)
		{
			return;
		}

		if (!PreviewWorld)
		{
			return;
		}

		PActor* NewCameraActor = new PActor();
		NewCameraActor->SetIsTickable(TRUE);
		CameraComponent = new PCameraComponent(0.0f, 0.0f, static_cast<FLOAT>(ViewportWidth), static_cast<FLOAT>(ViewportHeight));
		// BasePass records only the view that is the main one: it exists to skip secondary
		// editor cameras. This camera is the only view of the preview world, so it has to be
		// marked before the actor is added - the flag is read when the proxy is created.
		CameraComponent->SetIsMainCamera(TRUE);
		NewCameraActor->SetRootComponent(CameraComponent);
		PreviewWorld->AddActor(NewCameraActor);
		CameraActor = NewCameraActor;
		UpdateCameraTransform();
	}

	void EAssetPreviewScene::RecreatePreviewActor()
	{
		DestroyPreviewActor();

		if ((!PreviewWorld) || ((!StaticMeshAsset) && ((!SkinnedMeshAsset) || (!SkeletonAsset))))
		{
			return;
		}

		PActor* NewPreviewActor = new PActor();
		NewPreviewActor->SetIsTickable(TRUE);
		PreviewWorld->AddActor(NewPreviewActor);
		PreviewActor = NewPreviewActor;

		if (StaticMeshAsset)
		{
			StaticMeshComponent = new PStaticMeshComponent();
			PreviewActor->AddComponent(StaticMeshComponent);
			StaticMeshComponent->SetMeshAsset(StaticMeshAsset);
		}
		else if ((SkinnedMeshAsset) && (SkeletonAsset))
		{
			SkeletalMeshComponent = new PSkeletalMeshComponent();
			PreviewActor->AddComponent(SkeletalMeshComponent);
			SkeletalMeshComponent->SetMeshAsset(SkinnedMeshAsset);
			SkeletalMeshComponent->SetSkeletonAsset(SkeletonAsset);
			SkeletalMeshComponent->SetComponentRotation(GetPreviewMeshRotation());
		}

		UpdatePreviewPlacement();
	}

	void EAssetPreviewScene::DestroyPreviewActor()
	{
		if (PreviewActor)
		{
			PreviewActor->Destroy();
			PreviewActor = nullptr;
		}
		StaticMeshComponent = nullptr;
		SkeletalMeshComponent = nullptr;
	}

	void EAssetPreviewScene::UpdatePreviewPlacement()
	{
		const EMesh* PreviewMesh = GetPreviewMeshResource();
		if ((!PreviewActor) || (!PreviewMesh))
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
		CameraFocusOffset = Vector3(0.0f, WorldBoundsCenter.y - RotatedPivot.y, 0.0f);
		CameraDistance = MaxExtent * PreviewCameraDistanceExtentScale;

		PreviewActor->SetActorLocation(ZoneOrigin - RotatedPivot);
		PreviewActor->SetActorScale(Vector3::One());
		UpdateCameraProjection();
		UpdateCameraTransform();
	}

	void EAssetPreviewScene::UpdateCameraProjection()
	{
		if (!CameraComponent)
		{
			return;
		}

		// The preview world contains only the mesh and this camera, so the clip planes can track
		// the framing distance instead of the world defaults. The 1:1000 near/far ratio keeps
		// depth precision sane for both centimetre characters and tiny props.
		const FLOAT NearDist = EMath::Max(CameraDistance * PreviewCameraNearRatio, 1e-4f);
		const FLOAT FarDist = EMath::Max(CameraDistance * PreviewCameraFarRatio, NearDist * 100.0f);
		const FLOAT FovAngleY = CameraComponent->GetCameraViewInfo().FovAngleY;
		CameraComponent->SetCameraViewInfo(0.0f, 0.0f
			, static_cast<FLOAT>(ViewportWidth), static_cast<FLOAT>(ViewportHeight)
			, FovAngleY, FarDist, NearDist);
	}

	Quaternion EAssetPreviewScene::GetPreviewMeshRotation() const
	{
		if ((SkeletalMeshComponent) && (SkeletonAsset))
		{
			return MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(PreviewSkeletalMeshPitchDegrees));
		}
		return Quaternion::Identity();
	}

	Vector3 EAssetPreviewScene::GetPreviewPivot(const Vector3& InBoundsCenter) const
	{
		if ((!SkeletalMeshComponent) || (!SkeletonAsset))
		{
			return InBoundsCenter;
		}

		const ESkeleton* SkeletonResource = SkeletonAsset->GetStoragedResource();
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
		if (!CameraActor)
		{
			return;
		}

		const Vector3 FocusPoint = ZoneOrigin + CameraFocusOffset;
		const FLOAT CameraHeight = CameraDistance * PreviewCameraHeightScale;
		const Vector3 CameraPosition(
			FocusPoint.x + EMath::Cos(OrbitRadians) * CameraDistance,
			FocusPoint.y + CameraHeight,
			FocusPoint.z + EMath::Sin(OrbitRadians) * CameraDistance);

		CameraActor->SetActorLocation(CameraPosition);
		CameraActor->SetActorRotation(LookAtTargetQuaternionRotation(FocusPoint, CameraPosition, Vector3::YVector()));
	}

	void EAssetPreviewScene::ApplyAnimation(FLOAT InDeltaSeconds)
	{
		if ((!SkeletalMeshComponent) || (!SkeletonAsset))
		{
			return;
		}

		const ESkeleton* SkeletonResource = SkeletonAsset->GetStoragedResource();
		if (!SkeletonResource)
		{
			return;
		}

		ESkeletonBoneMemoryPool& BoneMemoryPool = SkeletalMeshComponent->GetBoneMemoryPool();
		BoneMemoryPool.GenerateFromSkeleton(SkeletonResource);

		if (AnimationClip)
		{
			if (bPlayAnimation)
			{
				PlaybackTimeSeconds += InDeltaSeconds * PlaybackRate;

				// Wrap the editor clock too, not just the sampled key time: otherwise the
				// Position slider sticks at the clip end while the mesh keeps looping, and the
				// accumulated seconds slowly lose float precision.
				const FLOAT DurationSeconds = GetClipDurationSeconds();
				if (DurationSeconds > 0.0f)
				{
					PlaybackTimeSeconds = EMath::FMod(PlaybackTimeSeconds, DurationSeconds);
				}
			}

			const FLOAT TicksPerSecond = (AnimationClip->TicksPerSecond > 1e-3f) ? AnimationClip->TicksPerSecond : EEngineSettings::ENGINE_ANIMATION_DEFAULT_TICKS_PER_SECOND;
			const FLOAT KeyTime = EAnimationSampler::WrapKeyTime(PlaybackTimeSeconds * TicksPerSecond, AnimationClip->Duration);
			auto VectorLerp = [](const Vector3& InValue0, const Vector3& InValue1, FLOAT InAlpha)->Vector3
			{
				return Vector3::Lerp(InValue0, InValue1, InAlpha);
			};
			auto QuaternionLerp = [](const Quaternion& InValue0, const Quaternion& InValue1, FLOAT InAlpha)->Quaternion
			{
				return Quaternion::SLerp(InValue0, InValue1, InAlpha);
			};

			const TMap<EString, USHORT>& BoneMapping = SkeletonResource->GetBoneMapping();
			for (INT32 CurveIndex = 0, CurveNum = AnimationClip->AnimationCurves.Num(); CurveIndex < CurveNum; CurveIndex++)
			{
				const EBoneAnimationCurve& Curve = AnimationClip->AnimationCurves[CurveIndex];
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
		SkeletalMeshComponent->MarkBoneMemoryPoolDirty();
	}

	const EMesh* EAssetPreviewScene::GetPreviewMeshResource() const
	{
		if (StaticMeshAsset)
		{
			return StaticMeshAsset->GetStoragedResource();
		}
		if (SkinnedMeshAsset)
		{
			return SkinnedMeshAsset->GetStoragedResource();
		}
		return nullptr;
	}
}

#endif