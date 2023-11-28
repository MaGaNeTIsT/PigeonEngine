#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	extern class PActor* SelectObjectInViewport(const class PCameraComponent* InCamera, TArray<class PActor*>& InActors, const INT32 InMouseX, const INT32 InMouseY, const ERect& InScreenRect);

};