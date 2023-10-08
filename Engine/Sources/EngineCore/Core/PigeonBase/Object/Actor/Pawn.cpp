#include "Pawn.h"


namespace PigeonEngine
{
    PPawn::PPawn()
    {
        this->SetIsTickable(TRUE);
    }

    PPawn::~PPawn()
    {

    }

    void PPawn::BeginAddedToScene(PWorld* World)
    {
        PActor::BeginAddedToScene(World);
    }

    void PPawn::RemovedFromScene()
    {
        PActor::RemovedFromScene();

    }

    void PPawn::UserBeginPlay()
    {
        PActor::UserBeginPlay();

    }

    void PPawn::UserEndPlay()
    {
        PActor::UserEndPlay();

    }

    void PPawn::UserTick(FLOAT deltaTime)
    {
        PActor::UserTick(deltaTime);

    }


}
