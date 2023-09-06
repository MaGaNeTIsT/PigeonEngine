#include "RenderScene.h"
#include "RenderProxy/PrimitiveSceneProxy.h"
#include <PigeonBase/Object/Component/PrimitiveComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RCommand, ERegisterBase>();
		RegisterClassType<RScene, EObjectBase, RSceneInterface>();
		RegisterClassType<RSceneNULL, EObjectBase, RSceneInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	static BOOL32 AddPrimitiveInternal(RPrimitiveSceneProxy* InSceneProxy, TMap<ObjectIdentityType, UINT32>& OutPrimitiveMapping, TArray<RPrimitiveSceneProxy*>& OutPrimitiveArray)
	{
		if (!InSceneProxy)
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive mapping and array length failed."), (OutPrimitiveMapping.Length() == OutPrimitiveArray.Length()));
		const ObjectIdentityType& PrimitiveID = InSceneProxy->GetUniqueID();
		if (OutPrimitiveMapping.ContainsKey(PrimitiveID))
		{
			return FALSE;
		}
		OutPrimitiveMapping.Add(PrimitiveID, OutPrimitiveArray.Length());
		OutPrimitiveArray.Add(InSceneProxy);
		return TRUE;
	}
	static BOOL32 RemovePrimitiveInternal(RPrimitiveSceneProxy* InSceneProxy, TMap<ObjectIdentityType, UINT32>& OutPrimitiveMapping, TArray<RPrimitiveSceneProxy*>& OutPrimitiveArray)
	{
		if ((!InSceneProxy) || (OutPrimitiveArray.Length() == 0u))
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive mapping and array length failed."), (OutPrimitiveMapping.Length() == OutPrimitiveArray.Length()));
		const ObjectIdentityType& PrimitiveID = InSceneProxy->GetUniqueID();
		UINT32 PrimitiveIndex = (UINT32)(-1);
		if (!(OutPrimitiveMapping.FindValue(PrimitiveID, PrimitiveIndex)))
		{
			return FALSE;
		}
		Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive index failed."), (PrimitiveIndex < OutPrimitiveArray.Length()));
		if (const UINT32 LastIndex = OutPrimitiveArray.Length() - 1u; PrimitiveIndex != LastIndex)
		{
			RPrimitiveSceneProxy* TempPrimitiveProxy = OutPrimitiveArray[LastIndex];
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
		UnbindErrorCheck();
	}
	void RScene::ClearAll()
	{
		UnbindErrorCheck();
		ScenePrimitiveOctree.ClearPrimitives();
	}
	void RScene::UnbindErrorCheck()
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static primitives' mapping failed"), (StaticPrimitiveMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static primitives failed"), (StaticPrimitives.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all dynamic primitives' mapping failed"), (DynamicPrimitiveMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all dynamic primitives failed"), (DynamicPrimitives.Length() == 0u));
	}
	void RScene::AddStaticPrimitive(PPrimitiveComponent* InComponent)
	{
		RScene* Scene = this;
		RPrimitiveSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveStaticPrimitive_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveStaticPrimitive(PPrimitiveComponent* InComponent)
	{
		RScene* Scene = this;
		RPrimitiveSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveStaticPrimitive_RenderThread(SceneProxy, FALSE);
			});
	}
	void RScene::AddDynamicPrimitive(PPrimitiveComponent* InComponent)
	{
		RScene* Scene = this;
		RPrimitiveSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveDynamicPrimitive_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveDynamicPrimitive(PPrimitiveComponent* InComponent)
	{
		RScene* Scene = this;
		RPrimitiveSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveDynamicPrimitive_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::UpdateDynamicPrimitive(PPrimitiveComponent* InComponent)
	{
		//TODO
	}
	void RScene::AddOrRemoveStaticPrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			AddPrimitiveInternal(InSceneProxy, StaticPrimitiveMapping, StaticPrimitives);
		}
		else
		{
			RemovePrimitiveInternal(InSceneProxy, StaticPrimitiveMapping, StaticPrimitives);
		}
	}
	void RScene::AddOrRemoveDynamicPrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			AddPrimitiveInternal(InSceneProxy, DynamicPrimitiveMapping, DynamicPrimitives);
		}
		else
		{
			RemovePrimitiveInternal(InSceneProxy, DynamicPrimitiveMapping, DynamicPrimitives);
		}
	}

};