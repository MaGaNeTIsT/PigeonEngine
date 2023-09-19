

#include "Object.h"

#include "../../../../../EngineThirdParty/RapidJson/Headers/JsonObject.h"

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PObject, EObjectBase>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PObject::PObject() : m_Active(FALSE)
	{
	}
	PObject::~PObject()
	{
		Destroy();
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
		// delete this;
	}

	TSharedPtr<CJsonObject> PObject::Serialize()
	{
		
		return TSharedPtr<CJsonObject>::MakeShared();
	}

	PWorld* PObject::GetWorld() const
	{
		return this->MyWorld;
	}

	void PObject::SetWorld(PWorld* NewWorld)
	{
		this->MyWorld = NewWorld;
	}

	void PObject::BeginAddedToScene(PWorld* World)
	{
		this->SetWorld(World);
	}

	void PObject::RemovedFromScene()
	{
	}


#if _EDITOR_ONLY
	void PObject::SetName(const EString& name)
	{
		DebugName = name;
	}
#endif
	void PObject::Uninit()
	{
		Destroy();
	}

	BOOL32 PObject::IsActive()const
	{
		return m_Active;
	}
	void PObject::Active()
	{
		m_Active = TRUE;
	}
	void PObject::Inactive()
	{
		m_Active = FALSE;
	}

};