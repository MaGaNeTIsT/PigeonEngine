#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <imgui.h>

#include <Base/DataStructure/BuiltInType.h>
#include <Base/DataStructure/Text/String.h>
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
		// GameThread: the frame's game-side work - animation sampling, camera, scene tick. The
		// image is recorded by RenderFrame, on the render thread, never here.
		void Tick(FLOAT InDeltaSeconds);

		// IEngineFrameRenderer
		virtual void PublishFrameCommands() override;
		virtual void RenderFrame() override;
		// Forwards a shader replacement to the preview renderer, whose pipeline states and
		// parameter blocks were derived from the object that was just replaced.
		virtual void OnShaderAssetReplaced(const EName& InShaderName) override;

		void SetStaticMesh(const EStaticMeshAsset* InStaticMeshAsset);
		void SetSkinnedMesh(const ESkinnedMeshAsset* InSkinnedMeshAsset, const ESkeletonAsset* InSkeletonAsset);
		void SetAnimationClip(const ESkeletonAnimationClip* InAnimationClip);
		void SetAutoRotate(BOOL8 InAutoRotate);
		void SetPlayAnimation(BOOL8 InPlayAnimation);
		void SetPlaybackRate(FLOAT InPlaybackRate);
		void ResetPlayback();
		// The editor's timeline: the position is in seconds and is clamped to the selected
		// clip. While playback is enabled the value still advances every Tick.
		void SetPlaybackTimeSeconds(FLOAT InTimeSeconds);
		PE_NODISCARD FLOAT GetPlaybackTimeSeconds() const;
		PE_NODISCARD FLOAT GetClipDurationSeconds() const;
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
		void ApplyAnimation(FLOAT InDeltaSeconds);
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
		BOOL8					bInitialized = FALSE;
		// Set by Tick (GameThread) and cleared by RenderFrame (RenderThread): one published
		// frame is one recorded frame, and a panel that stopped ticking stops rendering.
		BOOL8					bRenderRequested = FALSE;
		// The ImGui handle of the last recorded image. Built on the render thread in RenderFrame,
		// because registering the texture's bindless view is device work; the panel only reads it.
		ImTextureID				PreviewTextureId = ImTextureID_Invalid;
		BOOL8					bAutoRotate = TRUE;
		BOOL8					bPlayAnimation = TRUE;
		UINT32				ViewportWidth = 512u;
		UINT32				ViewportHeight = 512u;
		FLOAT				PlaybackRate = 1.0f;
		FLOAT				OrbitRadians = 0.0f;
		FLOAT				PlaybackTimeSeconds = 0.0f;
		FLOAT				CameraDistance = 200.0f;
		// Offset from the zone origin to the framing centre in preview space. The pivot sits on
		// the zone origin, while the camera still needs to look at the whole mesh.
		Vector3				CameraFocusOffset = Vector3::Zero();
		EString				DebugLabel;
		Vector3				ZoneOrigin = Vector3::Zero();
		PActor* 				CameraActor = nullptr;
		PCameraComponent*	CameraComponent = nullptr;
		PWorld*				PreviewWorld = nullptr;
		RSceneRenderer*		PreviewRenderer = nullptr;
		PActor*				PreviewActor = nullptr;
		PStaticMeshComponent*	StaticMeshComponent = nullptr;
		PSkeletalMeshComponent*	SkeletalMeshComponent = nullptr;
		const EStaticMeshAsset*	StaticMeshAsset = nullptr;
		const ESkinnedMeshAsset*	SkinnedMeshAsset = nullptr;
		const ESkeletonAsset*	SkeletonAsset = nullptr;
		const ESkeletonAnimationClip*	AnimationClip = nullptr;
	public:
		EAssetPreviewScene(const EAssetPreviewScene&) = delete;
		EAssetPreviewScene& operator=(const EAssetPreviewScene&) = delete;
	};
}

#endif