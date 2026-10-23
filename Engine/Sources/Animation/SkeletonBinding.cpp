#include "SkeletonBinding.h"

#include <AnimationAsset/AnimationAsset.h>
#include <Editor/EditorLogManager.h>
#include <SkeletonAsset/SkeletonAsset.h>

namespace PigeonEngine
{

	BOOL32 ESkeletonBinding::Bind(const ESkeleton* InSkeleton, const ESkeletonAnimation* InAnimation)
	{
		Unbind();
		if ((!InSkeleton) || (!InAnimation))
		{
			return FALSE;
		}
		if ((!InSkeleton->IsResourceValid()) || (!InAnimation->IsResourceValid()))
		{
			return FALSE;
		}

		Skeleton = InSkeleton;
		Animation = InAnimation;

		// The recorded skeleton name is a compatibility hint, not a hard link: a retargeted or
		// renamed skeleton can still serve the clip when the bone names line up. A mismatch is
		// worth surfacing because it usually means the wrong skeleton was picked.
		const EString& AuthoredSkeletonName = InAnimation->GetSkeletonName();
		const EString& TargetSkeletonName = InSkeleton->GetSkeletonName();
		if ((AuthoredSkeletonName.Length() > 0u)
			&& (AuthoredSkeletonName != EString(EEngineSettings::ENGINE_DEFAULT_NAME))
			&& (AuthoredSkeletonName != TargetSkeletonName))
		{
			PE_LOG_WARN(EString(PE_TEXT("Skeleton animation '")) + InAnimation->GetAnimationName()
				+ EString(PE_TEXT("' was authored against skeleton '")) + AuthoredSkeletonName
				+ EString(PE_TEXT("' but is bound to '")) + TargetSkeletonName + EString(PE_TEXT("'.")));
		}

		const TArray<ESkeletonAnimationClip>& AnimationClips = InAnimation->GetAnimationClips();
		ClipCurveBoneIndices.SetNum(AnimationClips.Num());

		INT32 BoundCurveNum = 0;
		for (INT32 ClipIndex = 0; ClipIndex < AnimationClips.Num(); ClipIndex++)
		{
			const ESkeletonAnimationClip& AnimationClip = AnimationClips[ClipIndex];
			TArray<INT32>& CurveBoneIndices = ClipCurveBoneIndices[ClipIndex];
			CurveBoneIndices.SetNum(AnimationClip.AnimationCurves.Num());

			for (INT32 CurveIndex = 0; CurveIndex < AnimationClip.AnimationCurves.Num(); CurveIndex++)
			{
				const EBoneAnimationCurve& Curve = AnimationClip.AnimationCurves[CurveIndex];
				USHORT BoneIndex = 0u;
				if (InSkeleton->GetBoneMapping().FindValue(Curve.BoneName, BoneIndex))
				{
					CurveBoneIndices[CurveIndex] = static_cast<INT32>(BoneIndex);
					BoundCurveNum++;
				}
				else
				{
					CurveBoneIndices[CurveIndex] = -1;
					MissingBoneNum++;
				}
			}
		}

		// A binding with no usable curve cannot animate anything. Treating it as a failed
		// bind is better than letting the player run a clip that writes nothing.
		if (BoundCurveNum <= 0)
		{
			Unbind();
			return FALSE;
		}

		if (MissingBoneNum > 0)
		{
			PE_LOG_WARN(EString(PE_TEXT("Skeleton animation '")) + InAnimation->GetAnimationName()
				+ EString(PE_TEXT("' has ")) + ToString(MissingBoneNum)
				+ EString(PE_TEXT(" curve(s) whose bone does not exist in skeleton '"))
				+ InSkeleton->GetSkeletonName() + EString(PE_TEXT("'.")));
		}
		return TRUE;
	}

	void ESkeletonBinding::Unbind()
	{
		Skeleton = nullptr;
		Animation = nullptr;
		ClipCurveBoneIndices.Empty();
		MissingBoneNum = 0;
	}

	BOOL32 ESkeletonBinding::IsValid()const
	{
		return (((Skeleton != nullptr) && (Animation != nullptr) && (ClipCurveBoneIndices.Num() > 0)) ? TRUE : FALSE);
	}

	const ESkeleton* ESkeletonBinding::GetSkeleton()const
	{
		return Skeleton;
	}

	const ESkeletonAnimation* ESkeletonBinding::GetAnimation()const
	{
		return Animation;
	}

	INT32 ESkeletonBinding::GetMissingBoneNum()const
	{
		return MissingBoneNum;
	}

	INT32 ESkeletonBinding::GetBoneIndex(INT32 InClipIndex, INT32 InCurveIndex)const
	{
		if ((InClipIndex < 0) || (InClipIndex >= ClipCurveBoneIndices.Num()))
		{
			return -1;
		}
		const TArray<INT32>& CurveBoneIndices = ClipCurveBoneIndices[InClipIndex];
		if ((InCurveIndex < 0) || (InCurveIndex >= CurveBoneIndices.Num()))
		{
			return -1;
		}
		return CurveBoneIndices[InCurveIndex];
	}

}
