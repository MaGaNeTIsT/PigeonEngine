#pragma once

#include "../../../../EngineCore/Core/Headers/Main.h"
#include "./Component.h"

namespace PigeonEngine
{

	class PActor;
	class PActorComponent : public PComponent
	{

		EClass(PActorComponent, PComponent)

	public:

	public:
		virtual BOOL ContainTransform()const;

		CLASS_VIRTUAL_NOCOPY_BODY(PActorComponent)

	public:
		PActor* GetOwnerActor()const;
	protected:
		void    SetOwnerActor(PActor* NewOwner);
	private:
		PActor* Owner = nullptr;
	};

};