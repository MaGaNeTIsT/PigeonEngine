#pragma once

#include <CoreMinimal.h>

namespace PigeonEngine
{

	class ESkeleton;
	class ESkeletonAnimation;
	struct ESkeletonAnimationClip;

	/*
	* The runtime link between one skeleton and one animation asset.
	*
	* An animation asset authors its curves by bone name, while the sampler writes into a
	* bone-indexed pose buffer. Resolving those names on every frame would put a hash lookup
	* on every curve of every character; this object resolves them once, when the pair is
	* bound, and keeps one index array per clip.
	*
	* A curve whose bone does not exist in the target skeleton is stored as -1 and skipped
	* by the player. That makes a partial or slightly different skeleton usable instead of
	* failing the whole animation, and the missing count is kept for the editor and for the
	* warning emitted at bind time.
	*/
	class ESkeletonBinding
	{
	public:
		/*
		* Resolves every curve of every clip against InSkeleton. Returns FALSE when either
		* side is invalid or when no curve at all could be bound, in which case the binding
		* is left empty.
		*/
		BOOL32 Bind(const ESkeleton* InSkeleton, const ESkeletonAnimation* InAnimation);
		void Unbind();

		PE_NODISCARD BOOL32 IsValid()const;
		PE_NODISCARD const ESkeleton* GetSkeleton()const;
		PE_NODISCARD const ESkeletonAnimation* GetAnimation()const;
		PE_NODISCARD INT32 GetMissingBoneNum()const;

		/*
		* The skeleton bone index driven by InCurveIndex of the clip at InClipIndex, or -1
		* when that curve has no matching bone. Both indices are the same indices the
		* animation asset itself uses, so a player can keep a clip index and a curve loop.
		*/
		PE_NODISCARD INT32 GetBoneIndex(INT32 InClipIndex, INT32 InCurveIndex)const;

	private:
		const ESkeleton*			Skeleton = nullptr;
		const ESkeletonAnimation*	Animation = nullptr;
		// One array per clip, parallel to ESkeletonAnimationClip::AnimationCurves.
		TArray<TArray<INT32>>		ClipCurveBoneIndices;
		INT32						MissingBoneNum = 0;
	};

}
