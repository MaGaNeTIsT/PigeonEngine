#include "SkeletonAsset.h"

namespace PigeonEngine
{

	ESkeleton::ESkeleton(const EString& InSkeletonName)
		: SkeletonName(InSkeletonName)
	{
#ifdef _EDITOR_ONLY
		DebugName = InSkeletonName;
#endif
	}
	ESkeleton::~ESkeleton()
	{

	}
	void ESkeleton::Release()
	{

	}

};