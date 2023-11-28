#include "EditorHelper.h"
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>

namespace PigeonEngine
{

	PActor* SelectObjectInViewport(const PCameraComponent* InCamera, TArray<PActor*>& InActors, const INT32 InMouseX, const INT32 InMouseY, const ERect& InScreenRect)
	{
		const UINT32 ActorNum = InActors.Length();
		const FLOAT UsedMouseX = (FLOAT)InMouseX;
		const FLOAT UsedMouseY = (FLOAT)InMouseY;
		if ((!InCamera) || (ActorNum == 0u) || (InScreenRect.Right <= InScreenRect.Left) || (InScreenRect.Bottom <= InScreenRect.Top))
		{
			return nullptr;
		}
		if ((UsedMouseX < InScreenRect.Left) || (UsedMouseX > InScreenRect.Right) || (UsedMouseY < InScreenRect.Top) || (UsedMouseY > InScreenRect.Bottom))
		{
			return nullptr;
		}

		const EFrustum& UsedFrustum = InCamera->GetCameraFrustum();
		Vector2 MouseScreenCoord(
			(UsedMouseX - InScreenRect.Left) / (InScreenRect.Right - InScreenRect.Left),
			(UsedMouseY - InScreenRect.Top) / (InScreenRect.Bottom - InScreenRect.Top)
		);

		return nullptr;
	}

};