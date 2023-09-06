#pragma once

#include <CoreMinimal.h>
#include "RenderInterface.h"
#include <RenderOctree.h>

namespace PigeonEngine
{

	class PPrimitiveComponent;

	class RCommand final : public ERegisterBase
	{
	public:
		template<typename _TFunctionType>
		void EnqueueCommand(_TFunctionType InFunction)
		{
			AddRegisterFunction<_TFunctionType>(InFunction);
		}
		void DoCommands()
		{
			DoRegister();
		}
		void EmptyQueue()
		{
			ClearRegister();
		}
	public:
		RCommand() {}
		RCommand(const RCommand&) = delete;
		virtual ~RCommand() {}
		RCommand& operator=(const RCommand&) = delete;
	};

	class RScene : public EObjectBase, public RSceneInterface
	{
	public:
		virtual void	ClearAll()override;
		virtual void	UnbindErrorCheck()override;
	public:
		virtual void	AddStaticPrimitive(PPrimitiveComponent* InComponent)override;
		virtual void	RemoveStaticPrimitive(PPrimitiveComponent* InComponent)override;
		virtual void	AddDynamicPrimitive(PPrimitiveComponent* InComponent)override;
		virtual void	RemoveDynamicPrimitive(PPrimitiveComponent* InComponent)override;
		virtual void	UpdateDynamicPrimitive(PPrimitiveComponent* InComponent)override;
	protected:
		void	AddOrRemoveStaticPrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy, BOOL32 InIsAdd);
		void	AddOrRemoveDynamicPrimitive_RenderThread(RPrimitiveSceneProxy* InSceneProxy, BOOL32 InIsAdd);
	protected:
		ROctree		ScenePrimitiveOctree;
	protected:
		TMap<ObjectIdentityType, UINT32>	StaticPrimitiveMapping;
		TArray<RPrimitiveSceneProxy*>		StaticPrimitives;
		TMap<ObjectIdentityType, UINT32>	DynamicPrimitiveMapping;
		TArray<RPrimitiveSceneProxy*>		DynamicPrimitives;
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
		virtual void	AddStaticPrimitive(PPrimitiveComponent* InComponent)override {}
		virtual void	RemoveStaticPrimitive(PPrimitiveComponent* InComponent)override {}
		virtual void	AddDynamicPrimitive(PPrimitiveComponent* InComponent)override {}
		virtual void	RemoveDynamicPrimitive(PPrimitiveComponent* InComponent)override {}
		virtual void	UpdateDynamicPrimitive(PPrimitiveComponent* InComponent)override {}
	public:
		RSceneNull() {}
		RSceneNull(const RSceneNull& Other) {}
		virtual ~RSceneNull() {}
		RSceneNull& operator=(const RSceneNull& Other) { return (*this); }
	};

};