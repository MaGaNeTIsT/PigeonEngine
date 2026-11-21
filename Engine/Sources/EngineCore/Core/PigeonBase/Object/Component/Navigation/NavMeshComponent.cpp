#include "NavMeshComponent.h"

#include <AssetLoadManager.h>
#include <Development/EditorLogManager.h>
#include <Navigation/NavigationGeometry.h>
#include <Navigation/NavigationManager.h>
#include <Navigation/NavigationProviderInterface.h>
#include <Navigation/NavigationProviderRegistry.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/Property.h>

#if ENGINE_EDITOR_ONLY
#include <Editor/Asset/AssetManager.h>
#include <RenderProxy/RenderSingletonObject.h>
#endif

namespace PigeonEngine
{

	static void SerializeNavMeshBuildSettings(EArchive& InArchive, const CHAR* InName, ENavMeshBuildSettings& InOutValue)
	{
		InArchive.BeginObject(InName);
		InArchive.SerializeFloat(PE_TEXT("CellSize"), InOutValue.CellSize);
		InArchive.SerializeFloat(PE_TEXT("CellHeight"), InOutValue.CellHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentRadius"), InOutValue.AgentRadius);
		InArchive.SerializeFloat(PE_TEXT("AgentHeight"), InOutValue.AgentHeight);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxClimb"), InOutValue.AgentMaxClimb);
		InArchive.SerializeFloat(PE_TEXT("AgentMaxSlopeAngle"), InOutValue.AgentMaxSlopeAngle);
		InArchive.SerializeFloat(PE_TEXT("MaxEdgeLength"), InOutValue.MaxEdgeLength);
		InArchive.SerializeFloat(PE_TEXT("MaxSimplificationError"), InOutValue.MaxSimplificationError);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleDistance"), InOutValue.DetailSampleDistance);
		InArchive.SerializeFloat(PE_TEXT("DetailSampleMaxError"), InOutValue.DetailSampleMaxError);
		InArchive.SerializeUInt32(PE_TEXT("MinRegionArea"), InOutValue.MinRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MergeRegionArea"), InOutValue.MergeRegionArea);
		InArchive.SerializeUInt32(PE_TEXT("MaxVertsPerPoly"), InOutValue.MaxVertsPerPoly);
		InArchive.SerializeUInt32(PE_TEXT("TileSizeCells"), InOutValue.TileSizeCells);
		InArchive.SerializeUInt32(PE_TEXT("MaxLayersPerTile"), InOutValue.MaxLayersPerTile);
		InArchive.SerializeUInt32(PE_TEXT("MaxCrowdAgents"), InOutValue.MaxCrowdAgents);
		InArchive.SerializeBool(PE_TEXT("FlipTriangleWinding"), InOutValue.bFlipTriangleWinding);
		InArchive.EndObject();
	}

	static void AddNavMeshFloatProperty(const size_t& InClassHash, const CHAR* InName,
		FLOAT ENavMeshBuildSettings::* InMember)
	{
		AddClassPropertyAccessors<PNavMeshComponent, FLOAT>(InClassHash, InName,
			[InMember](const PNavMeshComponent* Component) -> FLOAT
			{
				return (Component->GetBuildSettings().*InMember);
			},
			[InMember](PNavMeshComponent* Component, const FLOAT& Value) -> void
			{
				ENavMeshBuildSettings Settings = Component->GetBuildSettings();
				Settings.*InMember = Value;
				Component->SetBuildSettings(Settings);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE) | static_cast<UINT32>(EPropertyFlagsType::EDITOR_ONLY));
	}

	static void AddNavMeshUIntProperty(const size_t& InClassHash, const CHAR* InName,
		UINT32 ENavMeshBuildSettings::* InMember)
	{
		AddClassPropertyAccessors<PNavMeshComponent, UINT32>(InClassHash, InName,
			[InMember](const PNavMeshComponent* Component) -> UINT32
			{
				return (Component->GetBuildSettings().*InMember);
			},
			[InMember](PNavMeshComponent* Component, const UINT32& Value) -> void
			{
				ENavMeshBuildSettings Settings = Component->GetBuildSettings();
				Settings.*InMember = Value;
				Component->SetBuildSettings(Settings);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE) | static_cast<UINT32>(EPropertyFlagsType::EDITOR_ONLY));
	}

	static void AddNavMeshBoolProperty(const size_t& InClassHash, const CHAR* InName,
		BOOL32 ENavMeshBuildSettings::* InMember)
	{
		AddClassBoolPropertyAccessors<PNavMeshComponent>(InClassHash, InName,
			[InMember](const PNavMeshComponent* Component) -> BOOL32
			{
				return (Component->GetBuildSettings().*InMember);
			},
			[InMember](PNavMeshComponent* Component, const BOOL32& Value) -> void
			{
				ENavMeshBuildSettings Settings = Component->GetBuildSettings();
				Settings.*InMember = Value;
				Component->SetBuildSettings(Settings);
			},
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE) | static_cast<UINT32>(EPropertyFlagsType::EDITOR_ONLY));
	}

	// Authored off-mesh connections, kept as one indexed array of objects so a level file stays
	// readable and an older file without the field simply loads no links.
	static void SerializeNavigationLinks(EArchive& InArchive, const CHAR* InName, TArray<ENavigationLink>& InOutLinks)
	{
		UINT32 Count = InOutLinks.Num<UINT32>();
		InArchive.BeginArray(InName, Count);
		if (!InArchive.IsSaving())
		{
			InOutLinks.Empty();
		}
		for (UINT32 Index = 0u; Index < Count; Index++)
		{
			const EString IndexKey = EString::FromInt(Index);
			ENavigationLink Link;
			if (InArchive.IsSaving())
			{
				Link = InOutLinks[Index];
			}
			InArchive.BeginObject(*IndexKey);
			SerializeValue(InArchive, PE_TEXT("Start"), Link.Start);
			SerializeValue(InArchive, PE_TEXT("End"), Link.End);
			InArchive.SerializeFloat(PE_TEXT("Radius"), Link.Radius);
			InArchive.SerializeBool(PE_TEXT("Directional"), Link.bDirectional);
			INT32 AreaValue = static_cast<INT32>(Link.Area);
			SerializeEnumValue(InArchive, PE_TEXT("Area"), AreaValue, GetClassHashCode<ENavigationAreaFlagType>());
			Link.Area = static_cast<ENavigationAreaFlagType>(AreaValue);
			InArchive.EndObject();
			if (!InArchive.IsSaving())
			{
				InOutLinks.Add(Link);
			}
		}
		InArchive.EndArray();
	}

	static void RegisterNavigationLinkStruct()
	{
		const size_t StructHash = GetClassHashCode<ENavigationLink>();
		const EString StructName(PE_TEXT("ENavigationLink"));
		const UINT32 EditFlags = static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE);
		AddStructProperty(StructHash, StructName, PE_TEXT("Start"), &ENavigationLink::Start, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("End"), &ENavigationLink::End, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("Radius"), &ENavigationLink::Radius, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("Directional"), &ENavigationLink::bDirectional, EditFlags);
		AddStructProperty(StructHash, StructName, PE_TEXT("Area"), &ENavigationLink::Area, EditFlags);
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavMeshComponent>();
		AddClassAssetPropertyAccessors<PNavMeshComponent>(ClassHash, PE_TEXT("NavMeshAsset"),
			[](const PNavMeshComponent* Component) -> EString { return Component->GetNavMeshAssetName(); },
			[](PNavMeshComponent* Component, const EString& Value) -> void
			{
				if (Value.Length() > 0u)
				{
					Component->RequestNavMeshAsset(Value);
				}
				else
				{
					Component->ClearNavMeshAsset();
				}
			},
			EPropertyAssetType::NAV_MESH,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PNavMeshComponent, Vector3>(ClassHash, PE_TEXT("BakeVolumeExtent"),
			[](const PNavMeshComponent* Component) -> Vector3 { return Component->GetBakeVolumeExtent(); },
			[](PNavMeshComponent* Component, const Vector3& Value) -> void { Component->SetBakeVolumeExtent(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyAccessors<PNavMeshComponent, EString>(ClassHash, PE_TEXT("NavMeshName"),
			[](const PNavMeshComponent* Component) -> EString { return Component->GetNavMeshName(); },
			[](PNavMeshComponent* Component, const EString& Value) -> void { Component->SetNavMeshName(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PNavMeshComponent>(ClassHash, PE_TEXT("BuildOnBeginPlay"),
			[](const PNavMeshComponent* Component) -> BOOL32 { return Component->GetBuildOnBeginPlay(); },
			[](PNavMeshComponent* Component, const BOOL32& Value) -> void { Component->SetBuildOnBeginPlay(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassPropertyCustom<PNavMeshComponent, ENavMeshBuildSettings>(ClassHash, PE_TEXT("BuildSettings"),
			[](const PNavMeshComponent* Component) -> ENavMeshBuildSettings { return Component->GetBuildSettings(); },
			[](PNavMeshComponent* Component, const ENavMeshBuildSettings& Value) -> void { Component->SetBuildSettings(Value); },
			SerializeNavMeshBuildSettings,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));

		// Off-mesh connections: a struct array edited inline by the generic Details grid, so
		// authoring a jump needs no dedicated panel.
		RegisterNavigationLinkStruct();
		AddClassPropertyStructCustom<PNavMeshComponent, TArray<ENavigationLink>>(ClassHash, PE_TEXT("NavLinks"),
			[](const PNavMeshComponent* Component) -> TArray<ENavigationLink> { return Component->GetNavigationLinks(); },
			[](PNavMeshComponent* Component, const TArray<ENavigationLink>& Value) -> void { Component->SetNavigationLinks(Value); },
			SerializeNavigationLinks,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));

		// Editor views onto the serialized BuildSettings object. Keeping the object as the file
		// format avoids flattening the level schema while still giving each field its own
		// Details row and undo command.
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("CellSize"), &ENavMeshBuildSettings::CellSize);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("CellHeight"), &ENavMeshBuildSettings::CellHeight);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("AgentRadius"), &ENavMeshBuildSettings::AgentRadius);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("AgentHeight"), &ENavMeshBuildSettings::AgentHeight);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("AgentMaxClimb"), &ENavMeshBuildSettings::AgentMaxClimb);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("AgentMaxSlopeAngle"), &ENavMeshBuildSettings::AgentMaxSlopeAngle);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("MaxEdgeLength"), &ENavMeshBuildSettings::MaxEdgeLength);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("MaxSimplificationError"), &ENavMeshBuildSettings::MaxSimplificationError);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("DetailSampleDistance"), &ENavMeshBuildSettings::DetailSampleDistance);
		AddNavMeshFloatProperty(ClassHash, PE_TEXT("DetailSampleMaxError"), &ENavMeshBuildSettings::DetailSampleMaxError);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("MinRegionArea"), &ENavMeshBuildSettings::MinRegionArea);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("MergeRegionArea"), &ENavMeshBuildSettings::MergeRegionArea);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("MaxVertsPerPoly"), &ENavMeshBuildSettings::MaxVertsPerPoly);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("TileSizeCells"), &ENavMeshBuildSettings::TileSizeCells);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("MaxLayersPerTile"), &ENavMeshBuildSettings::MaxLayersPerTile);
		AddNavMeshUIntProperty(ClassHash, PE_TEXT("MaxCrowdAgents"), &ENavMeshBuildSettings::MaxCrowdAgents);
		AddNavMeshBoolProperty(ClassHash, PE_TEXT("FlipTriangleWinding"), &ENavMeshBuildSettings::bFlipTriangleWinding);
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavMeshComponent, PNavigationComponent>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavMeshComponent::PNavMeshComponent()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshComponent");
#if ENGINE_EDITOR_ONLY
		// Only the editor wireframe of the owner's own extent runs from a tick; builds are driven
		// by ENavigationManager's world build pass, so a shipped game does not tick this component.
		SetIsTickable(TRUE);
#endif
	}

	PNavMeshComponent::~PNavMeshComponent()
	{
	}

	const ENavMeshBuildSettings& PNavMeshComponent::GetBuildSettings() const
	{
		return BuildSettings;
	}

	void PNavMeshComponent::SetBuildSettings(const ENavMeshBuildSettings& InSettings)
	{
		BuildSettings = InSettings;
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	const Vector3& PNavMeshComponent::GetBakeVolumeExtent() const
	{
		return BakeVolumeExtent;
	}

	void PNavMeshComponent::SetBakeVolumeExtent(const Vector3& InExtent)
	{
		BakeVolumeExtent = Vector3(EMath::Max(InExtent.x, 0.0f), EMath::Max(InExtent.y, 0.0f), EMath::Max(InExtent.z, 0.0f));
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	const EString& PNavMeshComponent::GetNavMeshName() const
	{
		return NavMeshName;
	}

	void PNavMeshComponent::SetNavMeshName(const EString& InName)
	{
		if (NavMeshName == InName)
		{
			return;
		}
		// A different profile means a different set of volumes and possibly a different asset.
		NavMeshName = InName;
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	void PNavMeshComponent::RequestBuildNavigation()
	{
		// Building belongs to the level: ask for the world pass, which covers every profile.
		PWorld* World = GetWorld();
		if (World)
		{
			ENavigationManager::GetManagerSingleton()->RequestWorldBuild(*World, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
		}
	}

	BOOL32 PNavMeshComponent::GetBuildOnBeginPlay() const
	{
		return bBuildOnBeginPlay;
	}

	void PNavMeshComponent::SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay)
	{
		bBuildOnBeginPlay = InBuildOnBeginPlay;
	}

	const TArray<ENavigationLink>& PNavMeshComponent::GetNavigationLinks() const
	{
		return NavigationLinks;
	}

	void PNavMeshComponent::SetNavigationLinks(const TArray<ENavigationLink>& InLinks)
	{
		NavigationLinks = InLinks;
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	const ENavMeshAsset* PNavMeshComponent::GetNavMeshAsset() const
	{
		return NavMeshAsset;
	}

	const EString& PNavMeshComponent::GetNavMeshAssetName() const
	{
		if (NavMeshAsset)
		{
			return NavMeshAsset->GetAssetName();
		}
		return NavMeshAssetName;
	}

	void PNavMeshComponent::SetNavMeshAsset(const ENavMeshAsset* InAsset)
	{
		if (NavMeshAsset == InAsset)
		{
			return;
		}
		NavMeshAsset = InAsset;
		if (!!InAsset)
		{
			PE_LOG_LOG(EString(PE_TEXT("Navigation: using navmesh asset '")) + InAsset->GetAssetFullPathName() + EString(PE_TEXT("'.")));
		}
	}

	void PNavMeshComponent::RequestNavMeshAsset(const EString& InAssetName)
	{
		NavMeshAssetName = InAssetName;
		bAssetRequestIssued = FALSE;
		bAssetLoadFailed = FALSE;
		const UINT32 RequestGeneration = ++AssetRequestGeneration;
		if (InAssetName.Length() <= 0u)
		{
			return;
		}

		const EString AssetName = InAssetName;
		const EString AssetPath(EBaseSettings::ENGINE_NAV_MESH_PATH);
		const EString FullPathName = MakeAssetFullPathName(AssetPath, AssetName);
		const EString RequestKey = MakeAssetLoadRequestKey(PE_TEXT("NavMesh"), FullPathName);
		PNavMeshComponent* Component = this;
		TFunction<BOOL32()> Step = [Component, AssetPath, AssetName, FullPathName, RequestKey, RequestGeneration]() -> BOOL32
		{
			// A newer request owns the component state now. The old step must not assign an
			// asset, mark a failure, or consume the new request's bAssetRequestIssued flag.
			if (Component->AssetRequestGeneration != RequestGeneration)
			{
				return TRUE;
			}
			// Cache only, no disk access: this runs every frame until the request settles.
			const ENavMeshAsset* CachedAsset = ENavMeshAssetManager::GetManagerSingleton()->FindNavMeshAsset(AssetPath, AssetName);
			if (!!CachedAsset)
			{
				Component->SetNavMeshAsset(CachedAsset);
				return TRUE;
			}
			// One request, issued once. Re-requesting while the first one is still queued would
			// make a missing file retry and log forever.
			if (!Component->bAssetRequestIssued)
			{
				Component->bAssetRequestIssued = TRUE;
				const ENavMeshAsset* LoadedAsset = nullptr;
				if (ENavMeshAssetManager::GetManagerSingleton()->RequireLoadNavMeshAsset(AssetPath, AssetName, LoadedAsset) && LoadedAsset)
				{
					Component->SetNavMeshAsset(LoadedAsset);
					return TRUE;
				}
				return FALSE;
			}
			if (EAssetLoadManager::GetManagerSingleton()->IsRequestPending(RequestKey))
			{
				return FALSE;
			}
			if (Component->AssetRequestGeneration != RequestGeneration)
			{
				return TRUE;
			}
			// The request finished without producing an asset: the file is missing or was rejected.
			// Report it once and let the bake path take over.
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh asset '")) + FullPathName
				+ EString(PE_TEXT("' is missing or invalid; the level build will bake this profile from the world instead.")));
			Component->bAssetLoadFailed = TRUE;
			return TRUE;
		};
		// Tried once here as well, so an asset that is already loaded is applied before this call
		// returns rather than a frame later.
		if (Step() == FALSE)
		{
			AddDeferredApply(static_cast<TFunction<BOOL32()>&&>(Step));
		}
	}

	void PNavMeshComponent::ClearNavMeshAsset()
	{
		NavMeshAssetName = EString();
		bAssetRequestIssued = FALSE;
		bAssetLoadFailed = FALSE;
		AssetRequestGeneration++;
		SetNavMeshAsset(nullptr);
	}

	BOOL32 PNavMeshComponent::HasOutstandingAssetRequest() const
	{
		return ((NavMeshAssetName.Length() > 0u) && (!NavMeshAsset) && (!bAssetLoadFailed)) ? TRUE : FALSE;
	}

	EName PNavMeshComponent::GetProviderName() const
	{
		return ERecastNavigationProvider::GetProviderName();
	}

	const EString& PNavMeshComponent::GetPrimaryProfileName() const
	{
		return NavMeshName;
	}

	TArray<EBoundAABB> PNavMeshComponent::CollectBakeVolumesInWorld() const
	{
		TArray<EBoundAABB> Volumes;
		const PWorld* World = GetWorld();
		if (World)
		{
			// Volumes that support this profile (an empty support list means all profiles), so
			// one volume set can feed the default agent and a larger one.
			ENavigationGeometryCollector::CollectVolumeBounds(*World, NavMeshName, Volumes);
		}

		// The owner may still describe one region of its own, which is how levels authored
		// before bounds volumes existed keep working. A partially zero extent is an authoring
		// mistake, not "the whole world": it is reported instead of silently ignored.
		const BOOL32 bHasOwnExtent = ((BakeVolumeExtent.x > 0.0f) && (BakeVolumeExtent.y > 0.0f) && (BakeVolumeExtent.z > 0.0f)) ? TRUE : FALSE;
		if (bHasOwnExtent)
		{
			const Vector3 Location = GetComponentWorldLocation();
			Volumes.Add(EBoundAABB(Location - BakeVolumeExtent, Location + BakeVolumeExtent, TRUE));
		}
		else if ((BakeVolumeExtent.x > 0.0f) || (BakeVolumeExtent.y > 0.0f) || (BakeVolumeExtent.z > 0.0f))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: BakeVolumeExtent needs every axis > 0 to be a region; the partial extent of '"))
				+ POBJ_DEBUGNAME_GET(this) + EString(PE_TEXT("' was ignored.")));
		}
		return Volumes;
	}

	// Walks the actor/component tree the geometry collector walks. A child actor's components
	// belong to that actor and are visited through it, so the walk stops at an owner boundary.
	static void CollectOwnersInComponent(PActor& InActor, PSceneComponent& InSceneComponent, TArray<PNavMeshComponent*>& OutOwners)
	{
		if (InSceneComponent.GetOwnerActor() != (&InActor))
		{
			return;
		}
		if (PNavMeshComponent* Owner = dynamic_cast<PNavMeshComponent*>(&InSceneComponent))
		{
			OutOwners.Add(Owner);
		}
		const TArray<PSceneComponent*> Children = InSceneComponent.GetChildrenComponents();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			PSceneComponent* ChildComponent = Children[ChildIndex];
			if (!!ChildComponent)
			{
				CollectOwnersInComponent(InActor, *ChildComponent, OutOwners);
			}
		}
	}

	void PNavMeshComponent::CollectPrimaryOwnersInWorld(const PWorld& InWorld, TArray<PNavMeshComponent*>& OutOwners)
	{
		TArray<PNavMeshComponent*> AllOwners;
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 ActorIndex = 0u, ActorNum = Actors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
		{
			PActor* Actor = Actors[ActorIndex];
			if ((!Actor) || (!Actor->GetRootComponent()))
			{
				continue;
			}
			CollectOwnersInComponent(*Actor, *Actor->GetRootComponent(), AllOwners);
		}

		// One owner per profile: the oldest component publishes, so object id order (creation
		// order) is the deterministic tie break.
		for (UINT32 CandidateIndex = 0u, CandidateNum = AllOwners.Num<UINT32>(); CandidateIndex < CandidateNum; CandidateIndex++)
		{
			PNavMeshComponent* Candidate = AllOwners[CandidateIndex];
			BOOL32 bHasOlderOwner = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < CandidateNum; OtherIndex++)
			{
				PNavMeshComponent* Other = AllOwners[OtherIndex];
				if ((Other != Candidate) && (Other->GetNavMeshName() == Candidate->GetNavMeshName())
					&& (Other->GetUniqueID() < Candidate->GetUniqueID()))
				{
					bHasOlderOwner = TRUE;
					break;
				}
			}
			if (!bHasOlderOwner)
			{
				OutOwners.Add(Candidate);
			}
		}
	}

	BOOL32 PNavMeshComponent::IsPrimaryNavMeshOwner() const
	{
		const PWorld* World = GetWorld();
		if (!World)
		{
			// Not in a world; nothing can be built or shadowed.
			return TRUE;
		}
		TArray<PNavMeshComponent*> Owners;
		CollectPrimaryOwnersInWorld(*World, Owners);
		for (UINT32 Index = 0u, Num = Owners.Num<UINT32>(); Index < Num; Index++)
		{
			if (Owners[Index] == this)
			{
				return TRUE;
			}
		}
		if (!bWarnedNotPrimary)
		{
			bWarnedNotPrimary = TRUE;
			const PNavMeshComponent* PrimaryOwner = nullptr;
			for (UINT32 Index = 0u, Num = Owners.Num<UINT32>(); Index < Num; Index++)
			{
				if (Owners[Index]->GetNavMeshName() == NavMeshName)
				{
					PrimaryOwner = Owners[Index];
					break;
				}
			}
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '"))
				+ ((NavMeshName.Length() > 0u) ? NavMeshName : EString(PE_TEXT("Default")))
				+ EString(PE_TEXT("' is already owned by "))
				+ ((!!PrimaryOwner) ? POBJ_DEBUGNAME_GET(PrimaryOwner) : EString(PE_TEXT("<another component>")))
				+ EString(PE_TEXT("; this component stays inert.")));
		}
		return FALSE;
	}

	ERecastNavigationProvider* PNavMeshComponent::CreateConfiguredProvider() const
	{
		ENavigationProviderInterface* CreatedProvider = ENavigationProviderRegistry::Create(GetProviderName());
		if (!CreatedProvider)
		{
			return nullptr;
		}
		// This component is the Recast backend's author facing half, so the settings it holds are
		// that provider's settings. The cast is the single place where the coupling is visible,
		// and it cannot fail: the provider name comes from the same class the cast targets.
		ERecastNavigationProvider* Provider = dynamic_cast<ERecastNavigationProvider*>(CreatedProvider);
		if (!Provider)
		{
			delete CreatedProvider;
			PE_LOG_ERROR(EString(PE_TEXT("Navigation: the provider registered under the navmesh name is not a Recast provider.")));
			return nullptr;
		}
		Provider->SetSettings(BuildSettings);
		return Provider;
	}

	BOOL32 PNavMeshComponent::ImportAssignedAsset(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		// A non-primary owner importing its own copy would shadow the primary's mesh.
		if (!IsPrimaryNavMeshOwner())
		{
			return FALSE;
		}

		const ENavMeshData* Data = (!!NavMeshAsset) ? NavMeshAsset->GetStoragedResource() : nullptr;
		if ((!Data) || (!Data->IsResourceValid()))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset has no usable data; baking from the world instead.")));
			return FALSE;
		}
		if (Data->GetSettings().ProviderName != GetProviderName().ToString())
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset was baked by '"))
				+ Data->GetSettings().ProviderName + EString(PE_TEXT("', which this component cannot import; baking from the world instead.")));
			return FALSE;
		}

		ENavigationProviderInterface* CreatedProvider = ENavigationProviderRegistry::Create(GetProviderName());
		if (!CreatedProvider)
		{
			return FALSE;
		}
		ERecastNavigationProvider* Provider = dynamic_cast<ERecastNavigationProvider*>(CreatedProvider);
		if (!Provider)
		{
			delete CreatedProvider;
			return FALSE;
		}

		const FNavMeshAssetSettings& AssetSettings = Data->GetSettings();
		Provider->SetSettings(AssetSettings.BuildSettings);

		const TArray<UINT8>& Blob = Data->GetBlob();
		if (!Provider->ImportData(Blob.GetData(), Blob.Num<UINT32>()))
		{
			delete Provider;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: the assigned navmesh asset could not be imported; baking from the world instead.")));
			return FALSE;
		}

		OutStats.PolygonCount = AssetSettings.PolygonCount;
		OutStats.TileCount = AssetSettings.TileCount;
		// An imported mesh never runs the bake path, so its size comes from the blob it was
		// copied out of. It covers the same tile bytes plus the container header.
		OutStats.NavMeshBytes = Blob.Num<UINT32>();

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		Manager->RegisterProvider(GetWorld(), GetUniqueID(), NavMeshName, Provider, OutStats);
		Manager->SetProviderDebugDrawEnabled(GetWorld(), NavMeshName, bDebugDrawEnabled);
		PE_LOG_LOG(EString(PE_TEXT("Navigation: imported navmesh asset '")) + NavMeshAsset->GetAssetFullPathName()
			+ EString(PE_TEXT("' (")) + EString::FromInt(AssetSettings.PolygonCount)
			+ EString(PE_TEXT(" polygons, ")) + EString::FromInt(AssetSettings.TileCount)
			+ EString(PE_TEXT(" tiles).")));
		return TRUE;
	}

	BOOL32 PNavMeshComponent::BuildNavigation(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();
		PWorld* World = GetWorld();
		if (!World)
		{
			return FALSE;
		}
		// One pass builds every profile of the level; the result is reported by the manager.
		ENavigationManager::GetManagerSingleton()->RequestWorldBuild(*World, ENavigationManager::ENavigationWorldBuildReason::EDITOR);
		return FALSE;
	}

#if ENGINE_EDITOR_ONLY
	ERecastNavigationProvider* PNavMeshComponent::BakeProviderSynchronously(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		PWorld* World = GetWorld();
		if (!World)
		{
			return nullptr;
		}
		if (!IsPrimaryNavMeshOwner())
		{
			return nullptr;
		}
		ERecastNavigationProvider* Provider = CreateConfiguredProvider();
		if (!Provider)
		{
			return nullptr;
		}
		const TArray<EBoundAABB> Volumes = CollectBakeVolumesInWorld();
		if ((Volumes.Num<UINT32>() == 0u) && (NavMeshName.Length() > 0u))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh profile '")) + NavMeshName
				+ EString(PE_TEXT("' has no bounds volume; the bake was skipped.")));
			delete Provider;
			return nullptr;
		}
		ENavigationSourceGeometry Geometry;
		ENavigationGeometryCollector::CollectStaticMeshTriangles(*World, Volumes, BuildSettings.bFlipTriangleWinding, Geometry);
		Geometry.Links = NavigationLinks;
		if (!Provider->Build(Geometry, OutStats))
		{
			delete Provider;
			return nullptr;
		}
		return Provider;
	}

	BOOL32 PNavMeshComponent::ReloadNavMeshAsset(EString& OutError)
	{
		OutError = EString();
		LastAssetError = EString();
		if (NavMeshAssetName.Length() <= 0u)
		{
			OutError = EString(PE_TEXT("No navmesh asset name is assigned."));
			LastAssetError = OutError;
			return FALSE;
		}

		AssetRequestGeneration++;
		bAssetRequestIssued = FALSE;
		const ENavMeshAsset* ReloadedAsset = nullptr;
		if (!ENavMeshAssetManager::GetManagerSingleton()->ReloadNavMeshAsset(
			EString(EBaseSettings::ENGINE_NAV_MESH_PATH), NavMeshAssetName, ReloadedAsset, OutError))
		{
			bAssetLoadFailed = TRUE;
			LastAssetError = OutError;
			return FALSE;
		}

		bAssetLoadFailed = FALSE;
		SetNavMeshAsset(ReloadedAsset);

		ENavigationBuildStats Stats;
		if (!ImportAssignedAsset(Stats))
		{
			OutError = EString(PE_TEXT("Reloaded navmesh asset has no usable data; the next level build will bake this profile from the world."));
			LastAssetError = OutError;
			return FALSE;
		}
		return TRUE;
	}

	BOOL32 PNavMeshComponent::SaveProviderAsAsset(ENavigationProviderInterface& InProvider, const EString& InAssetName,
		const ENavigationBuildStats& InStats, EString& OutError, const BOOL32& bInAllowOverwrite)
	{
		OutError = EString();
		LastAssetError = EString();
		if (InAssetName.Length() == 0u)
		{
			OutError = EString(PE_TEXT("Enter a name for the navmesh asset first."));
			LastAssetError = OutError;
			return FALSE;
		}

		TArray<UINT8> Blob;
		if (!InProvider.ExportData(Blob))
		{
			OutError = EString(PE_TEXT("Baked navmesh could not be exported."));
			LastAssetError = OutError;
			return FALSE;
		}

		FNavMeshAssetSettings Settings;
		Settings.ProviderName = GetProviderName().ToString();
		Settings.BuildSettings = BuildSettings;
		Settings.Bounds = InProvider.GetBounds();
		Settings.PolygonCount = InStats.PolygonCount;
		Settings.TileCount = InStats.TileCount;

		const ENavMeshAsset* SavedAsset = nullptr;
		if (!ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			InAssetName, Settings, Blob, EString(EBaseSettings::ENGINE_NAV_MESH_PATH), SavedAsset, OutError, bInAllowOverwrite))
		{
			LastAssetError = OutError;
			return FALSE;
		}

		// Assigned directly instead of through SetNavMeshAsset: that setter logs the load path,
		// and this is a write. The asset manager owns the object; the component only points at it.
		NavMeshAsset = SavedAsset;
		NavMeshAssetName = InAssetName;
		// The browser only rescans after its own create/rename/delete actions, so a bake has to
		// ask for one explicitly.
		EAssetManager::GetManagerSingleton()->RequestContentRefresh();
		return TRUE;
	}

	BOOL32 PNavMeshComponent::SaveBakeAsAsset(const EString& InAssetName, EString& OutError, const BOOL32& bInAllowOverwrite)
	{
		OutError = EString();
		LastAssetError = EString();

		// Deliberately synchronous: this manual action has to publish the same bytes the file
		// holds. The level-wide path is Build Navigation / level save.
		ENavigationBuildStats Stats;
		ERecastNavigationProvider* Provider = BakeProviderSynchronously(Stats);
		if (!Provider)
		{
			OutError = EString(PE_TEXT("Bake produced no navmesh; nothing to save."));
			LastAssetError = OutError;
			return FALSE;
		}

		if (!SaveProviderAsAsset(*Provider, InAssetName, Stats, OutError, bInAllowOverwrite))
		{
			delete Provider;
			return FALSE;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		Manager->RegisterProvider(GetWorld(), GetUniqueID(), NavMeshName, Provider, Stats);
		Manager->SetProviderDebugDrawEnabled(GetWorld(), NavMeshName, bDebugDrawEnabled);
		return TRUE;
	}

	EString PNavMeshComponent::MakeNavMeshAssetName(const EString& InLevelStem, const EString& InNavMeshName)
	{
		if (InNavMeshName.Length() == 0u)
		{
			return InLevelStem + EString(PE_TEXT("_NavMesh"));
		}
		// File name safe form of the profile: a name like "Large Agent" must not split the asset
		// into another directory or lose the suffix.
		EString Sanitized;
		for (UINT32 Index = 0u, Num = InNavMeshName.Length(); Index < Num; Index++)
		{
			const CHAR Character = InNavMeshName[Index];
			const BOOL32 bAllowed = (((Character >= 'A') && (Character <= 'Z'))
				|| ((Character >= 'a') && (Character <= 'z'))
				|| ((Character >= '0') && (Character <= '9'))
				|| (Character == '_') || (Character == '-')) ? TRUE : FALSE;
			if (bAllowed)
			{
				const CHAR CharacterBuffer[2] = { Character, '\0' };
				Sanitized += CharacterBuffer;
			}
			else
			{
				Sanitized += EString(PE_TEXT("_"));
			}
		}
		return InLevelStem + EString(PE_TEXT("_")) + Sanitized + EString(PE_TEXT("_NavMesh"));
	}

	BOOL32 PNavMeshComponent::HasNavMeshAssetFailedToLoad() const
	{
		return bAssetLoadFailed;
	}

	const EString& PNavMeshComponent::GetLastNavMeshAssetError() const
	{
		return LastAssetError;
	}

	void PNavMeshComponent::EditorTick(FLOAT InDeltaTime)
	{
		PSceneComponent::EditorTick(InDeltaTime);

		if ((!bDebugDrawEnabled) || (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled()))
		{
			return;
		}
		if ((BakeVolumeExtent.x <= 0.0f) || (BakeVolumeExtent.y <= 0.0f) || (BakeVolumeExtent.z <= 0.0f))
		{
			// No own box; the profile's coverage comes from PNavMeshBoundsComponent volumes.
			return;
		}

		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		WireframeManager->DrawCuboid(GetComponentWorldLocation(), Quaternion::Identity(),
			BakeVolumeExtent.x * 2.0f, BakeVolumeExtent.y * 2.0f, BakeVolumeExtent.z * 2.0f, Color4::Green());
	}
#endif

}  // namespace PigeonEngine
