#include "SkeletonAsset.h"

namespace PigeonEngine
{

	static void BreakBoneRelation(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex)
	{
		SHORT BoneNum = static_cast<SHORT>(InOutDatas.Length());
		EBoneData& BoneData = InOutDatas[InBoneIndex];
		if (BoneData.Parent >= 0 && BoneData.Parent < BoneNum)
		{
			InOutDatas[BoneData.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex));
		}
		BoneData.Parent = -2;
		for (UINT ChildIndex = 0u, ChildNum = BoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData.Children[ChildIndex] >= 0 && BoneData.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData.Children[ChildIndex]].Parent = -2;
			}
		}
		BoneData.Children.Clear();
	}
	static void ExchangeBoneData(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex0, USHORT InBoneIndex1)
	{
		SHORT BoneNum = static_cast<SHORT>(InOutDatas.Length());
		EBoneData& BoneData0 = InOutDatas[InBoneIndex0];
		EBoneData& BoneData1 = InOutDatas[InBoneIndex1];
		if (BoneData0.Parent >= 0 && BoneData0.Parent < BoneNum)
		{
			InOutDatas[BoneData0.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex0));
			InOutDatas[BoneData0.Parent].Children.Add(static_cast<SHORT>(InBoneIndex1));
		}
		if (BoneData1.Parent >= 0 && BoneData1.Parent < BoneNum)
		{
			InOutDatas[BoneData1.Parent].Children.Remove(static_cast<SHORT>(InBoneIndex1));
			InOutDatas[BoneData1.Parent].Children.Add(static_cast<SHORT>(InBoneIndex0));
		}
		for (UINT ChildIndex = 0u, ChildNum = BoneData0.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData0.Children[ChildIndex] >= 0 && BoneData0.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData0.Children[ChildIndex]].Parent = static_cast<SHORT>(InBoneIndex1);
			}
		}
		for (UINT ChildIndex = 0u, ChildNum = BoneData1.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
		{
			if (BoneData1.Children[ChildIndex] >= 0 && BoneData1.Children[ChildIndex] < BoneNum)
			{
				InOutDatas[BoneData1.Children[ChildIndex]].Parent = static_cast<SHORT>(InBoneIndex0);
			}
		}
		EBoneData TempBoneData;
		TempBoneData.Index = InBoneIndex0;
		TempBoneData.Name = BoneData0.Name;
		TempBoneData.DefaultPosition = BoneData0.DefaultPosition;
		TempBoneData.DefaultRotation = BoneData0.DefaultRotation;
		TempBoneData.DefaultScaling = BoneData0.DefaultScaling;
		TempBoneData.Parent = BoneData0.Parent;
		if (BoneData0.Children.Length() > 0u)
		{
			TempBoneData.Children.Recapacity(BoneData0.Children.Length());
			for (UINT ChildIndex = 0u, ChildNum = BoneData0.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				if (BoneData0.Children[ChildIndex] >= 0 && BoneData0.Children[ChildIndex] < BoneNum)
				{
					TempBoneData.Children.Add(BoneData0.Children[ChildIndex]);
				}
			}
		}
		BoneData0.Index = InBoneIndex1;
		BoneData0.Name = BoneData1.Name;
		BoneData0.DefaultPosition = BoneData1.DefaultPosition;
		BoneData0.DefaultRotation = BoneData1.DefaultRotation;
		BoneData0.DefaultScaling = BoneData1.DefaultScaling;
		BoneData0.Parent = BoneData1.Parent;
		BoneData0.Children.Clear();
		if (BoneData1.Children.Length() > 0)
		{
			BoneData0.Children.Recapacity(BoneData1.Children.Length());
			for (UINT ChildIndex = 0u, ChildNum = BoneData1.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				if (BoneData1.Children[ChildIndex] >= 0 && BoneData1.Children[ChildIndex] < BoneNum)
				{
					BoneData0.Children.Add(BoneData1.Children[ChildIndex]);
				}
			}
		}
		BoneData1.Index = InBoneIndex0;
		BoneData1.Name = TempBoneData.Name;
		BoneData1.DefaultPosition = TempBoneData.DefaultPosition;
		BoneData1.DefaultRotation = TempBoneData.DefaultRotation;
		BoneData1.DefaultScaling = TempBoneData.DefaultScaling;
		BoneData1.Parent = TempBoneData.Parent;
		BoneData1.Children.Clear();
		if (TempBoneData.Children.Length() > 0u)
		{
			BoneData1.Children.Recapacity(TempBoneData.Children.Length());
			for (UINT ChildIndex = 0u, ChildNum = TempBoneData.Children.Length(); ChildIndex < ChildNum; ChildIndex++)
			{
				BoneData1.Children.Add(TempBoneData.Children[ChildIndex]);
			}
		}
	}
	static void RecursionRemoveBone(ESkeleton::EBonePart& InOutDatas, USHORT InBoneIndex)
	{
		USHORT CurrentBoneNum = static_cast<USHORT>(InOutDatas.Length());
		if (CurrentBoneNum > 0u)
		{
			if (InBoneIndex >= CurrentBoneNum)
			{
				return;
			}

			USHORT LastBoneIndex = CurrentBoneNum - 1u;
			if (InBoneIndex != (CurrentBoneNum - 1u))
			{
				ExchangeBoneData(InOutDatas, InBoneIndex, LastBoneIndex);
			}

			TArray<SHORT> TempChildren;
			if (InOutDatas[InBoneIndex].Children.Length() > 0u)
			{
				SHORT TempBoneNum = static_cast<SHORT>(CurrentBoneNum);
				TempChildren.Resize(InOutDatas[InBoneIndex].Children.Length());
				for (UINT ChildIndex = 0u, ChildNum = InOutDatas[InBoneIndex].Children.Length(); ChildIndex < ChildNum; ChildIndex++)
				{
					SHORT TempChildIndex = InOutDatas[InBoneIndex].Children[ChildIndex];
					if (TempChildIndex >= 0 && TempChildIndex < TempBoneNum)
					{
						TempChildren[ChildIndex] = TempChildIndex;
					}
					else
					{
						TempChildren[ChildIndex] = -2;
					}
				}
			}

			{
				BreakBoneRelation(InOutDatas, InBoneIndex);
				InOutDatas.RemoveAt(LastBoneIndex);
			}

			if (TempChildren.Length() > 0u)
			{
				for (UINT ChildIndex = 0u, ChildNum = TempChildren.Length(); ChildIndex < ChildNum; ChildIndex++)
				{
					if (TempChildren[ChildIndex] < 0)
					{
						continue;
					}
					RecursionRemoveBone(InOutDatas, static_cast<USHORT>(TempChildren[ChildIndex]));
				}
			}
		}
	}

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
	BOOL ESkeleton::IsValid()const
	{
		return ((Bones.Length() > 0u) && (Bones.Length() == BoneMapping.Length()));
	}
	void ESkeleton::Release()
	{
		SkeletonName = ENGINE_DEFAULT_NAME;
		Bones.Clear();
		BoneMapping.Clear();
	}
	UINT ESkeleton::GetBoneCount()const
	{
		return (this->Bones.Length());
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
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
		USHORT TempIndex = 0u;
		if (!BoneMapping.FindValue(InBoneName, TempIndex))
		{
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
			EString ErrorInfo("Try to removing not exist bone [");
			ErrorInfo = ErrorInfo + InBoneName + "] in skeleton [";
			ErrorInfo = ErrorInfo + SkeletonName + "].";
			PE_FAILED((ENGINE_ASSET_ERROR), ErrorInfo);
#endif
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (TempIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		RemoveBoneInternal(TempIndex);
		return TRUE;
	}
	BOOL ESkeleton::RemoveBoneElement(USHORT InBoneIndex, EString* OutBoneName)
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (InBoneIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		if (OutBoneName)
		{
			(*OutBoneName) = Bones[InBoneIndex].Name;
		}
		RemoveBoneInternal(InBoneIndex);
		return TRUE;
	}
	BOOL ESkeleton::GetBoneElement(const EString& InBoneName, const EBoneData*& OutBoneData)const
	{
		USHORT TempIndex = 0u;
		if (!BoneMapping.FindValue(InBoneName, TempIndex))
		{
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
			EString ErrorInfo("Try to getting not exist bone [");
			ErrorInfo = ErrorInfo + InBoneName + "] in skeleton [";
			ErrorInfo = ErrorInfo + SkeletonName + "].";
			PE_FAILED((ENGINE_ASSET_ERROR), ErrorInfo);
#endif
			return FALSE;
		}
		return (GetBoneElement(TempIndex, OutBoneData));
	}
	BOOL ESkeleton::GetBoneElement(USHORT InBoneIndex, const EBoneData*& OutBoneData)const
	{
		if (Bones.Length() == 0u)
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Try to removing bone from empty skeleton."));
			return FALSE;
		}
#if (defined(_DEBUG_MODE) || defined(_DEVELOP_MODE))
		if (InBoneIndex >= Bones.Length())
		{
			PE_FAILED((ENGINE_ASSET_ERROR), ("Check error that index out of bones list."));
			return FALSE;
		}
#endif
		OutBoneData = static_cast<const EBoneData*>(&(Bones[InBoneIndex]));
		return TRUE;
	}
	void ESkeleton::RemoveBoneInternal(USHORT InBoneIndex)
	{
		RecursionRemoveBone(this->Bones, InBoneIndex);
	}

	ESkeletonRenderResource::ESkeletonRenderResource(ESkeleton* InSkeleton)
		: Skeleton(InSkeleton)
	{

	}
	ESkeletonRenderResource::~ESkeletonRenderResource()
	{
		Release();
	}
	void ESkeletonRenderResource::Release()
	{
		Skeleton = nullptr;
		if (RenderResources.Length() > 0)
		{
			for (UINT Index = 0u, Length = RenderResources.Length(); Index < Length; Index++)
			{
				RenderResources[Index].Release();
			}
			RenderResources.Clear();
		}
	}

	ESkeletonAsset::ESkeletonAsset(const EString& InSkeletonPath
#ifdef _EDITOR_ONLY
		, const EString& InDebugName
#endif
	) : TRenderBaseAsset<ESkeleton, ESkeletonRenderResource>(
#ifdef _EDITOR_ONLY
		InDebugName
#endif
	), SkeletonPath(InSkeletonPath)
	{

	}
	ESkeletonAsset::~ESkeletonAsset()
	{

	}
	const EString& ESkeletonAsset::GetSkeletonPath()const
	{
		return SkeletonPath;
	}
	BOOL ESkeletonAsset::InitResource()
	{
		return FALSE;
	}

};