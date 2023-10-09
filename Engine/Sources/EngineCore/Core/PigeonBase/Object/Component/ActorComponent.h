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
		virtual BOOL32	IsVisible()const;
		virtual BOOL32	CheckIsRenderHidden()const;
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

#if _EDITOR_ONLY
	public:
		virtual void GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent);
		virtual void GenerateComponentDetail();
		virtual void OnSelectedByImGui();
#endif

	};
}