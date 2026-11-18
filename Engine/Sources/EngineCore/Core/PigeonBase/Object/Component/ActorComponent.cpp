#include "./ActorComponent.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/World/WorldTickManager.h>

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
		}
	}

	void PActorComponent::RemovedFromScene()
	{
		// The world pointer is still set here (PObject::RemovedFromScene clears it at the
		// end of the chain), so the registry entry can be dropped while the component is
		// still addressable. Subclass overrides chain to this one.
		if (PWorld* World = GetWorld())
		{
			World->UnregisterComponent(this);
		}
		PObject::RemovedFromScene();
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
		PWorld* World = GetWorld();
		const ObjectIdentityType ComponentID = GetUniqueID();
		// The handler only holds the world and the ID: if this component is destroyed
		// (or pending destruction), the lookup fails and the tick is a no-op, so no raw
		// pointer outlives the component.
		PostPhysicsTickHandler = TFunction<void(FLOAT)>([World, ComponentID](FLOAT deltaTime) -> void
		{
			if (!World)
			{
				return;
			}
			const PActorComponent* FoundComponent = World->GetComponentByUniqueID(ComponentID, TRUE);
			PActorComponent* MutableComponent = const_cast<PActorComponent*>(FoundComponent);
			if (MutableComponent && (!MutableComponent->IsPendingDestroy()))
			{
				MutableComponent->HandlePostPhysicsTick(deltaTime);
			}
		});
		EWorldTickManager::GetManagerSingleton()->RegisterPostPhysicsTick(PostPhysicsTickHandler);
		bPostPhysicsTickRegistered = TRUE;
	}

	void PActorComponent::UnregisterPostPhysicsTick()
	{
		if (bPostPhysicsTickRegistered)
		{
			EWorldTickManager::GetManagerSingleton()->UnregisterPostPhysicsTick(PostPhysicsTickHandler);
			bPostPhysicsTickRegistered = FALSE;
		}
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
	}
	void PActorComponent::CleanMarkRenderStateDirty()
	{
		RenderStateDirty = FALSE;
	}


#if ENGINE_EDITOR_ONLY
	void PActorComponent::GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent)
	{

	}

	void PActorComponent::GenerateComponentDetail()
	{

	}

	void PActorComponent::OnSelectedByImGui()
	{

	}
#endif
	// Render proxy functions END

};
