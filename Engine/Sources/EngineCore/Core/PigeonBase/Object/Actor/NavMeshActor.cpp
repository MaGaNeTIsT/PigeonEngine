#include "NavMeshActor.h"

#include <Navigation/NavigationManager.h>
#include <PigeonBase/Object/World/World.h>
#include <Reflection/Property.h>
#include <Development/EditorLogManager.h>

namespace PigeonEngine
{

	// Authoring safety net: agent names are the key the build pass routes by, so a duplicate is
	// an authoring mistake. Report it where it happens (actor added, profile edited) instead of
	// letting two actors silently fight over one mesh. The build keeps using the earliest actor.
	static void WarnIfAgentNameIsTaken(const PNavMeshActor& InActor, PWorld& InWorld)
	{
		const EString& AgentName = InActor.GetAgentName();
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 Index = 0u, Num = Actors.Num<UINT32>(); Index < Num; Index++)
		{
			const PNavMeshActor* Other = dynamic_cast<const PNavMeshActor*>(Actors[Index]);
			if ((!!Other) && (Other != (&InActor)) && (Other->GetAgentName() == AgentName))
			{
				PE_LOG_WARN(EString(PE_TEXT("Navigation: navmesh agent '"))
					+ ((AgentName.Length() > 0u) ? AgentName : EString(PE_TEXT("Default")))
					+ EString(PE_TEXT("' is also configured by '")) + POBJ_DEBUGNAME_GET(Other)
					+ EString(PE_TEXT("'; the earliest created navmesh actor provides this agent.")));
				return;
			}
		}
	}

	static void RegisterClassProperties()
	{
		const size_t ClassHash = GetClassHashCode<PNavMeshActor>();
		// Shared struct descriptors first: the Profile property resolves its nested settings and
		// link descriptors at registration time, and the Details grid needs them for editing.
		RegisterNavMeshProfileTypes();
		AddClassPropertyStructCustom<PNavMeshActor, FNavMeshProfile>(ClassHash, PE_TEXT("Profile"),
			[](const PNavMeshActor* Actor) -> FNavMeshProfile { return Actor->GetProfile(); },
			[](PNavMeshActor* Actor, const FNavMeshProfile& Value) -> void { Actor->SetProfile(Value); },
			SerializeNavMeshProfile,
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
		AddClassBoolPropertyAccessors<PNavMeshActor>(ClassHash, PE_TEXT("BuildOnBeginPlay"),
			[](const PNavMeshActor* Actor) -> BOOL32 { return Actor->GetBuildOnBeginPlay(); },
			[](PNavMeshActor* Actor, const BOOL32& Value) -> void { Actor->SetBuildOnBeginPlay(Value); },
			static_cast<UINT32>(EPropertyFlagsType::EDIT_ANYWHERE));
	}

	static void RegisterClassTypes()
	{
		RegisterClassType<PNavMeshActor, PActor>();
		RegisterClassProperties();
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	PNavMeshActor::PNavMeshActor()
	{
		POBJ_DEBUGNAME_SET(this, "NavMeshActor");
	}

	PNavMeshActor::~PNavMeshActor()
	{
	}

	const FNavMeshProfile& PNavMeshActor::GetProfile() const
	{
		return Profile;
	}

	void PNavMeshActor::SetProfile(const FNavMeshProfile& InProfile)
	{
		Profile = InProfile;
		// The profile is bake input: a settings/link/name edit has to schedule a rebuild.
		if (PWorld* World = GetWorld())
		{
			WarnIfAgentNameIsTaken(*this, *World);
			ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(World);
		}
	}

	const EString& PNavMeshActor::GetAgentName() const
	{
		return Profile.Name;
	}

	const ENavMeshBuildSettings& PNavMeshActor::GetBuildSettings() const
	{
		return Profile.BuildSettings;
	}

	const TArray<ENavigationLink>& PNavMeshActor::GetNavigationLinks() const
	{
		return Profile.NavLinks;
	}

	const EString& PNavMeshActor::GetNavMeshAssetName() const
	{
		return Profile.NavMeshAsset;
	}

	void PNavMeshActor::SetNavMeshAssetName(const EString& InAssetName)
	{
		Profile.NavMeshAsset = InAssetName;
	}

	BOOL32 PNavMeshActor::GetBuildOnBeginPlay() const
	{
		return bBuildOnBeginPlay;
	}

	void PNavMeshActor::SetBuildOnBeginPlay(const BOOL32& InBuildOnBeginPlay)
	{
		bBuildOnBeginPlay = InBuildOnBeginPlay;
	}

	void PNavMeshActor::CollectPrimaryActorsInWorld(const PWorld& InWorld, TArray<PNavMeshActor*>& OutActors)
	{
		TArray<PNavMeshActor*> AllActors;
		const TArray<PActor*> Actors = InWorld.GetAllActors();
		for (UINT32 ActorIndex = 0u, ActorNum = Actors.Num<UINT32>(); ActorIndex < ActorNum; ActorIndex++)
		{
			PActor* Actor = Actors[ActorIndex];
			if (PNavMeshActor* NavMeshActor = dynamic_cast<PNavMeshActor*>(Actor))
			{
				AllActors.Add(NavMeshActor);
			}
		}

		// One actor per agent name: the earliest created publishes. Object ids are allocated in
		// creation order, so the tie break is deterministic.
		for (UINT32 CandidateIndex = 0u, CandidateNum = AllActors.Num<UINT32>(); CandidateIndex < CandidateNum; CandidateIndex++)
		{
			PNavMeshActor* Candidate = AllActors[CandidateIndex];
			BOOL32 bHasOlderActor = FALSE;
			for (UINT32 OtherIndex = 0u; OtherIndex < CandidateNum; OtherIndex++)
			{
				PNavMeshActor* Other = AllActors[OtherIndex];
				if ((Other != Candidate) && (Other->GetAgentName() == Candidate->GetAgentName())
					&& (Other->GetUniqueID() < Candidate->GetUniqueID()))
				{
					bHasOlderActor = TRUE;
					break;
				}
			}
			if (!bHasOlderActor)
			{
				OutActors.Add(Candidate);
			}
		}
	}

	void PNavMeshActor::BeginAddedToScene(PWorld* World)
	{
		PActor::BeginAddedToScene(World);
		if (World)
		{
			WarnIfAgentNameIsTaken(*this, *World);
			ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(World);
		}
	}

	void PNavMeshActor::RemovedFromScene()
	{
		// Providers published from this actor are keyed inside its world by agent name; the source
		// id removes exactly the ones this actor authored.
		if (PWorld* World = GetWorld())
		{
			ENavigationManager::GetManagerSingleton()->UnregisterProfilesBySource(World, GetUniqueID());
			ENavigationManager::GetManagerSingleton()->BumpNavigationInputRevision(World);
		}
		PActor::RemovedFromScene();
	}

}  // namespace PigeonEngine
