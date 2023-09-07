#pragma once

#include <CoreMinimal.h>
#include "RenderInterface.h"
#include <RenderOctree.h>

namespace PigeonEngine
{

	class PStaticMeshComponent;
	class PSkeletalMeshComponent;
	class RStaticMeshSceneProxy;
	class RSkeletalMeshSceneProxy;

	class RCommand final
	{
	public:
		template<typename _TFunctionType>
		void EnqueueCommand(_TFunctionType InFunction)
		{
			RawCommands.AddRegisterFunction<_TFunctionType>(InFunction);
		}
		void DoCommands()
		{
			RawCommands.DoRegister();
		}
		void EmptyQueue()
		{
			RawCommands.ClearRegister();
		}
	private:
		ERegisterBase	RawCommands;
	public:
		RCommand() {}
		RCommand(const RCommand&) = delete;
		~RCommand() {}
		RCommand& operator=(const RCommand&) = delete;
	};

	template<typename _TProxyType>
	struct RSceneProxyMapping
	{
		RSceneProxyMapping() = default;
		RSceneProxyMapping(const RSceneProxyMapping&) = delete;
		RSceneProxyMapping& operator=(const RSceneProxyMapping&) = delete;

		/// <summary>
		/// Add scene proxy into mappings
		/// </summary>
		/// <param name="InSceneProxy">The scene proxy that needs to adding into mappings</param>
		/// <returns>If mappings already contains this scene proxy</returns>
		BOOL32 AddSceneProxy(_TProxyType* InSceneProxy)
		{
			if (!InSceneProxy)
			{
				return FALSE;
			}
			Check((ENGINE_RENDER_CORE_ERROR), ("Check scene proxy mapping and array length failed."), (SceneProxyMapping.Length() == SceneProxies.Length()));
			const ObjectIdentityType& PrimitiveID = InSceneProxy->GetUniqueID();
			if (SceneProxyMapping.ContainsKey(PrimitiveID))
			{
				return FALSE;
			}
			SceneProxyMapping.Add(PrimitiveID, SceneProxies.Length());
			SceneProxies.Add(InSceneProxy);
			return TRUE;
		}

		/// <summary>
		/// Remove scene proxy from mappings(This operation will NOT delete the memory that scene proxy pointer pointed to)
		/// </summary>
		/// <param name="InSceneProxy">The scene proxy that needs to removing from mappings</param>
		/// <returns>If mappings removes scene proxy success</returns>
		BOOL32 RemoveSceneProxy(_TProxyType* InSceneProxy)
		{
			if ((!InSceneProxy) || (SceneProxies.Length() == 0u))
			{
				return FALSE;
			}
			Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive mapping and array length failed."), (SceneProxyMapping.Length() == SceneProxies.Length()));
			const ObjectIdentityType& PrimitiveID = InSceneProxy->GetUniqueID();
			UINT32 PrimitiveIndex = (UINT32)(-1);
			if (!(SceneProxyMapping.FindValue(PrimitiveID, PrimitiveIndex)))
			{
				return FALSE;
			}
			Check((ENGINE_RENDER_CORE_ERROR), ("Check primitive index failed."), (PrimitiveIndex < SceneProxies.Length()));
			if (const UINT32 LastIndex = SceneProxies.Length() - 1u; PrimitiveIndex != LastIndex)
			{
				_TProxyType* TempPrimitiveProxy = SceneProxies[LastIndex];
				SceneProxies[LastIndex] = SceneProxies[PrimitiveIndex];
				SceneProxies[PrimitiveIndex] = TempPrimitiveProxy;
				SceneProxyMapping[TempPrimitiveProxy->GetUniqueID()] = PrimitiveIndex;
			}
			SceneProxyMapping.Remove(PrimitiveID);
			SceneProxies.Pop();
			return TRUE;
		}

		TMap<ObjectIdentityType, UINT32>	SceneProxyMapping;
		TArray<_TProxyType*>				SceneProxies;
	};

	class RScene : public EObjectBase, public RSceneInterface
	{
	public:
		virtual void	ClearAll()override;
		virtual void	UnbindErrorCheck()override;
	public:
		virtual void	AddStaticMesh(PStaticMeshComponent* InComponent)override;
		virtual void	RemoveStaticMesh(PStaticMeshComponent* InComponent)override;
		virtual void	UpdateStaticMesh(PStaticMeshComponent* InComponent)override;
		virtual void	AddSkeletalMesh(PSkeletalMeshComponent* InComponent)override;
		virtual void	RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)override;
		virtual void	UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)override;
	protected:
		void	AddOrRemoveStaticMesh_RenderThread(RStaticMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd);
		void	UpdateStaticMesh_RenderThread(RStaticMeshSceneProxy* InSceneProxy);
		void	AddOrRemoveSkeletalMesh_RenderThread(RSkeletalMeshSceneProxy* InSceneProxy, BOOL32 InIsAdd);
		void	UpdateSkeletalMesh_RenderThread(RSkeletalMeshSceneProxy* InSceneProxy);
	protected:
		ROctree		ScenePrimitiveOctree;
	protected:
		RSceneProxyMapping<RStaticMeshSceneProxy>	StaticMeshSceneProies;
		RSceneProxyMapping<RSkeletalMeshSceneProxy>	SkeletalMeshSceneProies;
	protected:
		RCommand	RenderAddRemoveCommands;
		RCommand	RenderUpdateCommands;
	public:
		RScene();
		virtual ~RScene();
		RScene(const RScene&) = delete;
		RScene& operator=(const RScene&) = delete;
	};

	class RSceneNull : public EObjectBase, public RSceneInterface
	{
	public:
		virtual void	ClearAll()override {}
		virtual void	UnbindErrorCheck()override {}
	public:
		virtual void	AddStaticMesh(PStaticMeshComponent* InComponent)override {}
		virtual void	RemoveStaticMesh(PStaticMeshComponent* InComponent)override {}
		virtual void	UpdateStaticMesh(PStaticMeshComponent* InComponent)override {}
		virtual void	AddSkeletalMesh(PSkeletalMeshComponent* InComponent)override {}
		virtual void	RemoveSkeletalMesh(PSkeletalMeshComponent* InComponent)override {}
		virtual void	UpdateSkeletalMesh(PSkeletalMeshComponent* InComponent)override {}
	public:
		RSceneNull() {}
		RSceneNull(const RSceneNull& Other) {}
		virtual ~RSceneNull() {}
		RSceneNull& operator=(const RSceneNull& Other) { return (*this); }
	};

};