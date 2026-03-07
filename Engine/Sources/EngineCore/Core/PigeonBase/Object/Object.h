#pragma once

#include <CoreMinimal.h>


namespace PigeonEngine
{
	class CJsonObject;
	class PWorld;
	class PObject : public EObjectBase
	{
	public:
		virtual void	Init() = 0;
		virtual void	Uninit();
		virtual void	Tick(FLOAT deltaTime) = 0;
#if _EDITOR_ONLY
		virtual void	EditorTick(FLOAT deltaTime) = 0;
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
		BOOL32 bCanTick = FALSE;
	public:
		virtual void Destroy();

	public:
		virtual TSharedPtr<CJsonObject> Serialize();
		

	public:
		virtual PWorld* GetWorld() const;
	protected:
		virtual void SetWorld(PWorld* NewWorld);
	private:
		PWorld* MyWorld = nullptr;

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World);
		virtual void RemovedFromScene();
	};

};