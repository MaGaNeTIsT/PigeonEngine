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
	protected:
		PActorComponent*				ParentActorComponent;
		std::vector<PActorComponent*>	ChildrenActorComponent;

		CLASS_VIRTUAL_COPY_BODY(PActorComponent)

	public:
		PActor* GetOwnerActor()const;
	protected:
		void    SetOwnerActor(PActor* NewOwner);
	private:
		PActor* Owner = nullptr;
	};

};