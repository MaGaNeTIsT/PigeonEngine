#pragma once
#include "PigeonBase/Object/Actor.h"

namespace PigeonEngine
{
	class PLevelActor : public PActor
	{
		friend class PWorld;
		CLASS_VIRTUAL_NOCOPY_BODY(PLevelActor)

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World);
		virtual void RemovedFromScene();
	protected:

		virtual void UserBeginPlay();
		virtual void UserTick(FLOAT deltaTime);
		virtual void UserEndPlay();

	private:
		// Demo only: the obstacle actor the navigation sample moves across the agent's path, so the
		// tile cache has something to rebuild around. Null when the level has no such actor.
		class PActor*	DemoObstacleActor = nullptr;
		FLOAT			DemoObstacleTimer = 0.0f;

	};

}

