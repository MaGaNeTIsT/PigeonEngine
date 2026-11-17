#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <AnimationAsset/AnimationAsset.h>

namespace PigeonEngine
{

	/*
	* Read-only key plots shared by the animation editors. Both callers draw the same curve
	* data at their own size; keeping the plotting here avoids a second copy drifting from
	* the one the montage editor has used since its first version.
	*/
	class EAnimationCurvePlot
	{
	public:
		static void DrawVectorKeys(const CHAR* InLabel
			, const TArray<EBoneAnimationCurve::TPositionKey>& InKeys
			, FLOAT InTimeMax, FLOAT InWidth, FLOAT InHeight);
		static void DrawRotationKeys(const TArray<EBoneAnimationCurve::TRotationKey>& InKeys
			, FLOAT InTimeMax, FLOAT InWidth, FLOAT InHeight);
	};

}

#endif
