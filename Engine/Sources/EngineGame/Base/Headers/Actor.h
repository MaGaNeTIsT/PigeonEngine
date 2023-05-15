#pragma once

#include "../../../EngineCore/Core/Headers/Main.h"
#include "./Object.h"


namespace PigeonEngine
{

	class PActor : public PObject
	{

		EClass(PActor, PObject)

	public:
		virtual void    Init() override;
		virtual void	FixedTick(FLOAT deltaTime);
	private:
		BOOL bCanTick = false;
		CLASS_VIRTUAL_NOCOPY_BODY(PActor)
		
	public:
		
		PActor* GetAttachedParentActor() const;
		void AttachToActor(PActor* AttachTo);
		
	private:

		PActor* AttachedParentActor = nullptr;
		
	public:
		
		class PSceneComponent* GetRootComponent() const;
		
	private:
		
		PSceneComponent* RootComponent = nullptr;

		
		
	};

};