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
		SkeletonName = ENGINE_DEFAULT_NAME;
		Bones.Clear();
		BoneMapping.Clear();
	}
	BOOL ESkeleton::AddBoneElement(EBoneData* InIndexData)
	{
		if (!InIndexData)
		{
			return FALSE;
		}
		USHORT TempIndex = 0u;
		if (BoneMapping.FindValue(InIndexData->Name, TempIndex))
		{
			return FALSE;
		}
		const UINT NewIndex = Bones.Length();
		Bones.Add(EBoneData());
		BoneMapping.Add(InIndexData->Name, static_cast<USHORT>(NewIndex));
		Bones[NewIndex].Index			= static_cast<SHORT>(NewIndex);
		Bones[NewIndex].Name			= InIndexData->Name;
		Bones[NewIndex].DefaultPosition	= InIndexData->DefaultPosition;
		Bones[NewIndex].DefaultRotation	= InIndexData->DefaultRotation;
		Bones[NewIndex].DefaultScaling	= InIndexData->DefaultScaling;
		Bones[NewIndex].BindPoseMatrix	= InIndexData->BindPoseMatrix;
		Bones[NewIndex].Parent			= static_cast<SHORT>(InIndexData->Parent);
		for (UINT ChildIndex = 0u, ChildrenNum = InIndexData->Children.Length(); ChildIndex < ChildrenNum; ChildIndex++)
		{
			Bones[NewIndex].Children.Add(InIndexData->Children[ChildIndex]);
		}
		return TRUE;
	}
	BOOL ESkeleton::RemoveBoneElement(const EString& InBoneName)
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Try to removing bone from empty skeleton."));
			return FALSE;
		}
		USHORT TempIndex = 0u;
		if (!BoneMapping.FindValue(InBoneName, TempIndex))
		{
#ifdef _DEBUG_MODE
			EString ErrorInfo("Try to removing not exist bone [");
			ErrorInfo = ErrorInfo + InBoneName + "] in skeleton [";
			ErrorInfo = ErrorInfo + SkeletonName + "].";
			PE_FAILED(EString(ENGINE_ASSET_ERROR), ErrorInfo);
#endif
			return FALSE;
		}
#ifdef _DEBUG_MODE
		if (TempIndex >= Bones.Length())
		{
			PE_FAILED(EString(ENGINE_ASSET_ERROR), EString("Check error that index out of bones list."));
		}
#endif
		if (TempIndex == (Bones.Length() - 1u))
		{
			//TODO
		}
	}
	BOOL ESkeleton::RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName)
	{

	}
	BOOL ESkeleton::GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const
	{

	}
	BOOL ESkeleton::GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const
	{

	}

};