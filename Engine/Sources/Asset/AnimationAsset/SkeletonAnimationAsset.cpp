#include "SkeletonAnimationAsset.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<ESkeletonAnimation, EObjectBase, EResourceInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	ESkeletonAnimation::ESkeletonAnimation(const EString& InMeshName)
		: AnimationName(InMeshName)
	{
	}
	ESkeletonAnimation::~ESkeletonAnimation()
	{
		ReleaseResource();
	}
	BOOL ESkeletonAnimation::IsResourceValid()const
	{
		//TODO
		return FALSE;
	}
	BOOL ESkeletonAnimation::InitResource()
	{
		//TODO
		return FALSE;
	}
	void ESkeletonAnimation::ReleaseResource()
	{
		//TODO
	}

};