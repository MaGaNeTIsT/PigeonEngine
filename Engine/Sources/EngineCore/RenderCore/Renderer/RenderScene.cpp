#include "RenderScene.h"
#include "RenderProxy/PrimitiveProxy.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RScene, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	static BOOL32 AddPrimitiveInternal(RPrimitiveProxy* InPrimitiveProxy, TMap<ObjectIdentityType, UINT32>& OutPrimitiveMapping, TArray<RPrimitiveProxy*>& OutPrimitiveArray)
	{
		if (!InPrimitiveProxy)
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive mapping and array length failed."), (OutPrimitiveMapping.Length() == OutPrimitiveArray.Length()));
		const ObjectIdentityType& PrimitiveID = InPrimitiveProxy->GetUniqueID();
		if (OutPrimitiveMapping.ContainsKey(PrimitiveID))
		{
			return FALSE;
		}
		OutPrimitiveMapping.Add(PrimitiveID, OutPrimitiveArray.Length());
		OutPrimitiveArray.Add(InPrimitiveProxy);
		return TRUE;
	}
	static BOOL32 RemovePrimitiveInternal(RPrimitiveProxy* InPrimitiveProxy, TMap<ObjectIdentityType, UINT32>& OutPrimitiveMapping, TArray<RPrimitiveProxy*>& OutPrimitiveArray)
	{
		if ((!InPrimitiveProxy) || (OutPrimitiveArray.Length() == 0u))
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive mapping and array length failed."), (OutPrimitiveMapping.Length() == OutPrimitiveArray.Length()));
		const ObjectIdentityType& PrimitiveID = InPrimitiveProxy->GetUniqueID();
		UINT32 PrimitiveIndex = (UINT32)(-1);
		if (!(OutPrimitiveMapping.FindValue(PrimitiveID, PrimitiveIndex)))
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive index failed."), (PrimitiveIndex < OutPrimitiveArray.Length()));
		if (const UINT32 LastIndex = OutPrimitiveArray.Length() - 1u; PrimitiveIndex != LastIndex)
		{
			RPrimitiveProxy* TempPrimitiveProxy = OutPrimitiveArray[LastIndex];
			OutPrimitiveArray[LastIndex] = OutPrimitiveArray[PrimitiveIndex];
			OutPrimitiveArray[PrimitiveIndex] = TempPrimitiveProxy;
			OutPrimitiveMapping[TempPrimitiveProxy->GetUniqueID()] = PrimitiveIndex;
		}
		OutPrimitiveMapping.Remove(PrimitiveID);
		OutPrimitiveArray.Pop();
		return TRUE;
	}

	RScene::RScene()
	{
	}
	RScene::~RScene()
	{
	}
	BOOL32 RScene::AddStaticPrimitive(RPrimitiveProxy* InPrimitiveProxy)
	{
		return AddPrimitiveInternal(InPrimitiveProxy, StaticPrimitiveMapping, StaticPrimitives);
	}
	BOOL32 RScene::RemoveStaticPrimitive(RPrimitiveProxy* InPrimitiveProxy)
	{
		return RemovePrimitiveInternal(InPrimitiveProxy, StaticPrimitiveMapping, StaticPrimitives);
	}
	BOOL32 RScene::AddDynamicPrimitive(RPrimitiveProxy* InPrimitiveProxy)
	{
		return AddPrimitiveInternal(InPrimitiveProxy, DynamicPrimitiveMapping, DynamicPrimitives);
	}
	BOOL32 RScene::RemoveDynamicPrimitive(RPrimitiveProxy* InPrimitiveProxy)
	{
		return RemovePrimitiveInternal(InPrimitiveProxy, DynamicPrimitiveMapping, DynamicPrimitives);
	}

};