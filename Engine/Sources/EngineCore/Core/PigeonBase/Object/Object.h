#pragma once

#include <CoreMinimal.h>


#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PigeonEngine
{
	class CJsonObject;
	class PObject : public EObjectBase
	{
	public:
		virtual void	Init() = 0;
		virtual void	Uninit();
		virtual void	Tick(FLOAT deltaTime) = 0;
#if _EDITOR_ONLY
		virtual void	EditorTick(FLOAT deltaTime) = 0;
	public:
		virtual void	SetName(const EString& name);
#endif
	public:
		virtual BOOL32	IsActive()const;
		virtual void	Active();
		virtual void	Inactive();
	protected:
		BOOL32			m_Active;

		CLASS_VIRTUAL_NOCOPY_BODY(PObject)
	
	public:
		void SetIsTickable(const BOOL32& bTickable);
		BOOL32 IsTickable() const;
	private:
		BOOL32 bCanTick = false;
	public:
		virtual void Destroy();

	// public:
	// 	virtual TSharedPtr<CJsonObject> Serialize();
		

	};

};