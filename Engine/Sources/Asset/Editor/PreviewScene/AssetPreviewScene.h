#pragma once

#if _EDITOR_ONLY

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
		void ApplyAnimation(FLOAT InDeltaSeconds);
		const EMesh* GetPreviewMeshResource() const;

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
		const ESkeletonAnimationClip*	m_AnimationClip = nullptr;
	public:
		EAssetPreviewScene(const EAssetPreviewScene&) = delete;
		EAssetPreviewScene& operator=(const EAssetPreviewScene&) = delete;
	};
}

#endif