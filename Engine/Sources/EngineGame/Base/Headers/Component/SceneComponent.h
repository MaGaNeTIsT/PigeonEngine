#pragma once
#include "../../../../EngineCore/Core/Headers/Main.h"
#include "./ActorComponent.h"
#include "./Transform.h"

namespace PigeonEngine
{

    class PSceneComponent : public PActorComponent
    {

		EClass(PSceneComponent, PActorComponent)

	public:

	public:
		virtual BOOL	ContainTransform()const override;
	protected:
		ETransform	Transform;


		CLASS_VIRTUAL_COPY_BODY(PSceneComponent)

    };

};

