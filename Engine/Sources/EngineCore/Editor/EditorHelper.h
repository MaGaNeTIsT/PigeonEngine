#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	extern class PActor* SelectObjectInViewport(const class PCameraComponent* InCamera, TArray<class PActor*>& InActors, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect);

	extern class PSceneComponent* SelectObjectInViewport(const class PCameraComponent* InCamera, TArray<class PSceneComponent*>& InActors, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect);

#if _EDITOR_ONLY
	extern void DrawObjectBounds(TArray<class PActor*>& InObjects);
#endif

};