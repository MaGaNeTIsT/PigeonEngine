#pragma once
#include <CoreMinimal.h>
#include "../Actor.h"
#include "../Component/Primitive/SkeletalMeshComponent.h"

namespace PigeonEngine
{
	class PPawn : public PActor
	{
		friend class PWorld;
		CLASS_VIRTUAL_NOCOPY_BODY(PPawn)

	protected:
		// for frame, 
		virtual void BeginAddedToScene(PWorld* World) override;
		virtual void RemovedFromScene() override;
	protected:

		virtual void UserBeginPlay() override;
		virtual void UserTick(FLOAT deltaTime) override;
		virtual void UserEndPlay() override;

	public:
		PSkeletalMeshComponent* SkeletalMeshComponent = NULL;
	};
}