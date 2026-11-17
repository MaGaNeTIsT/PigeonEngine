#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <imgui.h>

#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Text/String.h>
#include <Animation/AnimationTypes.h>
#include <Main/FrameRenderer.h>

namespace PigeonEngine
{
	class PActor;
	class PWorld;
	class PCameraComponent;
	class PStaticMeshComponent;
	class PSkeletalMeshComponent;
	class EStaticMeshAsset;
	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	class ESkeletonAnimationAsset;
	class EAnimStateMachine;
	class EAnimStateMachineInstance;
	class EAnimMontage;
	class EMesh;
	class RSceneRenderer;
	struct ESkeletonAnimationClip;

	class EAssetPreviewScene final : public IEngineFrameRenderer
	{
	public:
		EAssetPreviewScene() = default;
		~EAssetPreviewScene();

		void Initialize(const EString& InDebugLabel, const Vector3& InZoneOrigin);
		void Shutdown();

		void UpdateViewportSize(UINT32 InWidth, UINT32 InHeight);
		// GameThread: camera, preview scene tick, and the AnimInstance update that runs from
		// the skeletal component's TickRender. The image is recorded by RenderFrame, on the
		// render thread, never here.
		void Tick(FLOAT InDeltaSeconds);

		// IEngineFrameRenderer
		virtual void PublishFrameCommands() override;
		virtual void RenderFrame() override;

		void SetStaticMesh(const EStaticMeshAsset* InStaticMeshAsset);
		void SetSkinnedMesh(const ESkinnedMeshAsset* InSkinnedMeshAsset, const ESkeletonAsset* InSkeletonAsset);
		// The preview runs the same EAnimInstance the runtime uses. InAnimationClipIndex is
		// resolved against InAnimationAsset; -1 keeps the bind pose.
		void SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset, INT32 InAnimationClipIndex);
		// Editor draft state machine. The component owns the instance; the caller can inject
		// parameters through GetStateMachineInstance().
		void SetStateMachine(const EAnimStateMachine* InMachine);
		void ClearStateMachine();
		PE_NODISCARD EAnimStateMachineInstance* GetStateMachineInstance()const;
		// Editor draft montage. The preview plays one section as a layer over the bind pose so
		// BoneMask/Slot/Notify behavior can be inspected before the asset is applied. The
		// animation asset supplies the section clips; it is bound to the AnimInstance first.
		void SetMontage(const EAnimMontage* InMontage, const ESkeletonAnimationAsset* InAnimationAsset
			, const EString& InSectionName);
		void ClearMontage();
		PE_NODISCARD BOOL8 IsMontageMode()const;
		void SetAutoRotate(BOOL8 InAutoRotate);
		void SetPlayAnimation(BOOL8 InPlayAnimation);
		void SetPlaybackRate(FLOAT InPlaybackRate);
		void SetPlaybackLoopMode(EAnimationLoopMode InLoopMode);
		void ResetPlayback();
		// The editor's timeline: the position is in seconds and is clamped to the selected
		// clip. While playback is enabled the value still advances every Tick.
		void SetPlaybackTimeSeconds(FLOAT InTimeSeconds);
		PE_NODISCARD FLOAT GetPlaybackTimeSeconds()const;
		PE_NODISCARD FLOAT GetClipDurationSeconds()const;
		void ClearPreview();

		ImTextureID GetTextureId() const;
		BOOL8 HasPreviewMesh() const;
		const ESkinnedMeshAsset* GetPreviewSkinnedMeshAsset() const;
		const ESkeletonAsset* GetPreviewSkeletonAsset() const;

	private:
		void EnsureCameraActor();
		void RecreatePreviewActor();
		void DestroyPreviewActor();
		void UpdatePreviewPlacement();
		void UpdateCameraTransform();
		// Preview-only clip planes: they follow the framing distance so assets authored at
		// different scales (metres vs centimetres) are neither clipped nor lost in the distance.
		void UpdateCameraProjection();
		// Re-binds the current asset/clip to the preview component after it is created or the
		// mesh/skeleton changed. The component owns playback from then on.
		void BindPreviewAnimation();
		const EMesh* GetPreviewMeshResource() const;
		// Skeletal assets are authored Y-up and are rotated into the engine's Z-up space. The
		// component rotation and the placement maths have to use this one value: computing the
		// AABB centre in mesh space while drawing the mesh rotated is what offsets the preview.
		Quaternion GetPreviewMeshRotation() const;
		// The mesh point that is pinned to the preview zone origin. Static meshes use their
		// geometry centre; skinned meshes use the skeleton root so every character stands on the
		// same spot regardless of weapons or pose bounds.
		Vector3 GetPreviewPivot(const Vector3& InBoundsCenter) const;

	private:
		BOOL8					m_Initialized = FALSE;
		// Set by Tick (GameThread) and cleared by RenderFrame (RenderThread): one published
		// frame is one recorded frame, and a panel that stopped ticking stops rendering.
		BOOL8					m_bRenderRequested = FALSE;
		BOOL8					m_AutoRotate = TRUE;
		BOOL8					m_PlayAnimation = TRUE;
		UINT32				m_ViewportWidth = 512u;
		UINT32				m_ViewportHeight = 512u;
		FLOAT				m_PlaybackRate = 1.0f;
		FLOAT				m_OrbitRadians = 0.0f;
		FLOAT				m_PlaybackTimeSeconds = 0.0f;
		FLOAT				m_CameraDistance = 200.0f;
		// Offset from the zone origin to the framing centre in preview space. The pivot sits on
		// the zone origin, while the camera still needs to look at the whole mesh.
		Vector3				m_CameraFocusOffset = Vector3::Zero();
		EString				m_DebugLabel;
		Vector3				m_ZoneOrigin = Vector3::Zero();
		PActor* 				m_CameraActor = nullptr;
		PCameraComponent*	m_CameraComponent = nullptr;
		PWorld*				m_PreviewWorld = nullptr;
		RSceneRenderer*		m_PreviewRenderer = nullptr;
		PActor*				m_PreviewActor = nullptr;
		PStaticMeshComponent*	m_StaticMeshComponent = nullptr;
		PSkeletalMeshComponent*	m_SkeletalMeshComponent = nullptr;
		const EStaticMeshAsset*	m_StaticMeshAsset = nullptr;
		const ESkinnedMeshAsset*	m_SkinnedMeshAsset = nullptr;
		const ESkeletonAsset*	m_SkeletonAsset = nullptr;
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
		INT32					m_AnimationClipIndex = -1;
		const ESkeletonAnimationClip*	m_AnimationClip = nullptr;
		const EAnimMontage*		m_Montage = nullptr;
		EString					m_MontageSectionName;
		BOOL8					m_bMontageMode = FALSE;
		EAnimationLoopMode		m_LoopMode = ANIMATION_LOOP_REPEAT;
	public:
		EAssetPreviewScene(const EAssetPreviewScene&) = delete;
		EAssetPreviewScene& operator=(const EAssetPreviewScene&) = delete;
	};
}

#endif