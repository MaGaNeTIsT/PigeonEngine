#include "NavMeshBoundsComponent.h"

#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationManager.h>
#include <Reflection/Property.h>

#if ENGINE_EDITOR_ONLY
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine
{

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavMeshBoundsComponent>();
		// Empty list = every profile; the usual level never fills this in, so one volume set
		// serves the default agent and any larger agent profiles.
		AddClassPropertyAccessors<PNavMeshBoundsComponent, TArray<EString>>(ClassHash, PE_TEXT("SupportedNavMeshes"),
			[](const PNavMeshBoundsComponent* Component) -> TArray<EString> { return Component->GetSupportedNavMeshes(); },
			[](PNavMeshBoundsComponent* Component, const TArray<EString>& Value) -> void { Component->SetSupportedNavMeshes(Value); },
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
#if ENGINE_EDITOR_ONLY
		// The wireframe and the edit-change detection need a tick; a shipped game has neither.
		SetIsTickable(TRUE);
#endif
	}

	PNavMeshBoundsComponent::~PNavMeshBoundsComponent()
	{
	}

	const TArray<EString>& PNavMeshBoundsComponent::GetSupportedNavMeshes() const
	{
		return SupportedNavMeshes;
	}

	void PNavMeshBoundsComponent::SetSupportedNavMeshes(const TArray<EString>& InProfiles)
	{
		SupportedNavMeshes = InProfiles;
		// The volume may now feed a different set of profiles; every affected one has to bake.
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	BOOL32 PNavMeshBoundsComponent::SupportsNavMesh(const EString& InNavMeshName) const
	{
		if (SupportedNavMeshes.Num<UINT32>() == 0u)
		{
			// The common case: one volume set for every agent profile.
			return TRUE;
		}
		for (UINT32 Index = 0u, Num = SupportedNavMeshes.Num<UINT32>(); Index < Num; Index++)
		{
			if (SupportedNavMeshes[Index] == InNavMeshName)
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	const Vector3& PNavMeshBoundsComponent::GetExtent() const
	{
		return Extent;
	}

	void PNavMeshBoundsComponent::SetExtent(const Vector3& InExtent)
	{
		const Vector3 NewExtent(EMath::Max(InExtent.x, 0.0f), EMath::Max(InExtent.y, 0.0f), EMath::Max(InExtent.z, 0.0f));
		if ((NewExtent.x == Extent.x) && (NewExtent.y == Extent.y) && (NewExtent.z == Extent.z))
		{
			return;
		}
		Extent = NewExtent;
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	void PNavMeshBoundsComponent::BeginAddedToScene(PWorld* InWorld)
	{
		PSceneComponent::BeginAddedToScene(InWorld);
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	void PNavMeshBoundsComponent::RemovedFromScene()
	{
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
		PSceneComponent::RemovedFromScene();
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

		// Moving or rotating the volume has no other notification hook, so the editor tick is
		// where the owner's debounced rebuild is triggered from.
		const Vector3 Location = GetComponentWorldLocation();
		const Quaternion Rotation = GetComponentWorldRotation();
		const Vector3 Scale = GetComponentWorldScale();
		if ((!bHasLastTickTransform)
			|| (Location.x != LastTickLocation.x) || (Location.y != LastTickLocation.y) || (Location.z != LastTickLocation.z)
			|| (Rotation.x != LastTickRotation.x) || (Rotation.y != LastTickRotation.y)
			|| (Rotation.z != LastTickRotation.z) || (Rotation.w != LastTickRotation.w)
			|| (Scale.x != LastTickScale.x) || (Scale.y != LastTickScale.y) || (Scale.z != LastTickScale.z))
		{
			bHasLastTickTransform = TRUE;
			LastTickLocation = Location;
			LastTickRotation = Rotation;
			LastTickScale = Scale;
			ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
		}

		if (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled())
		{
			return;
		}

		// Red when the volume bakes nothing: it is the only signal an author needs, and the
		// bake skips invalid volumes instead of guessing what an axis at zero meant.
		const BOOL32 bValid = (MakeWorldBound().IsValid != FALSE) ? TRUE : FALSE;
		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		WireframeManager->DrawCuboid(Location, Rotation,
			Extent.x * 2.0f * Scale.x, Extent.y * 2.0f * Scale.y, Extent.z * 2.0f * Scale.z,
			bValid ? Color4::Green() : Color4::Red());
	}
#endif

}  // namespace PigeonEngine
