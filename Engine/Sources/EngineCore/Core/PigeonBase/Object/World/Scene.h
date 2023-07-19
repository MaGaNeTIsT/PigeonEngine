#pragma once

#include <CoreMinimal.h>

#include "Base/DataStructure/Container/Array.h"
#include "PigeonBase/Object/Object.h"


namespace PigeonEngine
{
	class PWorld;
	class PActor;
	
	/*
	 * 
	 */
	class PScene : public PObject
	{
		EClass(PScene, PObject)
		
	public:
		
		CLASS_VIRTUAL_NOCOPY_BODY(PScene)
		
	protected:
		
		friend class PWorld;

		void AddActorToScene(PActor* NewActor, const ETransform& RelativeTransform = ETransform());
		void LoadFromLevel(const EString& LevelFilePath);	
	private:
		// za wa lu do
		PWorld* World = nullptr;

		// All actors in this scene, include children.
		// For Actor Iteration
		TArray<PActor*> Actors;
		// actors attached direct to root
		TArray<PActor*> RootActors;

		PActor* SceneRoot;
	};

};