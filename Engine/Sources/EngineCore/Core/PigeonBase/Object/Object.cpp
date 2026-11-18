

#include "Object.h"

#include "../../../../../EngineThirdParty/RapidJson/Headers/JsonObject.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PObject, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	TDelegateBroadcast<void (PObject*)> PObject::OnObjectPreDelete;

	PObject::PObject() : EObjectBase()
	{
	}
	PObject::~PObject()
	{

	}

	void PObject::SetIsTickable(const BOOL32& bTickable)
	{
		this->bCanTick = bTickable;
	}

	BOOL32 PObject::IsTickable() const
	{
		return this->bCanTick;
	}

	void PObject::MarkPendingDestroy()
	{
		bPendingDestroy = TRUE;
	}

	BOOL32 PObject::IsPendingDestroy() const
	{
		return bPendingDestroy;
	}

	void PObject::Destroy()
	{
		// The flags are captured before the state flips so that no hook below can change
		// whether THIS orchestration runs a step.
		if ((LifecycleState == EObjectLifecycleStateType::DESTROYING) || (LifecycleState == EObjectLifecycleStateType::DESTROYED))
		{
			return;
		}
		const BOOL32 bWasAddedToScene = IsAddedToScene();
		const BOOL32 bWasInitialized = IsInitialized();
		LifecycleState = EObjectLifecycleStateType::DESTROYING;
		if (bWasAddedToScene)
		{
			RemovedFromScene();
		}
		OnDestroy();
		if (bWasInitialized)
		{
			Uninit();
		}
		OnPreDelete();
		// Global observers (the message bus purging this object's subscriptions) drop
		// their references while the object is still complete and its ID still readable.
		OnObjectPreDelete.Broadcast(this);
		LifecycleState = EObjectLifecycleStateType::DESTROYED;
		delete this;
	}

	void PObject::OnDestroy()
	{
	}

	void PObject::OnPreDelete()
	{
	}

	TSharedPtr<CJsonObject> PObject::Serialize()
	{
		TSharedPtr<CJsonObject> Json = EMemory::MakeShared<CJsonObject>();
		Json->SetUInt64Field("TypeHash", this->GetTypeHash());
#if ENGINE_EDITOR_ONLY
		Json->SetStringField("Name", this->GetDebugName());
#endif
		return Json;
	}

	PWorld* PObject::GetWorld() const
	{
		return this->MyWorld;
	}

	void PObject::SetWorld(PWorld* NewWorld)
	{
		this->MyWorld = NewWorld;
	}

	void PObject::SetInitialized(const BOOL32& bInInitialized)
	{
		if (bInInitialized)
		{
			if (LifecycleState == EObjectLifecycleStateType::CREATED)
			{
				LifecycleState = EObjectLifecycleStateType::INITIALIZED;
			}
		}
		else
		{
			if (LifecycleState == EObjectLifecycleStateType::INITIALIZED)
			{
				LifecycleState = EObjectLifecycleStateType::CREATED;
			}
		}
	}

	void PObject::SetAddedToScene(const BOOL32& bInAddedToScene)
	{
		bAddedToScene = bInAddedToScene;
	}

	void PObject::BeginAddedToScene(PWorld* World)
	{
		this->SetWorld(World);
		SetAddedToScene(TRUE);
	}

	void PObject::RemovedFromScene()
	{
		SetAddedToScene(FALSE);
		SetWorld(nullptr);
	}

	void PObject::Uninit()
	{
		// Only an INITIALIZED object flips back; a DESTROYING one keeps its state so the
		// orchestrator's final DESTROYED assignment stays ordered.
		if (LifecycleState == EObjectLifecycleStateType::INITIALIZED)
		{
			LifecycleState = EObjectLifecycleStateType::CREATED;
		}
	}

	BOOL32 PObject::IsInitialized() const
	{
		// DESTROYING counts as initialized: Uninit has not run yet, so the object is
		// still obligated to uninitialize during destroy.
		return ((LifecycleState == EObjectLifecycleStateType::INITIALIZED) || (LifecycleState == EObjectLifecycleStateType::DESTROYING)) ? TRUE : FALSE;
	}

	BOOL32 PObject::IsAddedToScene() const
	{
		return bAddedToScene;
	}

};
