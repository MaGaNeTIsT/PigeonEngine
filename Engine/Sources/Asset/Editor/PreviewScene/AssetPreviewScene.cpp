#include "AssetPreviewScene.h"

#if _EDITOR_ONLY

#include <algorithm>
#include <cmath>

#include <AnimationAsset/AnimationAsset.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>
#include <PigeonBase/Object/World/PreviewWorld.h>
#include <RenderCore/RenderDevice/DeviceD3D11.h>
#include <RenderCore/Renderer/SceneRenderer.h>
#include <RenderCore/RenderProxy/ViewProxy.h>

namespace PigeonEngine
{
	namespace
	{
		class RAssetPreviewSceneRenderer final : public RSceneRenderer
		{
		public:
			void RenderPreview()
			{
				RDeviceD3D11* RenderDevice = RDeviceD3D11::GetDeviceSingleton();
				RenderDevice->BindVSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState, 0u);
				RenderDevice->BindVSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState, 1u);
				RenderDevice->BindVSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState, 2u);
				RenderDevice->BindVSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState, 3u);
				RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState, 0u);
				RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState, 1u);
				RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState, 2u);
				RenderDevice->BindPSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState, 3u);
				RenderDevice->BindCSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_CLAMP].SamplerState, 0u);
				RenderDevice->BindCSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_POINT_WRAP].SamplerState, 1u);
				RenderDevice->BindCSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_CLAMP].SamplerState, 2u);
				RenderDevice->BindCSSamplerState(Samplers[RSamplerType::SAMPLER_TYPE_LINEAR_WRAP].SamplerState, 3u);

				RenderDevice->SetPrimitiveTopology(RPrimitiveTopologyType::PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				RenderDevice->SetRasterizerState(Rasterizer[RRasterizerType::RASTERIZER_TYPE_SOLID_BACK].RasterizerState);

				BasePass();
			}

			ImTextureID GetTextureId() const
			{
				if (!Scene)
				{
					return ImTextureID_Invalid;
				}

				const TArray<RViewProxy*>& ViewProxies = Scene->GetViewProxies().SceneProxies;
				if (ViewProxies.Num() <= 0)
				{
					return ImTextureID_Invalid;
				}

				RViewProxy* PreviewViewProxy = ViewProxies[0];
				if (!PreviewViewProxy)
				{
					return ImTextureID_Invalid;
				}

				const RSceneTextures* const* SceneTextures = ViewSceneTextures.FindValueAsPtr(PreviewViewProxy->GetUniqueID());
				if ((!SceneTextures) || (!(*SceneTextures)) || (!(*SceneTextures)->SceneColor.ShaderResourceView.Get()))
				{
					return ImTextureID_Invalid;
				}

				return reinterpret_cast<ImTextureID>((*SceneTextures)->SceneColor.ShaderResourceView.Get());
			}
		};

		template<typename TValueType, typename TKeyType, typename TLerpType>
		BOOL8 SampleAnimationKey(FLOAT InKeyTime, const TArray<TKeyType>& InKeys, TValueType& OutValue, const TLerpType& InLerp)
		{
			const INT32 KeyNum = InKeys.Num();
			if (KeyNum <= 0)
			{
				return FALSE;
			}

			if ((KeyNum == 1) || (InKeyTime <= static_cast<FLOAT>(InKeys[0].Time)))
			{
				OutValue = InKeys[0].Value;
				return TRUE;
			}

			for (INT32 KeyIndex = 0; KeyIndex < (KeyNum - 1); KeyIndex++)
			{
				const FLOAT CurrentKeyTime = static_cast<FLOAT>(InKeys[KeyIndex].Time);
				const FLOAT NextKeyTime = static_cast<FLOAT>(InKeys[KeyIndex + 1].Time);
				if (InKeyTime < NextKeyTime)
				{
					const FLOAT TimeRange = NextKeyTime - CurrentKeyTime;
					const FLOAT Alpha = (TimeRange > 1e-6f) ? ((InKeyTime - CurrentKeyTime) / TimeRange) : 0.0f;
					OutValue = InLerp(InKeys[KeyIndex].Value, InKeys[KeyIndex + 1].Value, EMath::Clamp(Alpha, 0.0f, 1.0f));
					return TRUE;
				}
			}

			OutValue = InKeys[KeyNum - 1].Value;
			return TRUE;
		}

		FLOAT GetClipKeyTime(const ESkeletonAnimationClip* InClip, FLOAT InPlaybackTimeSeconds)
		{
			if ((!InClip) || (InClip->Duration <= 0.0f))
			{
				return 0.0f;
			}

			const FLOAT TicksPerSecond = (InClip->TicksPerSecond > 1e-3f) ? InClip->TicksPerSecond : 30.0f;
			FLOAT KeyTime = InPlaybackTimeSeconds * TicksPerSecond;
			KeyTime = std::fmod(KeyTime, InClip->Duration);
			if (KeyTime < 0.0f)
			{
				KeyTime += InClip->Duration;
			}
			return KeyTime;
		}
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
	}

	void EAssetPreviewScene::Shutdown()
	{
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
		if (m_PreviewRenderer)
		{
			m_PreviewRenderer->InitNewFrame();
			static_cast<RAssetPreviewSceneRenderer*>(m_PreviewRenderer)->RenderPreview();
		}
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
		if (!m_PreviewRenderer)
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
			m_SkeletalMeshComponent->SetComponentRotation(MakeQuaternion(Vector3::XVector(), EMath::DegreesToRadians(90.0f)));
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

		const EBoundAABB& Bound = PreviewMesh->GetBoundAABB();
		const Vector3 Center = (Bound.AABBMin + Bound.AABBMax) * 0.5f;
		const Vector3 Extent = (Bound.AABBMax - Bound.AABBMin) * 0.5f;
		const FLOAT MaxExtent = EMath::Max(Extent.x, EMath::Max(Extent.y, Extent.z));
		m_CameraDistance = EMath::Max(MaxExtent * 3.0f, 120.0f);

		m_PreviewActor->SetActorLocation(m_ZoneOrigin - Center);
		m_PreviewActor->SetActorScale(Vector3::One());
		UpdateCameraTransform();
	}

	void EAssetPreviewScene::UpdateCameraTransform()
	{
		EnsureCameraActor();
		if (!m_CameraActor)
		{
			return;
		}

		const Vector3 FocusPoint = m_ZoneOrigin;
		const FLOAT CameraHeight = EMath::Max(m_CameraDistance * 0.35f, 40.0f);
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
			}

			const FLOAT KeyTime = GetClipKeyTime(m_AnimationClip, m_PlaybackTimeSeconds);
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
				SampleAnimationKey(KeyTime, Curve.PositionKeys, BoneTransform.Position, VectorLerp);
				SampleAnimationKey(KeyTime, Curve.RotationKeys, BoneTransform.Rotation, QuaternionLerp);
				SampleAnimationKey(KeyTime, Curve.ScalingKeys, BoneTransform.Scaling, VectorLerp);
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