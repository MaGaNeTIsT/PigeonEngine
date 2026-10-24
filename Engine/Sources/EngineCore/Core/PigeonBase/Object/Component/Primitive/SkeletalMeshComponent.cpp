#include "SkeletalMeshComponent.h"
#include <Animation/AnimInstance.h>
#include <AnimationAsset/AnimationAsset.h>
#include <Asset/AssetLoadManager.h>
#include <Base/Timer/Timer.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/World/World.h>
#include <Renderer/RenderInterface.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PSkeletalMeshComponent, PMeshComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PSkeletalMeshComponent::PSkeletalMeshComponent()
		: MeshAsset(nullptr), SkeletonAsset(nullptr), AnimInstance(nullptr)
	{
		POBJ_DEBUGNAME_SET(this, "SkeletalMeshComponent");
		// The instance is created with the component so the animation API is always safe to
		// call, even before an asset is assigned. It is a plain runtime object, not a PObject,
		// because it does not need the object graph or a tick of its own: the component drives
		// its Update from TickRenderImpl.
		AnimInstance = new EAnimInstance();
		AnimInstance->SetSkeletalMeshComponent(this);
	}
	PSkeletalMeshComponent::~PSkeletalMeshComponent()
	{
		// The proxy belongs to the render scene and is deleted by the remove command, so the
		// component never holds a pointer to it. The animation instance is owned here.
		if (AnimInstance)
		{
			delete AnimInstance;
			AnimInstance = nullptr;
		}
	}
	const ESkinnedMeshAsset* PSkeletalMeshComponent::GetMeshAsset()const
	{
		return MeshAsset;
	}
	const EName& PSkeletalMeshComponent::GetRenderProxyTypeName() const
	{
		// Has to match the name RSkeletalMeshSceneProxy registers itself with, because that is
		// what the render side looks the proxy class up by.
		static const EName ProxyTypeName(PE_TEXT("SkeletalMeshPrimitive"));
		return ProxyTypeName;
	}
	void PSkeletalMeshComponent::CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const
	{
		// The mesh base reports the mesh and the material slots; a skinned primitive adds its
		// skeleton, which is what its skin streams are indexed against.
		PMeshComponent::CollectRenderAssets(InOutReport);
		InOutReport.SetSkeletonAsset(SkeletonAsset);
	}
	BOOL32 PSkeletalMeshComponent::BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const
	{
		if (!PMeshComponent::BuildRenderSnapshot(OutSnapshot))
		{
			return FALSE;
		}
		// The bone matrices are animation state, so they are re-reported with every snapshot
		// rather than cached: the pool is what the animation system writes into.
		const auto& BoneToRootTransforms = BoneMemoryPool.GetBoneToRootTransforms();
		const UINT32 MatrixNum = BoneToRootTransforms.Num<UINT32>();
		if (MatrixNum > 0u)
		{
			OutSnapshot.BoneToRootMatrices.SetNum(MatrixNum);
			for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
			{
#if (_USE_MATRIX_FOR_BONE_TO_ROOT)
				OutSnapshot.BoneToRootMatrices[MatrixIndex] = BoneToRootTransforms[MatrixIndex];
#else
				OutSnapshot.BoneToRootMatrices[MatrixIndex] = BoneToRootTransforms[MatrixIndex].ToMatrix4x4();
#endif
			}
		}
		return TRUE;
	}
	void PSkeletalMeshComponent::SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset)
	{
		MeshAsset = InMeshAsset;
		SetMeshAssetBase(InMeshAsset);
		if (!!InMeshAsset)
		{
			const ESkinnedMesh* Mesh = InMeshAsset->GetStoragedResource();
#if _EDITOR_ONLY
			PE_CHECK((ENGINE_RENDER_CORE_ERROR), ("Skinned mesh asset does not contain mesh resource."), (!!Mesh));
			if (Mesh)
#endif
			{
				SetLocalBound(Mesh->GetBoundAABB());
				SetMaterialSlotsNum(Mesh->GetSubmeshPart().Num<UINT32>());
			}
		}
		// A different mesh is an asset level change: the render side re-resolves the geometry and
		// the pass slots rather than only rewriting values.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	const ESkeletonAsset* PSkeletalMeshComponent::GetSkeletonAsset()const
	{
		return SkeletonAsset;
	}
	void PSkeletalMeshComponent::SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset)
	{
		BOOL32 NeedRegenerate = (SkeletonAsset != InSkeletonAsset);
		SkeletonAsset = InSkeletonAsset;
		if (NeedRegenerate)
		{
			if (!!SkeletonAsset)
			{
				if (!!MeshAsset)
				{
					MeshAsset->SetSkeleton(SkeletonAsset->GetStoragedResource());
				}
				if (SkeletonAsset->IsResourceValid())
				{
					BoneMemoryPool.GenerateFromSkeleton(SkeletonAsset->GetStoragedResource());
				}
			}
			else
			{
				BoneMemoryPool.GenerateFromSkeleton(nullptr);
			}
			// The binding resolves curve names against this skeleton, so it has to be rebuilt
			// whenever the skeleton changes, including when it is cleared. The instance keeps
			// the current clip if the new skeleton can still serve it.
			if (AnimInstance)
			{
				AnimInstance->OnSkeletonChanged();
			}
		}
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_ASSETS);
	}
	void PSkeletalMeshComponent::RequestSkeletalMeshAssets(const EString& InMeshAssetName, const EString& InSkeletonAssetName
		, const FAssetImportSource* InImportSource)
	{
		// Copied by value: the binding outlives this call.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}
		PSkeletalMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InMeshAssetName, InSkeletonAssetName, ImportSource]() -> BOOL32
		{
			const ESkinnedMeshAsset* MeshAsset = nullptr;
			const ESkeletonAsset* SkeletonAsset = nullptr;
			// Both are asked for on every attempt, so neither asset waits for the other.
			const BOOL32 bHasMesh = EMeshAssetManager::GetManagerSingleton()->RequireLoadSkinnedMeshAsset(
				EBaseSettings::ENGINE_MESH_PATH, InMeshAssetName, MeshAsset, &ImportSource);
			const BOOL32 bHasSkeleton = ESkeletonAssetManager::GetManagerSingleton()->RequireLoadSkeletonAsset(
				EBaseSettings::ENGINE_SKELETON_PATH, InSkeletonAssetName, SkeletonAsset, &ImportSource);
			if ((!bHasMesh) || (!bHasSkeleton))
			{
				return FALSE;	// not loaded yet: the component retries on its next tick
			}
			// Order matters and the component knows it: SetSkeletonAsset is what points the mesh at its
			// skeleton and rebuilds the bone pool, so the mesh goes in first. The pool's root transform
			// is what the pool is generated against, so it is set before that happens.
			Component->GetBoneMemoryPool().SetRootTransform(Vector3::Zero(), Quaternion::Identity(), Vector3::One());
			Component->SetMeshAsset(MeshAsset);
			Component->SetSkeletonAsset(SkeletonAsset);
			return TRUE;
		};
		// Tried once here as well, so an already loaded pair is applied before this call returns.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}

	// Animation START
	const ESkeletonAnimationAsset* PSkeletalMeshComponent::GetAnimationAsset()const
	{
		return (AnimInstance ? AnimInstance->GetAnimationAsset() : nullptr);
	}
	EAnimInstance* PSkeletalMeshComponent::GetAnimInstance()const
	{
		return AnimInstance;
	}
	BOOL32 PSkeletalMeshComponent::SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset)
	{
		return (AnimInstance ? AnimInstance->SetAnimationAsset(InAnimationAsset) : FALSE);
	}
	void PSkeletalMeshComponent::RequestAnimationAsset(const EString& InAnimationAssetName
		, const FAssetImportSource* InImportSource, BOOL32 bInAutoPlayFirstClip)
	{
		// Copied by value: the request outlives this call.
		FAssetImportSource ImportSource;
		if (InImportSource)
		{
			ImportSource = *InImportSource;
		}

		PSkeletalMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InAnimationAssetName, ImportSource, bInAutoPlayFirstClip]() -> BOOL32
		{
			const ESkeletonAnimationAsset* LoadedAsset = nullptr;
			const BOOL32 bHasAnimation = EAnimationManager::GetManagerSingleton()->RequireLoadSkeletonAnimationAsset(
				EBaseSettings::ENGINE_ANIMATION_PATH, InAnimationAssetName, LoadedAsset, &ImportSource);
			if (!bHasAnimation)
			{
				return FALSE;
			}
			// A binding needs both sides. The mesh/skeleton request is retried on its own, so
			// waiting here is enough; this step simply runs again next frame.
			if (!Component->GetSkeletonAsset())
			{
				return FALSE;
			}
			// The bind can still fail when the animation and the skeleton share no bone at all.
			// The asset is remembered either way, so a later skeleton swap can retry it, but
			// the deferred step is done: retrying an incompatible pair every frame would not
			// change the answer.
			const BOOL32 bBound = Component->SetAnimationAsset(LoadedAsset);
			if ((bBound) && (bInAutoPlayFirstClip))
			{
				Component->PlayAnimation(0);
			}
			return TRUE;
		};

		// Tried once here as well, so an already loaded asset and skeleton bind before this
		// call returns.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}
	BOOL32 PSkeletalMeshComponent::PlayAnimation(const EString& InClipName, BOOL32 bInLoop, FLOAT InPlayRate, FLOAT InFadeSeconds)
	{
		return (AnimInstance ? AnimInstance->Play(InClipName, bInLoop, InPlayRate, InFadeSeconds) : FALSE);
	}
	BOOL32 PSkeletalMeshComponent::PlayAnimation(INT32 InClipIndex, BOOL32 bInLoop, FLOAT InPlayRate, FLOAT InFadeSeconds)
	{
		return (AnimInstance ? AnimInstance->Play(InClipIndex, bInLoop, InPlayRate, InFadeSeconds) : FALSE);
	}
	void PSkeletalMeshComponent::PauseAnimation()
	{
		if (AnimInstance)
		{
			AnimInstance->Pause();
		}
	}
	void PSkeletalMeshComponent::ResumeAnimation()
	{
		if (AnimInstance)
		{
			AnimInstance->Resume();
		}
	}
	void PSkeletalMeshComponent::StopAnimation()
	{
		if (AnimInstance)
		{
			AnimInstance->Stop();
		}
	}
	void PSkeletalMeshComponent::SetAnimationPlayRate(FLOAT InPlayRate)
	{
		if (AnimInstance)
		{
			AnimInstance->SetPlayRate(InPlayRate);
		}
	}
	void PSkeletalMeshComponent::SetAnimationLoop(BOOL32 bInLoop)
	{
		if (AnimInstance)
		{
			AnimInstance->SetLoop(bInLoop);
		}
	}
	void PSkeletalMeshComponent::SetAnimationLoopMode(EAnimationLoopMode InLoopMode)
	{
		if (AnimInstance)
		{
			AnimInstance->SetLoopMode(InLoopMode);
		}
	}
	EAnimationLoopMode PSkeletalMeshComponent::GetAnimationLoopMode()const
	{
		return (AnimInstance ? AnimInstance->GetLoopMode() : ANIMATION_LOOP_ONCE);
	}
	void PSkeletalMeshComponent::SetAnimationPositionSeconds(FLOAT InPositionSeconds)
	{
		if (AnimInstance)
		{
			AnimInstance->SetPositionSeconds(InPositionSeconds);
		}
	}
	// Animation END

	// State machine START
	BOOL32 PSkeletalMeshComponent::SetStateMachine(const EAnimStateMachine* InMachine)
	{
		return (AnimInstance ? AnimInstance->SetStateMachine(InMachine) : FALSE);
	}
	void PSkeletalMeshComponent::ClearStateMachine()
	{
		if (AnimInstance)
		{
			AnimInstance->ClearStateMachine();
		}
	}
	EAnimStateMachineInstance* PSkeletalMeshComponent::GetStateMachineInstance()const
	{
		return (AnimInstance ? AnimInstance->GetStateMachineInstance() : nullptr);
	}
	// State machine END

	// Montage START
	BOOL32 PSkeletalMeshComponent::PlayMontage(const EAnimMontage* InMontage, const EString& InSectionName, FLOAT InPlayRate)
	{
		return (AnimInstance ? AnimInstance->PlayMontage(InMontage, InSectionName, InPlayRate) : FALSE);
	}
	void PSkeletalMeshComponent::StopMontage(FLOAT InBlendOutSeconds)
	{
		if (AnimInstance)
		{
			AnimInstance->StopMontage(InBlendOutSeconds);
		}
	}
	void PSkeletalMeshComponent::PauseMontage()
	{
		if (AnimInstance)
		{
			AnimInstance->PauseMontage();
		}
	}
	void PSkeletalMeshComponent::ResumeMontage()
	{
		if (AnimInstance)
		{
			AnimInstance->ResumeMontage();
		}
	}
	BOOL32 PSkeletalMeshComponent::IsMontagePlaying()const
	{
		return (AnimInstance ? AnimInstance->IsMontagePlaying() : FALSE);
	}
	EAnimMontageInstance* PSkeletalMeshComponent::GetMontageInstance()const
	{
		return (AnimInstance ? AnimInstance->GetMontageInstance() : nullptr);
	}
	// Montage END

	const ESkeletonBoneMemoryPool& PSkeletalMeshComponent::GetBoneMemoryPool()const
	{
		return BoneMemoryPool;
	}
	ESkeletonBoneMemoryPool& PSkeletalMeshComponent::GetBoneMemoryPool()
	{
		return BoneMemoryPool;
	}
	void PSkeletalMeshComponent::MarkBoneMemoryPoolDirty()
	{
		// Bone matrices are a value level change: the render side rewrites the bone buffers and
		// leaves the pass slots alone. Whoever writes the pool - the animation update - is the
		// one that has to call this, because nothing else can see that the matrices moved.
		MarkRenderChannelsDirty(PRIMITIVE_UPDATE_VALUES);
	}

	void PSkeletalMeshComponent::TickRenderImpl()
	{
		// Evaluate before the dirty check below. PActor::Tick calls TickRender before the
		// component tick, so this is the last point in the frame where the bone pool can be
		// written and still be sent with this frame's snapshot. The delta is the same one the
		// world is ticking with; a non-tickable actor keeps its pose instead of advancing.
		if ((AnimInstance) && (GetOwnerActor()) && (GetOwnerActor()->IsTickable()))
		{
			const PWorld* World = GetWorld();
			const EGameTimer* GameTimer = (World ? World->GetGameTimer() : nullptr);
			const FLOAT DeltaSeconds = (GameTimer ? static_cast<FLOAT>(GameTimer->GetDeltaTime()) : 0.0f);
			if (AnimInstance->Update(DeltaSeconds))
			{
				MarkBoneMemoryPoolDirty();
			}
		}
		if (IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
		for (const auto& elem : ChildrenComponents)
		{
			elem->TickRender();
		}
	}

	// Render proxy functions START
	void PSkeletalMeshComponent::CreateRenderState()
	{
		PMeshComponent::CreateRenderState();
		if (ShouldRender())
		{
			this->GetWorld()->GetRenderScene()->AddSkeletalMesh(this);
		}
	}
	void PSkeletalMeshComponent::DestroyRenderState()
	{
		this->GetWorld()->GetRenderScene()->RemoveSkeletalMesh(this);
		PMeshComponent::DestroyRenderState();
	}
	void PSkeletalMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			this->GetWorld()->GetRenderScene()->UpdateSkeletalMesh(this);
		}
		PMeshComponent::SendUpdateRenderState();
	}
	// Render proxy functions END

}
