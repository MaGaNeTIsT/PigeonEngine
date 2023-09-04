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
		if (OutPrimitiveMapping.ContainsKey(InPrimitiveProxy->GetUniqueID()))
		{

		}
		return FALSE;
	}
	static BOOL32 RemovePrimitiveInternal(RPrimitiveProxy* InPrimitiveProxy, TMap<ObjectIdentityType, UINT32>& OutPrimitiveMapping, TArray<RPrimitiveProxy*>& OutPrimitiveArray)
	{
		return FALSE;
	}

	RScene::RScene()
	{
	}
	RScene::~RScene()
	{
	}

};