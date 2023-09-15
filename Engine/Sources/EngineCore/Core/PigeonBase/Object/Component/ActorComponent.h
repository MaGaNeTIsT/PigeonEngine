#pragma once

#include <CoreMinimal.h>

#include "Component.h"

namespace PigeonEngine
{

	class PActor;
	class PActorComponent : public PComponent
	{
	public:
		virtual void Init() override;
		virtual void Uninit() override;
	public:
		virtual BOOL32 ContainTransform()const;

		CLASS_VIRTUAL_NOCOPY_BODY(PActorComponent)

	public:
		PActor* GetOwnerActor()const;
		void    RemoveFromOwnerActor();
	protected:
		void    SetOwnerActor(PActor* NewOwner);
	private:
		PActor* Owner = nullptr;

	public:
		virtual void Destroy() override;

		// Render proxy functions START
	public:
		BOOL32			ShouldRender()const;
		BOOL32			IsRenderStateDirty()const;
		virtual void	CreateRenderState();
		virtual void	DestroyRenderState();
		virtual void	SendUpdateRenderState();
	protected:
		virtual void	MarkRenderStateAsDirty();
		virtual void	CleanMarkRenderStateDirty();
	protected:
		BOOL32	RenderStateDirty = FALSE;
		// Render proxy functions END
	};

};