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

	// How long the automatic bake waits for the asset queue to drain before it reports an
	// incomplete mesh instead of waiting forever. Long enough for a level load, short enough
	// that a permanently pending request does not leave the level without navigation.
	PE_CONSTEXPR_STATIC FLOAT NavigationAutoBuildWaitSeconds = 30.0f;
	// A failed automatic attempt (no ground yet, for example) is retried at this interval
	// rather than every frame: a bake walks the whole world.
	PE_CONSTEXPR_STATIC FLOAT NavigationAutoBuildRetrySeconds = 1.0f;
	// An edit bumps the input revision on every frame of a drag; the owner waits this long after
	// the last change, so dragging a volume produces one bake instead of dozens.
	PE_CONSTEXPR_STATIC FLOAT NavigationAutoRebuildDebounceSeconds = 0.5f;

	// Walks the same actor/component tree the geometry collector walks to find an owner of the
	// same group that was created before InComponent. Object ids are allocated in creation
	// order, so the oldest owner is the deterministic one.
	static const PNavMeshComponent* FindOlderNavMeshOwnerInComponent(const PActor& InActor
		, const PSceneComponent& InSceneComponent, const PNavMeshComponent& InComponent)
	{
		if (InSceneComponent.GetOwnerActor() != (&InActor))
		{
			return nullptr;
		}
		const PNavMeshComponent* Other = dynamic_cast<const PNavMeshComponent*>(&InSceneComponent);
		if ((!!Other) && (Other != (&InComponent)) && (Other->GetNavMeshName() == InComponent.GetNavMeshName())
			&& (Other->GetUniqueID() < InComponent.GetUniqueID()))
		{
			return Other;
		}
		const TArray<PSceneComponent*> Children = InSceneComponent.GetChildrenComponents();
		for (UINT32 ChildIndex = 0u, ChildNum = Children.Num<UINT32>(); ChildIndex < ChildNum; ChildIndex++)
		{
			const PSceneComponent* ChildComponent = Children[ChildIndex];
			if (!!ChildComponent)
			{
				if (const PNavMeshComponent* Found = FindOlderNavMeshOwnerInComponent(InActor, *ChildComponent, InComponent))
				{
					return Found;
				}
			}
		}
		return nullptr;
	}

	static const PNavMeshComponent* FindOlderNavMeshOwner(const PWorld& InWorld, const PNavMeshComponent& InComponent)
	{
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 ActorIndex = 0u, ActorNum = Actors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
		{
			const PActor* Actor = Actors[ActorIndex];
			if ((!Actor) || (!Actor->GetRootComponent()))
			{
				continue;
			}
			if (const PNavMeshComponent* Found = FindOlderNavMeshOwnerInComponent(*Actor, *Actor->GetRootComponent(), InComponent))
			{
				return Found;
			}
		}
		return nullptr;
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
		// The component has per frame work of its own: the automatic publish polls the asset
		// queue. Requesting it here is what keeps a level that forgot to mark the component
		// tickable from silently never getting a navmesh.
		SetIsTickable(TRUE);
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
		// A different group means a different set of volumes and possibly a different asset.
		NavMeshName = InName;
		ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(GetWorld());
	}

	void PNavMeshComponent::RequestBuildNavigation()
	{
		bBuildRequested = TRUE;
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
		// The provider in the manager was built from the previous source, so the next tick has to
		// publish again. Until then the old provider keeps answering queries.
		bHasBuilt = FALSE;
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
			// One request, issued once. Re-requesting while the first one is still queued would make
			// a missing file retry and log forever.
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
				+ EString(PE_TEXT("' is missing or invalid; the component will bake from the world instead.")));
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

	EName PNavMeshComponent::GetProviderName() const
	{
		return ERecastNavigationProvider::GetProviderName();
	}

	TArray<EBoundAABB> PNavMeshComponent::CollectBakeVolumesInWorld() const
	{
		TArray<EBoundAABB> Volumes;
		const PWorld* World = GetWorld();
		if (World)
		{
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

	BOOL32 PNavMeshComponent::IsPrimaryNavMeshOwner() const
	{
		const PWorld* World = GetWorld();
		if (!World)
		{
			// Not in a world; the manager refuses the publish anyway.
			return TRUE;
		}
		const PNavMeshComponent* OlderOwner = FindOlderNavMeshOwner(*World, *this);
		if (!OlderOwner)
		{
			return TRUE;
		}
		if (!bWarnedNotPrimary)
		{
			bWarnedNotPrimary = TRUE;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh group '")) + NavMeshName
				+ EString(PE_TEXT("' is owned by '")) + POBJ_DEBUGNAME_GET(OlderOwner)
				+ EString(PE_TEXT("'), so '")) + POBJ_DEBUGNAME_GET(this)
				+ EString(PE_TEXT("' will not publish a mesh; one owner per group publishes.")));
		}
		return FALSE;
	}

	void PNavMeshComponent::PublishProvider(ENavigationProviderInterface* InProvider, const ENavigationBuildStats& InStats)
	{
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		Manager->RegisterProvider(GetUniqueID(), NavMeshName, InProvider);
		Manager->SetProviderDebugDrawEnabled(GetUniqueID(), bDebugDrawEnabled);
		LastBuildStats = InStats;
	}

	BOOL32 PNavMeshComponent::HasBuiltNavMesh() const
	{
		return bHasBuilt;
	}

	ERecastNavigationProvider* PNavMeshComponent::CreateProvider()
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

	BOOL32 PNavMeshComponent::StartBakeFromWorld()
	{
		PWorld* World = GetWorld();
		if (!World)
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: build requested for a navmesh component that is not in a world.")));
			return FALSE;
		}
		if (!IsPrimaryNavMeshOwner())
		{
			return FALSE;
		}

		ERecastNavigationProvider* Provider = CreateProvider();
		if (!Provider)
		{
			return FALSE;
		}

		// Regions of this group. A named group without any volume would otherwise bake the
		// whole world into a second navmesh, so it is refused with a message instead.
		const TArray<EBoundAABB> Volumes = CollectBakeVolumesInWorld();
		if ((Volumes.Num<UINT32>() == 0u) && (NavMeshName.Length() > 0u))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh group '")) + NavMeshName
				+ EString(PE_TEXT("' has no bounds volume, so nothing was baked. Add a PNavMeshBoundsComponent with the same NavMeshName.")));
			return FALSE;
		}

		// Collection reads the world, so it stays here; only the Recast part runs on the worker.
		ENavigationSourceGeometry Geometry;
		ENavigationGeometryCollector::CollectStaticMeshTriangles(*World, Volumes, BuildSettings.bFlipTriangleWinding, Geometry);
		// The links belong to this volume, not to the world scan, so they are attached after
		// collection.
		Geometry.Links = NavigationLinks;

		if (!ENavigationManager::GetManagerSingleton()->RequestBuild(GetUniqueID(), NavMeshName, Provider, static_cast<ENavigationSourceGeometry&&>(Geometry)))
		{
			// Either a bake is already running for this volume (nothing to do) or the provider was
			// refused; in both cases this one is not needed.
			delete Provider;
			return FALSE;
		}
		bBakeFailed = FALSE;
		return TRUE;
	}

	BOOL32 PNavMeshComponent::BuildFromAsset(ENavigationBuildStats& OutStats)
	{
		OutStats = ENavigationBuildStats();

		// A second owner of the same group must not import its own copy and shadow the primary's
		// mesh; it stays inert like it does on the bake path.
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
		PublishProvider(Provider, OutStats);
		PE_LOG_LOG(EString(PE_TEXT("Navigation: imported navmesh asset '")) + NavMeshAsset->GetAssetFullPathName()
			+ EString(PE_TEXT("' (")) + EString::FromInt(AssetSettings.PolygonCount)
			+ EString(PE_TEXT(" polygons, ")) + EString::FromInt(AssetSettings.TileCount)
			+ EString(PE_TEXT(" tiles).")));
		return TRUE;
	}

	BOOL32 PNavMeshComponent::BuildNavigation(ENavigationBuildStats& OutStats)
	{
		// The shipped path first: a level that carries a baked mesh never pays for a bake, and a
		// mesh that fails to import falls through to the bake instead of leaving the level without
		// navigation.
		if ((!!NavMeshAsset) && (BuildFromAsset(OutStats)))
		{
			return TRUE;
		}

		// The world path is asynchronous: this only queues it. Returns FALSE because no provider is
		// live yet; the manager publishes it and Tick reports it.
		OutStats = ENavigationBuildStats();
		StartBakeFromWorld();
		return FALSE;
	}

	void PNavMeshComponent::RemovedFromScene()
	{
		// The base unregisters the provider. A component that is added again (a level actor
		// removed and re-added, or a scene exit without destruction) has to publish a new mesh,
		// so the begin-play path must see an unbuilt component and start from a clean wait.
		bHasBuilt = FALSE;
		bBakeFailed = FALSE;
		WaitedSeconds = 0.0f;
		NextAttemptSeconds = 0.0f;
		bHasSeenInputRevision = FALSE;
		PendingRebuildSeconds = 0.0f;
		PNavigationComponent::RemovedFromScene();
	}

	void PNavMeshComponent::Tick(FLOAT InDeltaTime)
	{
		PSceneComponent::Tick(InDeltaTime);

		// The manager publishes a finished bake before the world ticks, so a provider that became
		// ready this frame is already answering queries.
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		const ENavigationBuildStateType BuildState = Manager->GetBuildState(GetUniqueID());
		if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_READY)
		{
			// Also after a rebuild: the panel shows the newest mesh, not the first one.
			bHasBuilt = TRUE;
			LastBuildStats = Manager->GetBuildStats(GetUniqueID());
			bBuildRequested = FALSE;
			bBakeFailed = FALSE;
		}
		else if (BuildState == ENavigationBuildStateType::NAVIGATION_BUILD_FAILED)
		{
			bBuildRequested = FALSE;
			bBakeFailed = TRUE;
		}

		if (bBuildRequested)
		{
			// An explicit rebuild always comes from the world: the button exists to pick up geometry
			// changes, which the asset cannot know about. It is queued, not executed here.
			bBuildRequested = FALSE;
			StartBakeFromWorld();
			return;
		}

		if ((bBuildOnBeginPlay) && (!bHasBuilt))
		{
			TryBuildOnBeginPlay(InDeltaTime);
		}

#if ENGINE_EDITOR_ONLY
		// Authored inputs (bounds volumes, settings, links) changed. The debounce turns a drag
		// into a single rebuild; runtime builds stay on the explicit paths above.
		UpdateAutoRebuild(InDeltaTime);
#endif
	}

#if ENGINE_EDITOR_ONLY
	void PNavMeshComponent::UpdateAutoRebuild(FLOAT InDeltaTime)
	{
		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		const UINT32 Revision = Manager->GetNavigationInputRevision(GetWorld());
		if (!bHasSeenInputRevision)
		{
			// The first tick only records the revision: loading a level adds volumes, and that
			// must not schedule a rebuild of the mesh the level is about to import.
			bHasSeenInputRevision = TRUE;
			LastSeenInputRevision = Revision;
			return;
		}
		if (Revision != LastSeenInputRevision)
		{
			LastSeenInputRevision = Revision;
			// Restart the countdown on every change, so a drag produces one bake at its end.
			PendingRebuildSeconds = NavigationAutoRebuildDebounceSeconds;
			return;
		}
		if (PendingRebuildSeconds <= 0.0f)
		{
			return;
		}
		PendingRebuildSeconds -= InDeltaTime;
		if (PendingRebuildSeconds > 0.0f)
		{
			return;
		}
		PendingRebuildSeconds = 0.0f;

		// Begin-play owns the first build, and a non-primary owner publishes nothing at all.
		if ((!bHasBuilt) || (!IsPrimaryNavMeshOwner()))
		{
			return;
		}
		if (Manager->GetBuildState(GetUniqueID()) == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			// Let the running bake finish; the next input change schedules the following one.
			return;
		}
		StartBakeFromWorld();
	}

	BOOL32 PNavMeshComponent::IsRebuildPending() const
	{
		return (PendingRebuildSeconds > 0.0f) ? TRUE : FALSE;
	}
#endif

	void PNavMeshComponent::TryBuildOnBeginPlay(FLOAT InDeltaTime)
	{
		// A requested asset is part of the load queue, but its completion callback (the deferred
		// apply that assigns it) can run a frame after the queue drains. Waiting for that frame
		// prevents baking a mesh the asset was about to replace.
		const BOOL32 bAssetOutstanding = ((NavMeshAssetName.Length() > 0u) && (!NavMeshAsset) && (!bAssetLoadFailed)) ? TRUE : FALSE;
		if (bAssetOutstanding)
		{
			return;
		}

		WaitedSeconds += InDeltaTime;
		if (WaitedSeconds < NextAttemptSeconds)
		{
			return;
		}

		// Baking before the static mesh assets are loaded produces a navmesh with holes, and the
		// cold data of a mesh component is null until its request finishes. The queue is the only
		// signal available, so it gates the first bake.
		const UINT32 PendingNum = EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum();
		const BOOL32 bQueueDrained = (PendingNum == 0u);
		const BOOL32 bWaitExpired = (WaitedSeconds >= NavigationAutoBuildWaitSeconds);
		if ((!bQueueDrained) && (!bWaitExpired))
		{
			return;
		}

		ENavigationManager* Manager = ENavigationManager::GetManagerSingleton();
		if (Manager->GetBuildState(GetUniqueID()) == ENavigationBuildStateType::NAVIGATION_BUILD_RUNNING)
		{
			// A queued bake is in flight; Tick reports the result.
			return;
		}

		if (!!NavMeshAsset)
		{
			// Importing an asset is a memory copy and finishes the publish right here.
			ENavigationBuildStats AssetStats;
			if (BuildFromAsset(AssetStats))
			{
				bHasBuilt = TRUE;
				return;
			}
		}

		if (bBakeFailed)
		{
			// The last queued bake produced nothing. Retrying only helps while the asset queue is
			// still busy; otherwise the level has nothing rasterizable and a manual rebuild is the
			// only way forward.
			if (PendingNum == 0u)
			{
				bHasBuilt = TRUE;
				PE_LOG_WARN(EString(PE_TEXT("Navigation: no navmesh could be built from the current world; use Rebuild From World after adding walkable geometry.")));
				return;
			}
			NextAttemptSeconds = WaitedSeconds + NavigationAutoBuildRetrySeconds;
			return;
		}

		ENavigationBuildStats Stats;
		if (!BuildNavigation(Stats))
		{
			// Either the bake was queued (the usual case) or nothing could be collected. Both are
			// observed through the build state on the following ticks.
			return;
		}
		if (Stats.SkippedMeshCount == 0u)
		{
			bHasBuilt = TRUE;
			return;
		}
		if (PendingNum == 0u)
		{
			// Nothing is loading any more, so the components that were skipped are not going to
			// recover by themselves. Keep the incomplete mesh and let the warning say so; a manual
			// rebuild picks up whatever was fixed.
			bHasBuilt = TRUE;
			PE_LOG_WARN(EString(PE_TEXT("Navigation: automatic bake kept a mesh with "))
				+ EString::FromInt(Stats.SkippedMeshCount)
				+ EString(PE_TEXT(" skipped static mesh components; rebuild manually once they load.")));
			return;
		}
		// Something is still loading: bake again shortly instead of freezing the holes into the
		// navmesh for the rest of the session.
		NextAttemptSeconds = WaitedSeconds + NavigationAutoBuildRetrySeconds;
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
		ERecastNavigationProvider* Provider = CreateProvider();
		if (!Provider)
		{
			return nullptr;
		}
		const TArray<EBoundAABB> Volumes = CollectBakeVolumesInWorld();
		if ((Volumes.Num<UINT32>() == 0u) && (NavMeshName.Length() > 0u))
		{
			PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh group '")) + NavMeshName
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
		if (!BuildFromAsset(Stats))
		{
			bHasBuilt = FALSE;
			OutError = EString(PE_TEXT("Reloaded navmesh asset has no usable data; the world bake will run instead."));
			LastAssetError = OutError;
			return FALSE;
		}
		bHasBuilt = TRUE;
		return TRUE;
	}

	BOOL32 PNavMeshComponent::SaveBakeAsAsset(const EString& InAssetName, EString& OutError, const BOOL32& bInAllowOverwrite)
	{
		OutError = EString();
		LastAssetError = EString();
		if (InAssetName.Length() <= 0u)
		{
			OutError = EString(PE_TEXT("Enter a name for the navmesh asset first."));
			LastAssetError = OutError;
			return FALSE;
		}

		// Deliberately synchronous: saving is an explicit editor action, and it has to publish the
		// same bytes the file holds.
		ENavigationBuildStats Stats;
		ERecastNavigationProvider* Provider = BakeProviderSynchronously(Stats);
		if (!Provider)
		{
			OutError = EString(PE_TEXT("Bake produced no navmesh; nothing to save."));
			LastAssetError = OutError;
			return FALSE;
		}

		TArray<UINT8> Blob;
		if (!Provider->ExportData(Blob))
		{
			delete Provider;
			OutError = EString(PE_TEXT("Baked navmesh could not be exported."));
			LastAssetError = OutError;
			return FALSE;
		}

		FNavMeshAssetSettings Settings;
		Settings.ProviderName = GetProviderName().ToString();
		Settings.BuildSettings = BuildSettings;
		Settings.Bounds = Provider->GetBounds();
		Settings.PolygonCount = Stats.PolygonCount;
		Settings.TileCount = Stats.TileCount;

		const ENavMeshAsset* SavedAsset = nullptr;
		if (!ENavMeshAssetManager::GetManagerSingleton()->CreateNavMeshAsset(
			InAssetName, Settings, Blob, EString(EBaseSettings::ENGINE_NAV_MESH_PATH), SavedAsset, OutError, bInAllowOverwrite))
		{
			delete Provider;
			LastAssetError = OutError;
			return FALSE;
		}

		// Publish the provider that was written, then point the component at the file so the next
		// load of the level takes the asset path. The flag is set after SetNavMeshAsset because
		// assigning an asset deliberately clears it to force a republish.
		PublishProvider(Provider, Stats);
		SetNavMeshAsset(SavedAsset);
		NavMeshAssetName = InAssetName;
		bHasBuilt = TRUE;
		// The browser only rescans after its own create/rename/delete actions, so a bake has to
		// ask for one explicitly.
		EAssetManager::GetManagerSingleton()->RequestContentRefresh();
		return TRUE;
	}

	BOOL32 PNavMeshComponent::HasNavMeshAssetFailedToLoad() const
	{
		return bAssetLoadFailed;
	}

	const EString& PNavMeshComponent::GetLastNavMeshAssetError() const
	{
		return LastAssetError;
	}

	BOOL32 PNavMeshComponent::AutoAssignNamedNavMeshAsset(const EString& InLevelStem, EString& OutError)
	{
		OutError = EString();
		// The author named this component's asset themselves; the convention name must not
		// override an explicit choice.
		if (NavMeshAssetName.Length() > 0u)
		{
			return TRUE;
		}
		if (InLevelStem.Length() <= 0u)
		{
			return TRUE;
		}
		// Nothing published means nothing worth exporting; a fresh checkout loads the level
		// and bakes, or the author names the asset in the panel.
		if (!bHasBuilt)
		{
			return TRUE;
		}
		// Only the primary owner of a group may write the group's asset; a second owner would
		// otherwise overwrite it with a different bake.
		if (!IsPrimaryNavMeshOwner())
		{
			return TRUE;
		}
		// The convention name is generated from this level and group, so re-saving regenerates
		// it; manual panel saves keep refusing to overwrite.
		const EString AssetName = MakeNavMeshAssetName(InLevelStem, NavMeshName);
		if (!SaveBakeAsAsset(AssetName, OutError, TRUE))
		{
			return FALSE;
		}
		return TRUE;
	}

	EString PNavMeshComponent::MakeNavMeshAssetName(const EString& InLevelStem, const EString& InNavMeshName)
	{
		if (InNavMeshName.Length() == 0u)
		{
			return InLevelStem + EString(PE_TEXT("_NavMesh"));
		}
		// File name safe form of the group: a name like "Large Agent" must not split the asset
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

	void PNavMeshComponent::EditorTick(FLOAT InDeltaTime)
	{
		PSceneComponent::EditorTick(InDeltaTime);

		if ((!bDebugDrawEnabled) || (!ENavigationManager::GetManagerSingleton()->IsDebugDrawEnabled()))
		{
			return;
		}
		if ((BakeVolumeExtent.x <= 0.0f) || (BakeVolumeExtent.y <= 0.0f) || (BakeVolumeExtent.z <= 0.0f))
		{
			// A whole world volume has no box worth drawing.
			return;
		}

		RDebugWireframePrimitiveManager* WireframeManager = RDebugWireframePrimitiveManager::GetManagerSingleton();
		WireframeManager->DrawCuboid(GetComponentWorldLocation(), Quaternion::Identity(),
			BakeVolumeExtent.x * 2.0f, BakeVolumeExtent.y * 2.0f, BakeVolumeExtent.z * 2.0f, Color4::Green());
	}
#endif

}  // namespace PigeonEngine
