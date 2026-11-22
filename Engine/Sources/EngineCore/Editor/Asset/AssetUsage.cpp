#include <Editor/Asset/AssetUsage.h>

#if ENGINE_EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Actor/NavMeshActor.h>
#include <PigeonBase/Object/World/World.h>
#include <PigeonBase/Object/World/WorldManager.h>

namespace PigeonEngine
{
	namespace EAssetUsage
	{
		namespace
		{
			void AddUniqueUser(TArray<EString>& OutUsers, const EString& InUserName)
			{
				if ((InUserName.Length() > 0u) && (OutUsers.Find(InUserName) == static_cast<UINT32>(-1)))
				{
					OutUsers.Add(InUserName);
				}
			}
		}

		void CollectUsers(const EString& InAssetPath, const EAssetType& InType, TArray<EString>& OutUsers)
		{
			OutUsers.Empty();
			const EString AssetName = EPath::GetFileNameWithoutExtension(InAssetPath);
			if (AssetName.Length() <= 0u)
			{
				return;
			}

			if (InType == ASSET_TYPE_ANIMATION)
			{
				TArray<const EAnimMontageAsset*> MontageAssets;
				EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
				for (INT32 AssetIndex = 0, AssetNum = MontageAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const EAnimMontage* Montage = (MontageAssets[AssetIndex]
						? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (Montage && (Montage->AnimationAssetName == AssetName))
					{
						AddUniqueUser(OutUsers, MontageAssets[AssetIndex]->GetAssetName());
					}
				}
				TArray<const EAnimStateMachineAsset*> MachineAssets;
				EAnimStateMachineManager::GetManagerSingleton()->GetLoadedAssets(MachineAssets);
				for (INT32 AssetIndex = 0, AssetNum = MachineAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const EAnimStateMachine* Machine = (MachineAssets[AssetIndex]
						? MachineAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (Machine && (Machine->AnimationAssetName == AssetName))
					{
						AddUniqueUser(OutUsers, MachineAssets[AssetIndex]->GetAssetName());
					}
				}
			}
			else if (InType == ASSET_TYPE_SKELETON)
			{
				TArray<const EAnimMontageAsset*> MontageAssets;
				EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
				for (INT32 AssetIndex = 0, AssetNum = MontageAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const EAnimMontage* Montage = (MontageAssets[AssetIndex]
						? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (Montage && (Montage->SkeletonName == AssetName))
					{
						AddUniqueUser(OutUsers, MontageAssets[AssetIndex]->GetAssetName());
					}
				}
				TArray<const EAnimStateMachineAsset*> MachineAssets;
				EAnimStateMachineManager::GetManagerSingleton()->GetLoadedAssets(MachineAssets);
				for (INT32 AssetIndex = 0, AssetNum = MachineAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const EAnimStateMachine* Machine = (MachineAssets[AssetIndex]
						? MachineAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (Machine && (Machine->SkeletonName == AssetName))
					{
						AddUniqueUser(OutUsers, MachineAssets[AssetIndex]->GetAssetName());
					}
				}
				TArray<const ESkeletonAnimationAsset*> AnimationAssets;
				EAnimationManager::GetManagerSingleton()->GetLoadedAssets(AnimationAssets);
				for (INT32 AssetIndex = 0, AssetNum = AnimationAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const ESkeletonAnimation* Animation = (AnimationAssets[AssetIndex]
						? AnimationAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (Animation && (Animation->GetSkeletonName() == AssetName))
					{
						AddUniqueUser(OutUsers, AnimationAssets[AssetIndex]->GetAssetName());
					}
				}
			}
			else if (InType == ASSET_TYPE_ANIM_NOTIFY)
			{
				TArray<const ESkeletonAnimationAsset*> AnimationAssets;
				EAnimationManager::GetManagerSingleton()->GetLoadedAssets(AnimationAssets);
				for (INT32 AssetIndex = 0, AssetNum = AnimationAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const ESkeletonAnimation* Animation = (AnimationAssets[AssetIndex]
						? AnimationAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (!Animation)
					{
						continue;
					}
					BOOL32 bUsed = FALSE;
					for (INT32 ClipIndex = 0; (ClipIndex < Animation->GetAnimationClipNum()) && (bUsed == FALSE); ClipIndex++)
					{
						const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
						for (INT32 NotifyIndex = 0; Clip && (NotifyIndex < Clip->Notifies.Num()); NotifyIndex++)
						{
							if (Clip->Notifies[NotifyIndex].NotifyAssetName == AssetName)
							{
								bUsed = TRUE;
								break;
							}
						}
					}
					if (bUsed != FALSE)
					{
						AddUniqueUser(OutUsers, AnimationAssets[AssetIndex]->GetAssetName());
					}
				}

				TArray<const EAnimMontageAsset*> MontageAssets;
				EAnimMontageManager::GetManagerSingleton()->GetLoadedAssets(MontageAssets);
				for (INT32 AssetIndex = 0, AssetNum = MontageAssets.Num<INT32>(); AssetIndex < AssetNum; AssetIndex++)
				{
					const EAnimMontage* Montage = (MontageAssets[AssetIndex]
						? MontageAssets[AssetIndex]->GetStoragedResource() : nullptr);
					if (!Montage)
					{
						continue;
					}
					BOOL32 bUsed = FALSE;
					for (INT32 SectionIndex = 0; (SectionIndex < Montage->Sections.Num()) && (bUsed == FALSE); SectionIndex++)
					{
						const TArray<FAnimNotifyEvent>& Notifies = Montage->Sections[SectionIndex].Notifies;
						for (INT32 NotifyIndex = 0; NotifyIndex < Notifies.Num(); NotifyIndex++)
						{
							if (Notifies[NotifyIndex].NotifyAssetName == AssetName)
							{
								bUsed = TRUE;
								break;
							}
						}
					}
					if (bUsed != FALSE)
					{
						AddUniqueUser(OutUsers, MontageAssets[AssetIndex]->GetAssetName());
					}
				}
			}
			else if (InType == ASSET_TYPE_NAV_MESH)
			{
				// Navmesh assets are referenced by the agent profiles of the open level, so the
				// users are its navmesh actors.
				PWorld* World = EWorldManager::GetManagerSingleton()->GetWorld();
				if (World)
				{
					const TArray<PActor*>& Actors = World->GetAllActors();
					for (INT32 ActorIndex = 0, ActorNum = Actors.Num<INT32>(); ActorIndex < ActorNum; ActorIndex++)
					{
						PActor* Actor = Actors[ActorIndex];
						const PNavMeshActor* NavMeshActor = dynamic_cast<const PNavMeshActor*>(Actor);
						if ((!!NavMeshActor) && (NavMeshActor->GetNavMeshAssetName() == AssetName))
						{
							AddUniqueUser(OutUsers, POBJ_DEBUGNAME_GET(Actor));
						}
					}
				}
			}
			else if ((InType == ASSET_TYPE_ANIM_MONTAGE) || (InType == ASSET_TYPE_ANIM_STATE_MACHINE))
			{
				// No loaded runtime object references these by name today: they are opened
				// as documents, and rename/delete already refuses while the document is open.
				// The branches stay explicit so adding a reference later lands here.
			}
		}
	}
}

#endif
