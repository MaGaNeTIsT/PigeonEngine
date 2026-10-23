#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	/*
	* Stateless helpers shared by the runtime animation player and the editor preview.
	*
	* A curve's keys are sorted by time when the asset is imported. Sampling is a binary
	* search followed by a blend between the two neighbouring keys, so the cost is
	* logarithmic in the number of keys and independent of how far playback has advanced.
	*
	* The curve's PreState/PostState values are deliberately not interpreted here: looping,
	* clamping and one-shot behaviour are the player's decision, because only the player
	* knows how the clip is being played.
	*/
	class EAnimationSampler
	{
	public:
		/*
		* Writes the value of InKeys at InKeyTime into OutValue.
		*
		* InLerp is called as InLerp(Value0, Value1, Alpha). The caller supplies it so a
		* position/scale curve can use Vector3::Lerp while a rotation curve uses
		* Quaternion::SLerp without this class knowing which is which.
		*
		* Returns FALSE only when the curve has no keys, in which case OutValue is left
		* untouched and the caller keeps whatever base pose it started from.
		*/
		template<typename TValueType, typename TKeyType, typename TLerpType>
		static BOOL32 SampleKey(FLOAT InKeyTime, const TArray<TKeyType>& InKeys, TValueType& OutValue, const TLerpType& InLerp)
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
			if (InKeyTime >= static_cast<FLOAT>(InKeys[KeyNum - 1].Time))
			{
				OutValue = InKeys[KeyNum - 1].Value;
				return TRUE;
			}

			// Find the last key whose time is not greater than InKeyTime. The two bounds
			// start at the ends and close in, which is valid because the keys are sorted.
			INT32 LowIndex = 0;
			INT32 HighIndex = KeyNum - 1;
			while ((LowIndex + 1) < HighIndex)
			{
				const INT32 MidIndex = (LowIndex + HighIndex) / 2;
				if (static_cast<FLOAT>(InKeys[MidIndex].Time) <= InKeyTime)
				{
					LowIndex = MidIndex;
				}
				else
				{
					HighIndex = MidIndex;
				}
			}

			const FLOAT CurrentKeyTime = static_cast<FLOAT>(InKeys[LowIndex].Time);
			const FLOAT NextKeyTime = static_cast<FLOAT>(InKeys[HighIndex].Time);
			const FLOAT TimeRange = NextKeyTime - CurrentKeyTime;
			const FLOAT Alpha = (TimeRange > 1e-6f) ? ((InKeyTime - CurrentKeyTime) / TimeRange) : 0.0f;
			OutValue = InLerp(InKeys[LowIndex].Value, InKeys[HighIndex].Value, EMath::Clamp(Alpha, 0.0f, 1.0f));
			return TRUE;
		}

		/*
		* Wraps a key time into [0, InDuration). Used by repeating playback; a negative
		* result from a negative play rate is wrapped back into the positive range rather
		* than clamped, so reverse looping keeps working.
		*/
		static FLOAT WrapKeyTime(FLOAT InKeyTime, FLOAT InDuration);
	};

}
