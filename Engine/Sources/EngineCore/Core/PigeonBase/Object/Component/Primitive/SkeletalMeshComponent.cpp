#include "SkeletalMeshComponent.h"
#include <Animation/AnimInstance.h>
#include <AnimationAsset/AnimationAsset.h>
#include <Asset/AssetLoadManager.h>
#include <Base/Timer/Timer.h>
#include <Development/Profiler.h>
#include <MeshAsset/MeshAsset.h>
#include <MultiThread/Atomic.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/Property.h>
#include <Renderer/RenderInterface.h>
#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{
	namespace
	{
		struct FSkeletalMeshAssetNames
		{
			EString Mesh;
			EString Skeleton;
		};

		struct FAnimationRequest
		{
			EString Asset;
			EString Clip;
			INT32 LoopMode = static_cast<INT32>(ANIMATION_LOOP_REPEAT);
			FLOAT PlayRate = 1.0f;
		};

		void SerializeSkeletalMeshAssetNames(EArchive& InArchive, const CHAR* InName, FSkeletalMeshAssetNames& InOutValue)
		{
			InArchive.BeginObject(InName);
			InArchive.SerializeString(PE_TEXT("Mesh"), InOutValue.Mesh);
			InArchive.SerializeString(PE_TEXT("Skeleton"), InOutValue.Skeleton);
			InArchive.EndObject();
		}

		void SerializeAnimationRequest(EArchive& InArchive, const CHAR* InName, FAnimationRequest& InOutValue)
		{
			InArchive.BeginObject(InName);
			InArchive.SerializeString(PE_TEXT("Asset"), InOutValue.Asset);
			InArchive.SerializeString(PE_TEXT("Clip"), InOutValue.Clip);
			SerializeEnumValue(InArchive, PE_TEXT("LoopMode"), InOutValue.LoopMode, GetClassHashCode<EAnimationLoopMode>());
			InArchive.SerializeFloat(PE_TEXT("PlayRate"), InOutValue.PlayRate);
			InArchive.EndObject();
		}

		void RegisterClassProperties()
		{
			const size_t ClassHash = GetClassHashCode<PSkeletalMeshComponent>();
			// Mesh and skeleton are one property: RequestSkeletalMeshAssets needs both names
			// and binds them as a pair, so separate properties would race on load order.
			AddClassPropertyCustom<PSkeletalMeshComponent, FSkeletalMeshAssetNames>(ClassHash, PE_TEXT("MeshAssets"),
				[](const PSkeletalMeshComponent* Component) -> FSkeletalMeshAssetNames
				{
					FSkeletalMeshAssetNames Names;
					Names.Mesh = Component->GetMeshAssetName();
					Names.Skeleton = Component->GetSkeletonAssetName();
					return Names;
				},
				[](PSkeletalMeshComponent* Component, const FSkeletalMeshAssetNames& Value) -> void
				{
					if ((Value.Mesh.Length() > 0u) || (Value.Skeleton.Length() > 0u))
					{
						Component->RequestSkeletalMeshAssets(Value.Mesh, Value.Skeleton, nullptr);
					}
				},
				SerializeSkeletalMeshAssetNames,
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyCustom<PSkeletalMeshComponent, FAnimationRequest>(ClassHash, PE_TEXT("Animation"),
				[](const PSkeletalMeshComponent* Component) -> FAnimationRequest
				{
					FAnimationRequest Request;
					Request.Asset = Component->GetAnimationAssetName();
					Request.Clip = Component->GetAnimationClipName();
					Request.LoopMode = static_cast<INT32>(Component->GetAnimationRequestedLoopMode());
					Request.PlayRate = Component->GetAnimationPlayRate();
					return Request;
				},
				[](PSkeletalMeshComponent* Component, const FAnimationRequest& Value) -> void
				{
					if (Value.Asset.Length() > 0u)
					{
						Component->RequestAnimation(Value.Asset, Value.Clip, static_cast<EAnimationLoopMode>(Value.LoopMode), Value.PlayRate);
					}
				},
				SerializeAnimationRequest,
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassBoolPropertyAccessors<PSkeletalMeshComponent>(ClassHash, PE_TEXT("RootMotionEnabled"),
				[](const PSkeletalMeshComponent* Component) -> BOOL32 { return Component->IsRootMotionEnabled(); },
				[](PSkeletalMeshComponent* Component, const BOOL32& Value) -> void { Component->SetRootMotionEnabled(Value); },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			AddClassPropertyAccessors<PSkeletalMeshComponent, INT32>(ClassHash, PE_TEXT("RootMotionBoneIndex"),
				[](const PSkeletalMeshComponent* Component) -> INT32 { return Component->GetRootMotionBoneIndex(); },
				[](PSkeletalMeshComponent* Component, const INT32& Value) -> void { Component->SetRootMotionBoneIndex(Value); },
				static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
			// Editor views onto the serialized MeshAssets pair. The pair stays the file format,
			// while the picker edits one side at a time; the request keeps the other side until
			// both names are present, then binds them together.
			const UINT32 AssetEditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE)
				| static_cast<UINT32>(EPropertyFlagsType::EDITOR_ONLY);
			AddClassAssetPropertyAccessors<PSkeletalMeshComponent>(ClassHash, PE_TEXT("MeshAsset"),
				[](const PSkeletalMeshComponent* Component) -> EString { return Component->GetMeshAssetName(); },
				[](PSkeletalMeshComponent* Component, const EString& Value) -> void
				{
					if (Value.Length() > 0u)
					{
						Component->RequestSkeletalMeshAssets(Value, Component->GetSkeletonAssetName(), nullptr);
					}
					else
					{
						Component->ClearSkeletalMeshAssets();
					}
				},
				EPropertyAssetType::SKELETAL_MESH,
				AssetEditFlags);
			AddClassAssetPropertyAccessors<PSkeletalMeshComponent>(ClassHash, PE_TEXT("SkeletonAsset"),
				[](const PSkeletalMeshComponent* Component) -> EString { return Component->GetSkeletonAssetName(); },
				[](PSkeletalMeshComponent* Component, const EString& Value) -> void
				{
					if (Value.Length() > 0u)
					{
						Component->RequestSkeletalMeshAssets(Component->GetMeshAssetName(), Value, nullptr);
					}
					else
					{
						Component->ClearSkeletalMeshAssets();
					}
				},
				EPropertyAssetType::SKELETON,
				AssetEditFlags);
		}
	}

	// Characters whose animation Update ran since the last consume. Written and read on the
	// game thread today; atomic so a future parallel evaluation can keep the same profiling
	// hook. File-local: only ConsumeAnimatedCharacterNum publishes it.
	static TAtomic<UINT32> AnimatedCharacterNumCounter(0u);

	static void RegisterClassTypes()
	{
		RegisterClassType<PSkeletalMeshComponent, PMeshComponent>();
		RegisterClassProperties();
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
	const ESkinnedMeshAsset* PSkeletalMeshComponent::GetMeshAsset() const
	{
		return MeshAsset;
	}
	const EString& PSkeletalMeshComponent::GetMeshAssetName() const
	{
		if (MeshAsset)
		{
			return MeshAsset->GetAssetName();
		}
		return RequestedMeshAssetName;
	}
	const EString& PSkeletalMeshComponent::GetSkeletonAssetName() const
	{
		if (SkeletonAsset)
		{
			return SkeletonAsset->GetAssetName();
		}
		return RequestedSkeletonAssetName;
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
		PE_PROFILE_SCOPE(PROFILER_SCOPE_ANIM_SNAPSHOT);

		if (!PMeshComponent::BuildRenderSnapshot(OutSnapshot))
		{
			return FALSE;
		}
		// The bone matrices are animation state, so they are re-reported with every snapshot
		// rather than cached: the pool is what the animation system writes into.
		#if (SKELETON_USE_MATRIX_FOR_BONE_TO_ROOT)
		const TArray<Matrix4x4>& BoneToRootTransforms = BoneMemoryPool.GetBoneToRootTransforms();
		#else
		const TArray<EBoneTransform>& BoneToRootTransforms = BoneMemoryPool.GetBoneToRootTransforms();
		#endif
		const UINT32 MatrixNum = BoneToRootTransforms.Num<UINT32>();
		if (MatrixNum > 0u)
		{
			OutSnapshot.BoneToRootMatrices.SetNum(MatrixNum);
			for (UINT32 MatrixIndex = 0u; MatrixIndex < MatrixNum; MatrixIndex++)
			{
#if (SKELETON_USE_MATRIX_FOR_BONE_TO_ROOT)
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
#if ENGINE_EDITOR_ONLY
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
	const ESkeletonAsset* PSkeletalMeshComponent::GetSkeletonAsset() const
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
		RequestedMeshAssetName = InMeshAssetName;
		RequestedSkeletonAssetName = InSkeletonAssetName;
		// The pair is loaded together. While one side is still empty, keep the requested names
		// so the other picker can complete the pair on the next edit instead of issuing a load
		// request for an empty asset name every frame.
		if ((InMeshAssetName.Length() == 0u) || (InSkeletonAssetName.Length() == 0u))
		{
			return;
		}
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

	void PSkeletalMeshComponent::ClearSkeletalMeshAssets()
	{
		RequestedMeshAssetName = EString();
		RequestedSkeletonAssetName = EString();
		SetMeshAsset(nullptr);
		SetSkeletonAsset(nullptr);
		SetLocalBound(EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE));
		SetMaterialSlotsNum(0u);
	}

	// Animation START
	const ESkeletonAnimationAsset* PSkeletalMeshComponent::GetAnimationAsset() const
	{
		return (AnimInstance ? AnimInstance->GetAnimationAsset() : nullptr);
	}
	EAnimInstance* PSkeletalMeshComponent::GetAnimInstance() const
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
	void PSkeletalMeshComponent::RequestAnimation(const EString& InAnimationAssetName, const EString& InClipName
		, EAnimationLoopMode InLoopMode, FLOAT InPlayRate)
	{
		RequestedAnimationAssetName = InAnimationAssetName;
		RequestedAnimationClipName = InClipName;
		RequestedAnimationLoopMode = InLoopMode;
		RequestedAnimationPlayRate = InPlayRate;
		// With a named clip the request must not auto-play clip 0: the deferred step below
		// selects the authored clip once the asset is bound.
		RequestAnimationAsset(InAnimationAssetName, nullptr, (InClipName.Length() == 0u) ? TRUE : FALSE);

		PSkeletalMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, InClipName, InLoopMode, InPlayRate]() -> BOOL32
		{
			if (!Component->GetAnimationAsset())
			{
				return FALSE;
			}
			if (InClipName.Length() == 0u)
			{
				// The asset request auto-played clip 0; only the exact loop mode is left.
				Component->SetAnimationLoopMode(InLoopMode);
				return TRUE;
			}
			if (!Component->PlayAnimation(InClipName, TRUE, InPlayRate, 0.0f))
			{
				PE_LOG_WARN(EString(PE_TEXT("Skeletal mesh: animation clip not found: ")) + InClipName);
				return TRUE;	// the asset is there; retrying would give the same answer
			}
			Component->SetAnimationLoopMode(InLoopMode);
			return TRUE;
		};
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}
	EString PSkeletalMeshComponent::GetAnimationAssetName() const
	{
		const ESkeletonAnimationAsset* AnimationAsset = GetAnimationAsset();
		return AnimationAsset ? AnimationAsset->GetAssetName() : RequestedAnimationAssetName;
	}
	EString PSkeletalMeshComponent::GetAnimationClipName() const
	{
		const ESkeletonAnimationClip* Clip = AnimInstance ? AnimInstance->GetCurrentClip() : nullptr;
		return Clip ? Clip->ClipName : RequestedAnimationClipName;
	}
	EAnimationLoopMode PSkeletalMeshComponent::GetAnimationRequestedLoopMode() const
	{
		return AnimInstance ? AnimInstance->GetLoopMode() : RequestedAnimationLoopMode;
	}
	FLOAT PSkeletalMeshComponent::GetAnimationPlayRate() const
	{
		return AnimInstance ? AnimInstance->GetPlayRate() : RequestedAnimationPlayRate;
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
	EAnimationLoopMode PSkeletalMeshComponent::GetAnimationLoopMode() const
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
	EAnimStateMachineInstance* PSkeletalMeshComponent::GetStateMachineInstance() const
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
	void PSkeletalMeshComponent::StopMontage(const EString& InSlotName, FLOAT InBlendOutSeconds)
	{
		if (AnimInstance)
		{
			AnimInstance->StopMontage(InSlotName, InBlendOutSeconds);
		}
	}
	void PSkeletalMeshComponent::PauseMontage()
	{
		if (AnimInstance)
		{
			AnimInstance->PauseMontage();
		}
	}
	void PSkeletalMeshComponent::PauseMontage(const EString& InSlotName)
	{
		if (AnimInstance)
		{
			AnimInstance->PauseMontage(InSlotName);
		}
	}
	void PSkeletalMeshComponent::ResumeMontage()
	{
		if (AnimInstance)
		{
			AnimInstance->ResumeMontage();
		}
	}
	void PSkeletalMeshComponent::ResumeMontage(const EString& InSlotName)
	{
		if (AnimInstance)
		{
			AnimInstance->ResumeMontage(InSlotName);
		}
	}
	void PSkeletalMeshComponent::SetMontagePositionSeconds(FLOAT InPositionSeconds)
	{
		if (AnimInstance)
		{
			AnimInstance->SetMontagePositionSeconds(InPositionSeconds);
		}
	}
	void PSkeletalMeshComponent::SetMontagePlayRate(FLOAT InPlayRate)
	{
		if (AnimInstance)
		{
			AnimInstance->SetMontagePlayRate(InPlayRate);
		}
	}
	void PSkeletalMeshComponent::SetMontageLoopMode(EAnimationLoopMode InLoopMode)
	{
		if (AnimInstance)
		{
			AnimInstance->SetMontageLoopMode(InLoopMode);
		}
	}
	BOOL32 PSkeletalMeshComponent::IsMontagePlaying() const
	{
		return (AnimInstance ? AnimInstance->IsMontagePlaying() : FALSE);
	}
	BOOL32 PSkeletalMeshComponent::IsMontagePlaying(const EString& InSlotName) const
	{
		return (AnimInstance ? AnimInstance->IsMontagePlaying(InSlotName) : FALSE);
	}
	EAnimMontageInstance* PSkeletalMeshComponent::GetMontageInstance() const
	{
		return (AnimInstance ? AnimInstance->GetMontageInstance() : nullptr);
	}
	EAnimMontageInstance* PSkeletalMeshComponent::GetMontageInstanceBySlot(const EString& InSlotName) const
	{
		return (AnimInstance ? AnimInstance->GetMontageInstanceBySlot(InSlotName) : nullptr);
	}
	INT32 PSkeletalMeshComponent::GetMontageInstanceCount() const
	{
		return (AnimInstance ? AnimInstance->GetMontageInstanceCount() : 0);
	}
	EAnimMontageInstance* PSkeletalMeshComponent::GetMontageInstanceAt(INT32 InIndex) const
	{
		return (AnimInstance ? AnimInstance->GetMontageInstanceAt(InIndex) : nullptr);
	}
	// Montage END

	// Root motion START
	void PSkeletalMeshComponent::SetRootMotionEnabled(BOOL32 bInEnabled)
	{
		if (AnimInstance)
		{
			AnimInstance->SetRootMotionEnabled(bInEnabled);
		}
	}
	BOOL32 PSkeletalMeshComponent::IsRootMotionEnabled() const
	{
		return (AnimInstance ? AnimInstance->IsRootMotionEnabled() : FALSE);
	}
	void PSkeletalMeshComponent::SetRootMotionBoneIndex(INT32 InBoneIndex)
	{
		if (AnimInstance)
		{
			AnimInstance->SetRootMotionBoneIndex(InBoneIndex);
		}
	}
	INT32 PSkeletalMeshComponent::GetRootMotionBoneIndex() const
	{
		return (AnimInstance ? AnimInstance->GetRootMotionBoneIndex() : 0);
	}
	// Root motion END

	const ESkeletonBoneMemoryPool& PSkeletalMeshComponent::GetBoneMemoryPool() const
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
			// An editor build runs World::Tick and then World::EditorTick, and both walk the
			// component tree through TickRender. Advancing twice would play every clip at
			// double speed, so the same frame index is rejected here. A zero index means no
			// update has happened yet (or there is no timer), and is deliberately not stamped.
			const UINT64 FrameIndex = (GameTimer ? GameTimer->GetFrameIndex() : 0u);
			const BOOL32 bAlreadyAdvanced = (((FrameIndex != 0u) && (FrameIndex == LastAnimationFrameIndex)) ? TRUE : FALSE);
			if (!bAlreadyAdvanced)
			{
				if (FrameIndex != 0u)
				{
					LastAnimationFrameIndex = FrameIndex;
				}
				AnimatedCharacterNumCounter.FetchAdd(1u);
				const FLOAT DeltaSeconds = (GameTimer ? static_cast<FLOAT>(GameTimer->GetDeltaTime()) : 0.0f);
				if (AnimInstance->Update(DeltaSeconds))
				{
					MarkBoneMemoryPoolDirty();
				}
			}
		}
		if (IsRenderStateDirty())
		{
			SendUpdateRenderState();
		}
		for (PSceneComponent* Component : ChildrenComponents)
		{
			Component->TickRender();
		}
	}

	UINT32 PSkeletalMeshComponent::ConsumeAnimatedCharacterNum()
	{
		return AnimatedCharacterNumCounter.Exchange(0u);
	}

	// Render proxy functions START
	void PSkeletalMeshComponent::CreateRenderState()
	{
		PMeshComponent::CreateRenderState();
		if (ShouldRender())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->AddSkeletalMesh(this);
				}
			}
		}
	}
	void PSkeletalMeshComponent::DestroyRenderState()
	{
		// A component that never joined a world - or whose world already released its
		// render scene - has nothing registered; the guard turns that teardown into a
		// no-op instead of a null dereference.
		if (PWorld* World = this->GetWorld())
		{
			if (RSceneInterface* RenderScene = World->GetRenderScene())
			{
				RenderScene->RemoveSkeletalMesh(this);
			}
		}
		PMeshComponent::DestroyRenderState();
	}
	void PSkeletalMeshComponent::SendUpdateRenderState()
	{
		if (ShouldRender() && IsRenderStateDirty())
		{
			if (PWorld* World = this->GetWorld())
			{
				if (RSceneInterface* RenderScene = World->GetRenderScene())
				{
					RenderScene->UpdateSkeletalMesh(this);
				}
			}
		}
		PMeshComponent::SendUpdateRenderState();
	}
	// Render proxy functions END

}
