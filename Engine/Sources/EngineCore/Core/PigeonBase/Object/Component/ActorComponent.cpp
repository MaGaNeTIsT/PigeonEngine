#include "./ActorComponent.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>


namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PActorComponent, PComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PActorComponent::PActorComponent()
		: PComponent()
	{
		POBJ_DEBUGNAME_SET(this, "ActorComponent");
	}
	PActorComponent::~PActorComponent()
	{
	}

	PActor* PActorComponent::GetOwnerActor() const
	{
		return this->Owner;
	}


	void PActorComponent::RemoveFromOwnerActor()
	{
		Owner = nullptr;
	}

	void PActorComponent::OnAddedToScene(PWorld* World)
	{
		if (!IsAddedToScene())
		{
			BeginAddedToScene(World);
		}
	}

	void PActorComponent::OnRemovedFromScene()
	{
		if (IsAddedToScene())
		{
			RemovedFromScene();
		}
	}

	void PActorComponent::BeginAddedToScene(PWorld* World)
	{
		PObject::BeginAddedToScene(World);
		if (World)
		{
			World->RegisterComponent(this);
			RegisterTickDomains();
		}
	}

	void PActorComponent::RemovedFromScene()
	{
		// The world pointer is still set here (PObject::RemovedFromScene clears it at the
		// end of the chain), so the registry entry can be dropped while the component is
		// still addressable. Subclass overrides chain to this one.
		UnregisterTickDomains();
		UnregisterPostPhysicsTick();
		if (PWorld* World = GetWorld())
		{
			World->UnregisterComponent(this);
		}
		PObject::RemovedFromScene();
	}

	void PActorComponent::RegisterTickDomains()
	{
		if (!IsTickable())
		{
			return;
		}
		PWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		if (!VariableTickHandle.IsValid())
		{
			VariableTickHandle = Scheduler.RegisterComponent(*this, ETickDomain::VARIABLE, GetTickPriority());
		}
		if (!FixedTickHandle.IsValid())
		{
			FixedTickHandle = Scheduler.RegisterComponent(*this, ETickDomain::FIXED, GetTickPriority());
		}
#if ENGINE_EDITOR_ONLY
		if (!EditorTickHandle.IsValid())
		{
			EditorTickHandle = Scheduler.RegisterComponent(*this, ETickDomain::EDITOR, GetTickPriority());
		}
#endif
	}

	void PActorComponent::UnregisterTickDomains()
	{
		PWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		Scheduler.Unregister(VariableTickHandle);
		Scheduler.Unregister(FixedTickHandle);
#if ENGINE_EDITOR_ONLY
		Scheduler.Unregister(EditorTickHandle);
#endif
	}

	void PActorComponent::SetOwnerActor(PActor* NewOwner)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "You are setting this component's owner actor to a nullptr. ", NewOwner != nullptr);
		this->Owner = NewOwner;
		// NewOwner->AddComponent(this);
	}

	void PActorComponent::RegisterPostPhysicsTick()
	{
		if (bPostPhysicsTickRegistered || (!GetWorld()))
		{
			return;
		}
		PostPhysicsTickHandle = GetWorld()->GetTickScheduler().RegisterComponent(
			*this, ETickDomain::POST_PHYSICS, GetTickPriority());
		bPostPhysicsTickRegistered = PostPhysicsTickHandle.IsValid();
	}

	void PActorComponent::UnregisterPostPhysicsTick()
	{
		if (!bPostPhysicsTickRegistered)
		{
			return;
		}
		if (PWorld* World = GetWorld())
		{
			World->GetTickScheduler().Unregister(PostPhysicsTickHandle);
		}
		else
		{
			PostPhysicsTickHandle.Reset();
		}
		bPostPhysicsTickRegistered = FALSE;
	}

	void PActorComponent::HandlePostPhysicsTick(FLOAT deltaTime)
	{
		(void)deltaTime;
	}

	void PActorComponent::OnPreDelete()
	{
		// Clearing the owner here - after Uninit - is the fix for the old order, which
		// cleared it first and then let the virtual Uninit run with a null owner. The
		// owner drops the reference at the same time, so its list never keeps a pointer
		// to memory that is about to go away.
		if (Owner)
		{
			Owner->RemoveComponentReference(this);
			Owner = nullptr;
		}
	}

	void PActorComponent::Init()
	{
		PComponent::Init();
	}

	void PActorComponent::SetIsTickable(const BOOL32& bTickable)
	{
		PObject::SetIsTickable(bTickable);
		if (bTickable)
		{
			RegisterTickDomains();
		}
		else
		{
			UnregisterTickDomains();
		}
	}

	void PActorComponent::SetTickPriority(const INT32& InPriority)
	{
		PObject::SetTickPriority(InPriority);
		PWorld* World = GetWorld();
		if (!World)
		{
			return;
		}
		FWorldTickScheduler& Scheduler = World->GetTickScheduler();
		Scheduler.SetPriority(VariableTickHandle, InPriority);
		Scheduler.SetPriority(FixedTickHandle, InPriority);
		Scheduler.SetPriority(PostPhysicsTickHandle, InPriority);
#if ENGINE_EDITOR_ONLY
		Scheduler.SetPriority(EditorTickHandle, InPriority);
#endif
	}

	void PActorComponent::Uninit()
	{
		PComponent::Uninit();
	}

	BOOL32 PActorComponent::ContainTransform() const
	{
		return FALSE;
	}

	// Render proxy functions START
	BOOL32 PActorComponent::IsVisible() const
	{
		return TRUE;
	}
	BOOL32 PActorComponent::ShouldRender() const
	{
		return IsVisible();
	}
	BOOL32 PActorComponent::IsRenderStateDirty() const
	{
		return RenderStateDirty;
	}
	void PActorComponent::CreateRenderState()
	{
		//TODO
	}
	void PActorComponent::DestroyRenderState()
	{
		//TODO
	}
	void PActorComponent::SendUpdateRenderState()
	{
		//TODO
		CleanMarkRenderStateDirty();
	}
	void PActorComponent::MarkRenderStateAsDirty()
	{
		RenderStateDirty = TRUE;
		if (PActor* OwnerActor = GetOwnerActor())
		{
			OwnerActor->MarkRenderTreeDirty();
		}
	}

	void PActorComponent::CleanMarkRenderStateDirty()
	{
		RenderStateDirty = FALSE;
	}

	// Render proxy functions END

};
