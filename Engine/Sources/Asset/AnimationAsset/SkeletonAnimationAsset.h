#pragma once

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include <EngineCommon.h>
#include <BaseAsset.h>

namespace PigeonEngine
{

	enum EAnimationBehaviourType
	{
		ANIMATION_BEHAVIOUR_DEFAULT		= 0,
		ANIMATION_BEHAVIOUR_CONSTANT	= 1,
		ANIMATION_BEHAVIOUR_LINEAR		= 2,
		ANIMATION_BEHAVIOUR_REPEAT		= 3
	};
	struct EBoneAnimationCurve : public ETransformTimeCurve<FLOAT>
	{
		EBoneAnimationCurve()noexcept : BoneName(ENGINE_DEFAULT_NAME), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EBoneAnimationCurve(const EString& InBoneName)noexcept : BoneName(InBoneName), PreState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT), PostState(EAnimationBehaviourType::ANIMATION_BEHAVIOUR_DEFAULT) {}
		EString									BoneName;
		EAnimationBehaviourType					PreState;
		EAnimationBehaviourType					PostState;
	};
	struct ESkeletonAnimationClip
	{
		ESkeletonAnimationClip()noexcept : ClipName(ENGINE_DEFAULT_NAME), Duration(0.f), TicksPerSecond(0.f) {}
		ESkeletonAnimationClip(const EString& InClipName)noexcept : ClipName(InClipName), Duration(0.f), TicksPerSecond(0.f) {}
		EString							ClipName;
		FLOAT							Duration;
		FLOAT							TicksPerSecond;
		TArray<EBoneAnimationCurve>		AnimationCurves;
	};
	class ESkeletonAnimation : public EObjectBase, public EResourceInterface
	{

		EClass(ESkeletonAnimation, EObjectBase)

	public:
		ESkeletonAnimation(const EString& InMeshName);
		virtual ~ESkeletonAnimation();
		virtual BOOL IsValid()const override;
		virtual void Release()override;
	protected:
		EString							AnimationName;
		TArray<ESkeletonAnimationClip>	AnimationClips;
	public:
		ESkeletonAnimation() = delete;

		CLASS_REMOVE_COPY_BODY(ESkeletonAnimation)

	};

};