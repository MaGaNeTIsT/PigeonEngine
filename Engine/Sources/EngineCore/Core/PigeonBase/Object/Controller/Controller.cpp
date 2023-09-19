#include "Controller.h"

#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"

namespace PigeonEngine
{
    PController::PController()
    {
       this->SetRootComponent(new PCameraComponent());
    }
    PController::~PController()
    {
    }
}
