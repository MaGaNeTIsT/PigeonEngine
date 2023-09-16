#include "LevelActor.h"
#include "../Component/SceneComponent.h"
namespace PigeonEngine
{
    PLevelActor::PLevelActor()
    {
        if(!GetRootComponent())
        {
            SetRootComponent(new PSceneComponent());
        }
    }
	
    PLevelActor::~PLevelActor()
    {
        this->Destroy();
    }
	
}