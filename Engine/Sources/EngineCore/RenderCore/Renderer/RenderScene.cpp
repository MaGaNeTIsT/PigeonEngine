#include "RenderScene.h"
#include <RenderProxy/PrimitiveSceneProxy.h>
#include <RenderProxy/MeshSceneProxy.h>
#include <RenderProxy/StaticMeshSceneProxy.h>
#include <RenderProxy/SkeletalMeshSceneProxy.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/Component/Primitive/SkeletalMeshComponent.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<RCommand, ERegisterBase>();
		RegisterClassType<RScene, EObjectBase, RSceneInterface>();
		RegisterClassType<RSceneNull, EObjectBase, RSceneInterface>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	RScene::RScene()
	{
	}
	RScene::~RScene()
	{
		UnbindErrorCheck();
	}
	void RScene::Init()
	{

	}
	void RScene::Uninit()
	{

	}
	void RScene::ClearAll()
	{
		UnbindErrorCheck();
		ScenePrimitiveOctree.ClearPrimitives();
	}
	void RScene::UnbindErrorCheck()
	{
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes' mapping failed"), (StaticMeshSceneProies.SceneProxyMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all static meshes failed"), (StaticMeshSceneProies.SceneProxies.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes' mapping failed"), (SkeletalMeshSceneProies.SceneProxyMapping.Length() == 0u));
		Check((ENGINE_RENDER_CORE_ERROR), ("Check render scene clear all skeletal meshes failed"), (SkeletalMeshSceneProies.SceneProxies.Length() == 0u));
	}
	void RScene::AddStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveStaticMesh_RenderThread(SceneProxy, FALSE);
			});
	}
	void RScene::UpdateStaticMesh(PStaticMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RStaticMeshSceneProxy* SceneProxy = InComponent->GetSceneProxy();
		//TODO
		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->UpdateStaticMesh_RenderThread(SceneProxy);
			});
	}
	void RScene::AddSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->CreateSceneProxy();
		RenderAddRemoveCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->AddOrRemoveSkeletalMesh_RenderThread(SceneProxy, TRUE);
			});
	}
	void RScene::UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)
	{
		RScene* Scene = this;
		RSkeletalMeshSceneProxy* SceneProxy = InComponent->GetSceneProxy();
		//TODO
		RenderUpdateCommands.EnqueueCommand(
			[Scene, SceneProxy]()->void
			{
				Scene->UpdateSkeletalMesh_RenderThread(SceneProxy);
			});
	}
	void RScene::AddOrRemoveStaticMesh_RenderThread(RStaticMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			StaticMeshSceneProies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			StaticMeshSceneProies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::UpdateStaticMesh_RenderThread(RStaticMeshSceneProxy* InSceneProxy)
	{
		//TODO
	}
	void RScene::AddOrRemoveSkeletalMesh_RenderThread(RSkeletalMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd)
	{
		if (InIsAdd)
		{
			SkeletalMeshSceneProies.AddSceneProxy(InSceneProxy);
		}
		else
		{
			SkeletalMeshSceneProies.RemoveSceneProxy(InSceneProxy);
		}
	}
	void RScene::UpdateSkeletalMesh_RenderThread(RSkeletalMeshSceneProxy* InSceneProxy)
	{
		//TODO
	}

};