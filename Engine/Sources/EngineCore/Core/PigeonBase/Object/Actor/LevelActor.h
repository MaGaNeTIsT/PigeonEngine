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

    };

}

