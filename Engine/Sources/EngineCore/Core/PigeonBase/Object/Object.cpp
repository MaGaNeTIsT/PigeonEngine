

#include "Object.h"

#include "../../../../../EngineThirdParty/RapidJson/Headers/JsonObject.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PObject, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PObject::PObject() : EObjectBase(), bActive(FALSE)
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

	void PObject::Destroy()
	{
		if (IsInitialized())
		{
			Uninit();
		}
		delete this;
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
		bInitialized = bInInitialized;
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
		bInitialized = FALSE;
		// Destroy();
	}

	BOOL32 PObject::IsInitialized() const
	{
		return bInitialized;
	}

	BOOL32 PObject::IsAddedToScene() const
	{
		return bAddedToScene;
	}

	BOOL32 PObject::IsActive() const
	{
		return bActive;
	}
	void PObject::Active()
	{
		bActive = TRUE;
	}
	void PObject::Inactive()
	{
		bActive = FALSE;
	}

};
