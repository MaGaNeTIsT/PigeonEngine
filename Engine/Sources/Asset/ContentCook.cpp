#include "ContentCook.h"

#if ENGINE_EDITOR_ONLY

#include <Asset/AssetLoadManager.h>
#include <Asset/AssetSourceRegistry.h>
#include <Config/EngineConfig.h>
#include <Development/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <MeshAsset/MeshAsset.h>
#include <PigeonBase/Object/Actor.h>
#include <PigeonBase/Object/Component/Primitive/StaticMeshComponent.h>
#include <PigeonBase/Object/World/World.h>
#include <RapidJson/Headers/JsonObject.h>
#include <TextureAsset/TextureAsset.h>

#include <cstdlib>
#include <cstring>

namespace PigeonEngine
{
	namespace
	{
		enum class ECookPhase : UINT8
		{
			IDLE = 0,
			WAIT_IMPORT,
			VALIDATE_LEVELS,
			WAIT_LEVEL,
			REPORT,
			DONE,
		};

		struct FCookLevelResult
		{
			EString				Path;
			UINT32				ActorNum = 0u;
			UINT32				MeshNum = 0u;
			UINT32				MeshBoundNum = 0u;
			TArray<EString>		MissingMeshNames;
		};

		static ECookPhase	GCookPhase = ECookPhase::IDLE;
		static BOOL32		GCookReadRequest = FALSE;
		static BOOL32		GCookRequested = FALSE;
		static UINT32		GCookWaitFrames = 0u;
		static UINT32		GCookIdleFrames = 0u;
		static BOOL32		GCookChildrenRequested = FALSE;
		static BOOL32		GCookSecondPassRequested = FALSE;
		static UINT32		GCookRequestedMeshNum = 0u;
		static UINT32		GCookRequestedTextureNum = 0u;
		static TArray<EString>	GCookLevelPaths;
		static UINT32		GCookLevelIndex = 0u;
		static TArray<FCookLevelResult>	GCookLevelResults;

		static BOOL32 IsCookRequested()
		{
			if (GCookReadRequest == FALSE)
			{
				GCookReadRequest = TRUE;
				char* Value = nullptr;
				size_t ValueLength = 0u;
				if ((_dupenv_s(&Value, &ValueLength, "PIGEON_COOK_CONTENT") == 0) && (Value != nullptr))
				{
					GCookRequested = (std::strcmp(Value, "1") == 0) ? TRUE : FALSE;
					free(Value);
				}
			}
			return GCookRequested;
		}

		static void GatherLevelPaths()
		{
			GCookLevelPaths.Empty();
			TArray<EString> Folders;
			TArray<EString> Files;
			if (!EFileHelper::ScanDirectory(EString(EBaseSettings::ENGINE_LEVEL_PATH), Folders, Files, FALSE))
			{
				return;
			}
			for (UINT32 FileIndex = 0u; FileIndex < Files.Num<UINT32>(); FileIndex++)
			{
				const EString& FileName = Files[FileIndex];
				if ((FileName.Length() >= 7u) && (FileName.Right(7u) == EString(PE_TEXT(".PLevel"))))
				{
					// ScanDirectory may hand back a bare name or a full path depending on the
					// working directory; accept both and never double-prefix.
					const EString Candidate = EFileHelper::IsFileExists(FileName)
						? FileName : (EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
					GCookLevelPaths.Add(Candidate);
				}
			}
		}

		static void RequestRegistryAssets()
		{
			GCookRequestedMeshNum = 0u;
			GCookRequestedTextureNum = 0u;
			const TArray<FAssetSourceEntry>& Entries = EAssetSourceRegistry::GetEntries();
			for (UINT32 EntryIndex = 0u; EntryIndex < Entries.Num<UINT32>(); EntryIndex++)
			{
				const FAssetSourceEntry& Entry = Entries[EntryIndex];
				if (Entry.Family == EAssetSourceFamilyType::MESH)
				{
					const EStaticMeshAsset* IgnoredAsset = nullptr;
					// The cook owns the source: pass it explicitly instead of relying on the
					// registry fallback, which may be deduped by a request a level made first.
					EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
						EBaseSettings::ENGINE_MESH_PATH, Entry.AssetName, IgnoredAsset, &Entry.ImportSource);
					GCookRequestedMeshNum++;
				}
				else if (Entry.Family == EAssetSourceFamilyType::TEXTURE_2D)
				{
					const ETexture2DAsset* IgnoredAsset = nullptr;
					ETextureAssetManager::GetManagerSingleton()->RequireLoadTexture2DAsset(
						EBaseSettings::ENGINE_TEXTURE_PATH, Entry.AssetName, IgnoredAsset, &Entry.ImportSource);
					GCookRequestedTextureNum++;
				}
				else if (Entry.Family == EAssetSourceFamilyType::TEXTURE_CUBE)
				{
					const ETextureCubeAsset* IgnoredAsset = nullptr;
					ETextureAssetManager::GetManagerSingleton()->RequireLoadTextureCubeAsset(
						EBaseSettings::ENGINE_TEXTURE_PATH, Entry.AssetName, IgnoredAsset, &Entry.CubeSource);
					GCookRequestedTextureNum++;
				}
			}
			PE_LOG_LOG(EString(PE_TEXT("[ContentCook] Requested ")) + ToString(GCookRequestedMeshNum) + EString(PE_TEXT(" meshes and "))
				+ ToString(GCookRequestedTextureNum) + EString(PE_TEXT(" textures from the source registry.")));
		}

		static void RequestSceneChildren()
		{
			UINT32 ChildNum = 0u;
			const TArray<FAssetSourceEntry>& Entries = EAssetSourceRegistry::GetEntries();
			for (UINT32 EntryIndex = 0u; EntryIndex < Entries.Num<UINT32>(); EntryIndex++)
			{
				const FAssetSourceEntry& Entry = Entries[EntryIndex];
				if ((Entry.Family != EAssetSourceFamilyType::MESH) || (Entry.OutputPrefix.Length() == 0u))
				{
					continue;
				}
				const EString ManifestPath = EString(EBaseSettings::ENGINE_MESH_PATH) + Entry.AssetName + EString(PE_TEXT(".Scene.json"));
				EString ManifestText;
				if (!EFileHelper::ReadFileAsString(ManifestPath, ManifestText))
				{
					PE_LOG_WARN(EString(PE_TEXT("[ContentCook] No scene manifest at ")) + ManifestPath);
					continue;
				}
				CJsonObject RootObject(ManifestText);
				TArray<CJsonObject*> MeshObjects;
				if ((!RootObject.IsValid()) || (!RootObject.GetObjectArrayField(EString(PE_TEXT("Meshes")), MeshObjects)))
				{
					continue;
				}
				for (UINT32 MeshIndex = 0u; MeshIndex < MeshObjects.Num<UINT32>(); MeshIndex++)
				{
					EString AssetName;
					if (MeshObjects[MeshIndex] && MeshObjects[MeshIndex]->GetStringField(EString(PE_TEXT("Asset")), AssetName))
					{
						const EStaticMeshAsset* IgnoredAsset = nullptr;
						EMeshAssetManager::GetManagerSingleton()->RequireLoadStaticMeshAsset(
							EBaseSettings::ENGINE_MESH_PATH, AssetName, IgnoredAsset);
						ChildNum++;
					}
				}
			}
			GCookRequestedMeshNum += ChildNum;
			PE_LOG_LOG(EString(PE_TEXT("[ContentCook] Requested ")) + ToString(ChildNum) + EString(PE_TEXT(" scene child meshes.")));
		}

		static BOOL32 IsLoadQueueIdle()
		{
			return (EAssetLoadManager::GetManagerSingleton()->GetPendingRequestNum() == 0u) ? TRUE : FALSE;
		}

		static void InspectLoadedLevel(PWorld& InWorld, FCookLevelResult& OutResult)
		{
			OutResult.ActorNum = InWorld.GetActorCount();
			const TArray<PActor*> Actors = InWorld.GetAllActors();
			for (UINT32 ActorIndex = 0u; ActorIndex < Actors.Num<UINT32>(); ActorIndex++)
			{
				PActor* Actor = Actors[ActorIndex];
				PStaticMeshComponent* MeshComponent = Actor ? Actor->FindComponent<PStaticMeshComponent>() : nullptr;
				if (!MeshComponent)
				{
					continue;
				}
				OutResult.MeshNum++;
				if (MeshComponent->GetMeshAssetBase())
				{
					OutResult.MeshBoundNum++;
				}
				else
				{
					const EString& MeshName = MeshComponent->GetMeshAssetName();
					if (MeshName.Length() > 0u)
					{
						OutResult.MissingMeshNames.Add(MeshName);
					}
				}
			}
		}

		static void WriteReport()
		{
			EString Report;
			Report += EString(PE_TEXT("{\n  \"Levels\": [\n"));
			for (UINT32 LevelIndex = 0u; LevelIndex < GCookLevelResults.Num<UINT32>(); LevelIndex++)
			{
				const FCookLevelResult& Result = GCookLevelResults[LevelIndex];
				Report += EString(PE_TEXT("    {\"Path\": \"")) + Result.Path
					+ EString(PE_TEXT("\", \"Actors\": ")) + ToString(Result.ActorNum)
					+ EString(PE_TEXT(", \"Meshes\": ")) + ToString(Result.MeshNum)
					+ EString(PE_TEXT(", \"MeshBound\": ")) + ToString(Result.MeshBoundNum)
					+ EString(PE_TEXT(", \"MissingMeshes\": ["));
				for (UINT32 MissingIndex = 0u; MissingIndex < Result.MissingMeshNames.Num<UINT32>(); MissingIndex++)
				{
					Report += ((MissingIndex > 0u) ? EString(PE_TEXT(", ")) : EString());
					Report += EString(PE_TEXT("\"")) + Result.MissingMeshNames[MissingIndex] + EString(PE_TEXT("\""));
				}
				Report += EString(PE_TEXT("]}"));
				Report += ((LevelIndex + 1u < GCookLevelResults.Num<UINT32>()) ? EString(PE_TEXT(",\n")) : EString(PE_TEXT("\n")));
			}
			Report += EString(PE_TEXT("  ],\n  \"ImportedMeshes\": ")) + ToString(GCookRequestedMeshNum)
				+ EString(PE_TEXT(",\n  \"ImportedTextures\": ")) + ToString(GCookRequestedTextureNum)
				+ EString(PE_TEXT("\n}\n"));

			const EString ReportPath = EString(EBaseSettings::ENGINE_BUILD_DIRECTORY) + EString(PE_TEXT("CookedContentReport.json"));
			if (EFileHelper::SaveStringToFile(ReportPath, Report))
			{
				PE_LOG_LOG(EString(PE_TEXT("[ContentCook] Report written: ")) + ReportPath);
			}
			else
			{
				PE_LOG_ERROR(EString(PE_TEXT("[ContentCook] Could not write ")) + ReportPath);
			}
		}
	}

	namespace EContentCook
	{
		BOOL32 Tick(PWorld& InWorld)
		{
			if (IsCookRequested() == FALSE)
			{
				return FALSE;
			}
			switch (GCookPhase)
			{
			case ECookPhase::IDLE:
				GCookLevelPaths.Empty();
				GCookLevelResults.Empty();
				GCookLevelIndex = 0u;
				GCookWaitFrames = 0u;
				GCookIdleFrames = 0u;
				GCookChildrenRequested = FALSE;
				GCookSecondPassRequested = FALSE;
				GatherLevelPaths();
				RequestRegistryAssets();
				GCookPhase = ECookPhase::WAIT_IMPORT;
				return TRUE;
			case ECookPhase::WAIT_IMPORT:
				GCookWaitFrames++;
				if (GCookWaitFrames > 6000u)
				{
					PE_LOG_WARN(EString(PE_TEXT("[ContentCook] Import wait timed out; continuing with what is on disk.")));
					GCookPhase = ECookPhase::VALIDATE_LEVELS;
					return TRUE;
				}
				if (IsLoadQueueIdle())
				{
					GCookIdleFrames++;
					if (GCookIdleFrames >= 2u)
					{
						if (GCookSecondPassRequested == FALSE)
						{
							// A level loaded at startup may have queued the same key without a
							// source before the registry was consulted; the first pass then deduped.
							// Once the queue is empty the key is released, so ask once more with the
							// registry source before validating.
							RequestRegistryAssets();
							GCookSecondPassRequested = TRUE;
							GCookWaitFrames = 0u;
							GCookIdleFrames = 0u;
						}
						else if (GCookChildrenRequested == FALSE)
						{
							RequestSceneChildren();
							GCookChildrenRequested = TRUE;
							GCookWaitFrames = 0u;
							GCookIdleFrames = 0u;
						}
						else
						{
							GCookPhase = ECookPhase::VALIDATE_LEVELS;
						}
					}
				}
				else
				{
					GCookIdleFrames = 0u;
				}
				return TRUE;
			case ECookPhase::VALIDATE_LEVELS:
				if (GCookLevelIndex >= GCookLevelPaths.Num<UINT32>())
				{
					GCookPhase = ECookPhase::REPORT;
					return TRUE;
				}
				InWorld.ClearLevelActors();
				if (!InWorld.LoadLevel(GCookLevelPaths[GCookLevelIndex]))
				{
					PE_LOG_WARN(EString(PE_TEXT("[ContentCook] Level load failed: ")) + GCookLevelPaths[GCookLevelIndex]);
					GCookLevelIndex++;
					return TRUE;
				}
				GCookWaitFrames = 0u;
				GCookPhase = ECookPhase::WAIT_LEVEL;
				return TRUE;
			case ECookPhase::WAIT_LEVEL:
				GCookWaitFrames++;
				if (GCookWaitFrames < 240u)
				{
					return TRUE;
				}
				{
					FCookLevelResult Result;
					Result.Path = GCookLevelPaths[GCookLevelIndex];
					InspectLoadedLevel(InWorld, Result);
					PE_LOG_LOG(EString(PE_TEXT("[ContentCook] ")) + Result.Path
						+ EString(PE_TEXT(": actors=")) + ToString(Result.ActorNum)
						+ EString(PE_TEXT(", meshes=")) + ToString(Result.MeshNum)
						+ EString(PE_TEXT(", bound=")) + ToString(Result.MeshBoundNum)
						+ EString(PE_TEXT(", missing=")) + ToString(Result.MissingMeshNames.Num<UINT32>()));
					GCookLevelResults.Add(Result);
				}
				GCookLevelIndex++;
				GCookPhase = ECookPhase::VALIDATE_LEVELS;
				return TRUE;
			case ECookPhase::REPORT:
				WriteReport();
				GCookPhase = ECookPhase::DONE;
				::PostQuitMessage(0);
				return TRUE;
			case ECookPhase::DONE:
			default:
				return TRUE;
			}
		}
	}
}

#endif	// ENGINE_EDITOR_ONLY
