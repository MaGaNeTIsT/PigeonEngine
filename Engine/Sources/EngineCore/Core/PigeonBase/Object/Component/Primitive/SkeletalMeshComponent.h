#pragma once

#include <CoreMinimal.h>
#include "MeshComponent.h"
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;
	class ESkeletonAnimationAsset;
	class EAnimInstance;
	class EAnimStateMachine;
	class EAnimStateMachineInstance;
	class EAnimMontage;
	class EAnimMontageInstance;
	enum EAnimationLoopMode : UINT8;

	/*
	* A skinned mesh primitive. Its material slots and its mesh handle live on the mesh base;
	* what is left here is the mesh and skeleton handles, the bone pool the animation writes
	* into, and the reporting that turns those into a snapshot.
	*/
	class PSkeletalMeshComponent : public PMeshComponent
	{

		CLASS_VIRTUAL_NOCOPY_BODY(PSkeletalMeshComponent)

	public:
		const ESkinnedMeshAsset*        GetMeshAsset()const;
		void                            SetMeshAsset(const ESkinnedMeshAsset* InMeshAsset);
		const ESkeletonAsset*           GetSkeletonAsset()const;
		void                            SetSkeletonAsset(const ESkeletonAsset* InSkeletonAsset);
		const ESkeletonBoneMemoryPool&  GetBoneMemoryPool()const;
		ESkeletonBoneMemoryPool&        GetBoneMemoryPool();
		// Call this after writing the bone matrices, which is what the animation update does.
		// Nothing else can see that they moved, so without this call the render side keeps the
		// previous pose.
		void                            MarkBoneMemoryPoolDirty();
		/*
		* Game thread: binds a skinned mesh and its skeleton to this component without waiting for them.
		* Both are asked for now - an already loaded pair is applied before this call returns - and the
		* binding is retried every frame until both exist, because the mesh's skin stream is indexed
		* against the skeleton and a half applied pair could not be drawn anyway. The bone pool is
		* rebuilt from the skeleton by SetSkeletonAsset, which is why the mesh goes in first.
		*/
		void                            RequestSkeletalMeshAssets(const EString& InMeshAssetName, const EString& InSkeletonAssetName
			, const FAssetImportSource* InImportSource = nullptr);

		// Animation START
	public:
		PE_NODISCARD const ESkeletonAnimationAsset* GetAnimationAsset()const;
		PE_NODISCARD EAnimInstance*                 GetAnimInstance()const;
		/*
		* Game thread: binds an already loaded animation asset to this component's skeleton.
		* Returns FALSE when either side is not loaded or no curve could be bound; the asset
		* is still remembered, so a later skeleton change retries the bind.
		*/
		BOOL32                                      SetAnimationAsset(const ESkeletonAnimationAsset* InAnimationAsset);
		/*
		* Game thread: asks the animation manager for an asset without waiting. The request
		* is retried every frame until the asset and this component's skeleton are both
		* present, then the asset is applied. InImportSource is the editor fallback that
		* imports the raw FBX when the PAsset is not on disk yet.
		*
		* bInAutoPlayFirstClip plays clip 0 once the bind succeeds, which is what a demo or
		* a simple prop wants. A caller that selects its clip by name passes FALSE and calls
		* PlayAnimation after the asset arrives.
		*/
		void                                        RequestAnimationAsset(const EString& InAnimationAssetName
			, const FAssetImportSource* InImportSource = nullptr, BOOL32 bInAutoPlayFirstClip = FALSE);

		// InFadeSeconds blends from the clip that is currently playing to the new one.
		// Zero switches immediately; a positive value crossfades over that many seconds.
		BOOL32                                      PlayAnimation(const EString& InClipName, BOOL32 bInLoop = TRUE, FLOAT InPlayRate = 1.0f, FLOAT InFadeSeconds = 0.0f);
		BOOL32                                      PlayAnimation(INT32 InClipIndex, BOOL32 bInLoop = TRUE, FLOAT InPlayRate = 1.0f, FLOAT InFadeSeconds = 0.0f);
		void                                        PauseAnimation();
		void                                        ResumeAnimation();
		void                                        StopAnimation();
		void                                        SetAnimationPlayRate(FLOAT InPlayRate);
		// Convenience form: TRUE maps to REPEAT, FALSE maps to ONCE.
		void                                        SetAnimationLoop(BOOL32 bInLoop);
		void                                        SetAnimationLoopMode(EAnimationLoopMode InLoopMode);
		PE_NODISCARD EAnimationLoopMode             GetAnimationLoopMode()const;
		void                                        SetAnimationPositionSeconds(FLOAT InPositionSeconds);
		// Animation END

		// State machine START
	public:
		// Binds an already parsed state machine. Its clip names are resolved against the
		// animation asset currently bound to this component.
		BOOL32                                      SetStateMachine(const EAnimStateMachine* InMachine);
		void                                        ClearStateMachine();
		PE_NODISCARD EAnimStateMachineInstance*     GetStateMachineInstance()const;
		// State machine END

		// Montage START
	public:
		BOOL32                                      PlayMontage(const EAnimMontage* InMontage
			, const EString& InSectionName = EString(), FLOAT InPlayRate = 1.0f);
		void                                        StopMontage(FLOAT InBlendOutSeconds = -1.0f);
		void                                        StopMontage(const EString& InSlotName, FLOAT InBlendOutSeconds = -1.0f);
		void                                        PauseMontage();
		void                                        PauseMontage(const EString& InSlotName);
		void                                        ResumeMontage();
		void                                        ResumeMontage(const EString& InSlotName);
		PE_NODISCARD BOOL32                         IsMontagePlaying()const;
		PE_NODISCARD BOOL32                         IsMontagePlaying(const EString& InSlotName)const;
		PE_NODISCARD EAnimMontageInstance*          GetMontageInstance()const;
		PE_NODISCARD EAnimMontageInstance*          GetMontageInstanceBySlot(const EString& InSlotName)const;
		PE_NODISCARD INT32                          GetMontageInstanceCount()const;
		PE_NODISCARD EAnimMontageInstance*          GetMontageInstanceAt(INT32 InIndex)const;
		// Montage END

		// Root motion START
	public:
		void                                        SetRootMotionEnabled(BOOL32 bInEnabled);
		PE_NODISCARD BOOL32                         IsRootMotionEnabled()const;
		void                                        SetRootMotionBoneIndex(INT32 InBoneIndex);
		PE_NODISCARD INT32                          GetRootMotionBoneIndex()const;
		// Root motion END

	protected:
		const ESkinnedMeshAsset*        MeshAsset;
		const ESkeletonAsset*           SkeletonAsset;
		EAnimInstance*                  AnimInstance;
		ESkeletonBoneMemoryPool         BoneMemoryPool;

		// What this primitive reports to the render tier START
	public:
		// The name the render side builds this component's proxy under. It has to match the one
		// RSkeletalMeshSceneProxy registers itself with.
		PE_NODISCARD virtual const EName& GetRenderProxyTypeName() const override;
		// The mesh base reports the mesh and the material slots; a skinned primitive adds its
		// skeleton.
		virtual void CollectRenderAssets(FPrimitiveRenderAssetReport& InOutReport) const override;
		// The base fills identity, transform, flags and the two reports. A skinned primitive
		// adds the one thing that is neither an asset nor a shader value: where its bones are.
		PE_NODISCARD virtual BOOL32 BuildRenderSnapshot(FPrimitiveRenderSnapshot& OutSnapshot) const override;
		// What this primitive reports to the render tier END

	protected:
		virtual void TickRenderImpl() override;

		// Render proxy functions START
	public:
		virtual void                CreateRenderState()override;
		virtual void                DestroyRenderState()override;
		virtual void                SendUpdateRenderState()override;
		// Render proxy functions END
	};

};
