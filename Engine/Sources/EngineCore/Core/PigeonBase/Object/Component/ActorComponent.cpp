#include "./ActorComponent.h"
#include <PigeonBase/Object/Actor.h>

namespace PigeonEngine
{

	static void RegisterClassTypes()
	{
		RegisterClassType<PActorComponent, PComponent>();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PActorComponent::PActorComponent() 
	{
		POBJ_DEBUGNAME_SET(this, "ActorComponent");
	}
	PActorComponent::~PActorComponent()
	{
	}

	PActor* PActorComponent::GetOwnerActor() const
	{
		return this->Owner;
	}

	
	void PActorComponent::RemoveFromOwnerActor()
	{
		Owner = nullptr;
	}

	void PActorComponent::SetOwnerActor(PActor* NewOwner)
	{
		PE_CHECK(ENGINE_COMPONENT_ERROR, "You are setting this component's owner actor to a nullptr. ", NewOwner != nullptr);
		this->Owner = NewOwner;
		// NewOwner->AddComponent(this);
	}

	void PActorComponent::Destroy()
	{
		RemoveFromOwnerActor();
		PComponent::Destroy();
	}

	void PActorComponent::Init()
	{
		PComponent::Init();
	}

	void PActorComponent::Uninit()
	{
		PComponent::Uninit();
	}

	BOOL32 PActorComponent::ContainTransform()const
	{
		return FALSE;
	}

	// Render proxy functions START
	BOOL32 PActorComponent::IsVisible()const
	{
		return TRUE;
	}
	BOOL32 PActorComponent::CheckIsRenderHidden()const
	{
		return FALSE;
	}
	BOOL32 PActorComponent::ShouldRender()const
	{
		return IsVisible();
	}
	BOOL32 PActorComponent::IsRenderStateDirty()const
	{
		return RenderStateDirty;
	}
	void PActorComponent::CreateRenderState()
	{
		//TODO
	}
	void PActorComponent::DestroyRenderState()
	{
		//TODO
	}
	void PActorComponent::SendUpdateRenderState()
	{
		//TODO
		CleanMarkRenderStateDirty();
	}
	void PActorComponent::MarkRenderStateAsDirty()
	{
		RenderStateDirty = TRUE;
	}
	void PActorComponent::CleanMarkRenderStateDirty()
	{
		RenderStateDirty = FALSE;
	}



	void PActorComponent::GenerateComponentOutline(const PActorComponent* WorldCurrentSelectedComponent)
	{

	}

	void PActorComponent::GenerateComponentDetail()
	{

	}

	void PActorComponent::OnSelectedByImGui()
	{

	}

	// Render proxy functions END

};