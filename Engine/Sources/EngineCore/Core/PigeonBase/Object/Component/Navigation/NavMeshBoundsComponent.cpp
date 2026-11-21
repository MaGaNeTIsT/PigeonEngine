#include "NavMeshBoundsComponent.h"

#include <Navigation/NavigationGeometry.h>
#include <Reflection/Property.h>

#if ENGINE_EDITOR_ONLY
#include <Navigation/NavigationManager.h>
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavMeshBoundsComponent>();
		AddClassPropertyAccessors<PNavMeshBoundsComponent, EString>(ClassHash, PE_TEXT("NavMeshName"),
			[](const PNavMeshBoundsComponent* Component) -> EString { return Component->GetNavMeshName(); },
			[](PNavMeshBoundsComponent* Component, const EString& Value) -> void { Component->SetNavMeshName(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PNavMeshBoundsComponent, Vector3>(ClassHash, PE_TEXT("Extent"),
			[](const PNavMeshBoundsComponent* Component) -> Vector3 { return Component->GetExtent(); },
			[](PNavMeshBoundsComponent* Component, const Vector3& Value) -> void { Component->SetExtent(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavMeshBoundsComponent, PSceneComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavMeshBoundsComponent::PNavMeshBoundsComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshBoundsComponent");
		// The editor wireframe is drawn from a tick, and the change detection in it is what
		// schedules a rebuild after the volume is moved.
		SetIsTickable(TRUE);
	}

	PNavMeshBoundsComponent::~PNavMeshBoundsComponent()
	{
	}

	const EString& PNavMeshBoundsComponent::GetNavMeshName() const
	{
		return NavMeshName;
	}

	void PNavMeshBoundsComponent::SetNavMeshName(const EString& InName)
	{
		NavMeshName = InName;
	}

	const Vector3& PNavMeshBoundsComponent::GetExtent() const
	{
		return Extent;
	}

	void PNavMeshBoundsComponent::SetExtent(const Vector3& InExtent)
	{
		Extent = Vector3(EMath::Max(InExtent.x, 0.0f), EMath::Max(InExtent.y, 0.0f), EMath::Max(InExtent.z, 0.0f));
	}

	EBoundAABB PNavMeshBoundsComponent::MakeWorldBound() const
	{
		if ((Extent.x <= 0.0f) || (Extent.y <= 0.0f) || (Extent.z <= 0.0f))
		{
			return EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		}
		const EBoundAABB LocalBox(Vector3(-Extent.x, -Extent.y, -Extent.z), Vector3(Extent.x, Extent.y, Extent.z), TRUE);
		return ENavigationGeometryCollector::MakeComponentWorldBound(*this, LocalBox);
	}

#if ENGINE_EDITOR_ONLY
	void PNavMeshBoundsComponent::EditorTick(FLOAT InDeltaTime)
	{
		PSceneComponent::EditorTick(InDeltaTime);

		if (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled())
		{
			return;
		}

		// Red when the volume bakes nothing: it is the only signal an author needs, and the
		// bake skips invalid volumes instead of guessing what an axis at zero meant.
		const BOOL32 bValid = (MakeWorldBound().IsValid != FALSE) ? TRUE : FALSE;
		const Vector3 Scale = GetComponentWorldScale();
		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		WireframeManager->DrawCuboid(GetComponentWorldLocation(), GetComponentWorldRotation(),
			Extent.x * 2.0f * Scale.x, Extent.y * 2.0f * Scale.y, Extent.z * 2.0f * Scale.z,
			bValid ? Color4::Green() : Color4::Red());
	}
#endif

}  // namespace PigeonEngine
