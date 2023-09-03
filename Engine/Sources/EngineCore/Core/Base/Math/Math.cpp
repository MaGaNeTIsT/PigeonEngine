#include "Math.h"

namespace PigeonEngine
{

	static INT32 _GSRandSeed;

	void EMath::SRandInit(const INT32 InSeed)
	{
		_GSRandSeed = InSeed;
	}
	INT32 EMath::GetRandSeed()
	{
		return _GSRandSeed;
	}
	FLOAT EMath::SRand()
	{
		_GSRandSeed = (_GSRandSeed * 196314165) + 907633515;
		union { FLOAT f; INT32 i; } Result;
		union { FLOAT f; INT32 i; } Temp;
		const FLOAT SRandTemp = 1.0f;
		Temp.f = SRandTemp;
		Result.i = (Temp.i & 0xff800000) | (_GSRandSeed & 0x007fffff);
		return Fractional(Result.f);
	}

};