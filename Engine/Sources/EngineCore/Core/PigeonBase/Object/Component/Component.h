#pragma once

#include <CoreMinimal.h>
#include <PigeonBase/Object/Object.h>


namespace PigeonEngine
{

	class PComponent : public PObject
	{

		EClass(PComponent, PObject)

	public:
		virtual void	Init()override;
		virtual void	Uninit()override;
		virtual void	Tick(FLOAT deltaTime)override;
		virtual void	FixedTick(FLOAT deltaTime);
#ifdef _EDITOR_ONLY
		virtual void	EditorTick(FLOAT deltaTime)override;
#endif

		CLASS_VIRTUAL_NOCOPY_BODY(PComponent)
	public:
		void Destroy() override;


	};

};