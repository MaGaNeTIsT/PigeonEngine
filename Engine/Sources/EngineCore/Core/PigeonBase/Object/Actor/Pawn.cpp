#include "Pawn.h"


namespace PigeonEngine
{
    PPawn::PPawn()
    {
        POBJ_DEBUGNAME_SET(this, "Pawn");
        this->SetIsTickable(TRUE);
    }

    PPawn::~PPawn()
    {

    }

    void PPawn::BeginAddedToScene(PWorld* World)
    {
        PActor::BeginAddedToScene(World);
        //SkeletalMeshComponent = new PSkeletalMeshComponent();
        //AddComponent(SkeletalMeshComponent, ETransform());
    }

    void PPawn::RemovedFromScene()
    {
        PActor::RemovedFromScene();
        //DestoyComponent(SkeletalMeshComponent);
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
