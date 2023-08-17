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
	}
	BOOL ESkeletonAnimation::IsValid()const
	{
		return FALSE;
	}
	void ESkeletonAnimation::Release()
	{
	}

};