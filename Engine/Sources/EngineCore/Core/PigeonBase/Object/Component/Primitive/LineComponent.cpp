#include "LineComponent.h"
#include <Reflection/Property.h>
namespace PigeonEngine
{
	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PLineComponent>();
		AddClassPropertyAccessors<PLineComponent, TArray<Vector3>>(ClassHash, PE_TEXT("Points"),
			[](const PLineComponent* Component) -> TArray<Vector3> { return Component->GetPoints(); },
			[](PLineComponent* Component, const TArray<Vector3>& Value) -> void { Component->SetPoints(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PLineComponent, PPrimitiveComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PLineComponent::PLineComponent()
	{
		POBJ_DEBUGNAME_SET(this, "LineComponent");
	}
	PLineComponent::~PLineComponent()
	{

	}

	void PLineComponent::SetPoints(const TArray<Vector3>& InPoints)
	{
		Points = InPoints;
		MarkRenderStateAsDirty();
	}

	void PLineComponent::CreateRenderState()
	{
		PPrimitiveComponent::CreateRenderState();
	}

	void PLineComponent::DestroyRenderState()
	{
		PPrimitiveComponent::DestroyRenderState();
	}

	void PLineComponent::SendUpdateRenderState()
	{
		PPrimitiveComponent::SendUpdateRenderState();
	}

	void PLineComponent::MarkRenderTransformAsDirty()
	{
		PPrimitiveComponent::MarkRenderTransformAsDirty();
	}

	void PLineComponent::MarkRenderStateAsDirty()
	{
		PPrimitiveComponent::MarkRenderStateAsDirty();
	}

	void PLineComponent::CleanMarkRenderStateDirty()
	{
		PPrimitiveComponent::CleanMarkRenderStateDirty();
	}
}
