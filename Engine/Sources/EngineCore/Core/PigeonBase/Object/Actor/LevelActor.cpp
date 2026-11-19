#include "LevelActor.h"

namespace PigeonEngine
{

	PLevelActor::PLevelActor()
	{
		this->SetIsTickable(TRUE);
	}

	PLevelActor::~PLevelActor()
	{
		// this->Destroy();
	}

	void PLevelActor::BeginAddedToScene(PWorld* World)
	{
		PActor::BeginAddedToScene(World);
	}

	void PLevelActor::RemovedFromScene()
	{
		PActor::RemovedFromScene();
	}

	void PLevelActor::UserBeginPlay()
	{
		// Content is a level asset (Assets/Levels/Demo.PLevel); this actor is the world root.
	}

	void PLevelActor::UserEndPlay()
	{

	}

	void PLevelActor::UserTick(FLOAT deltaTime)
	{
		(void)deltaTime;
	}


}
