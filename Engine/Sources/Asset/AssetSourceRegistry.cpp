#include "AssetSourceRegistry.h"

#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <Development/EditorLogManager.h>
#include <IO/FileHelper.h>
#include <MultiThread/Mutex.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	namespace
	{
		TArray<FAssetSourceEntry>	GAssetSourceEntries;
		BOOL32						GAssetSourceRegistryLoaded = FALSE;
		EMutex						GAssetSourceRegistryMutex;

		static EString MakeTrailingSlashPath(const EString& InPath)
		{
			if ((InPath.Length() == 0u) || (InPath.Right(1u) == EString(PE_TEXT("/"))))
			{
				return InPath;
			}
			return (InPath + EString(PE_TEXT("/")));
		}

		static BOOL32 ParseFamily(const EString& InFamily, EAssetSourceFamilyType& OutFamily)
		{
			if (InFamily == EString(PE_TEXT("Mesh")))				{ OutFamily = EAssetSourceFamilyType::MESH; return TRUE; }
			if (InFamily == EString(PE_TEXT("SkinnedMesh")))		{ OutFamily = EAssetSourceFamilyType::SKINNED_MESH; return TRUE; }
			if (InFamily == EString(PE_TEXT("Texture2D")))			{ OutFamily = EAssetSourceFamilyType::TEXTURE_2D; return TRUE; }
			if (InFamily == EString(PE_TEXT("TextureCube")))		{ OutFamily = EAssetSourceFamilyType::TEXTURE_CUBE; return TRUE; }
			if (InFamily == EString(PE_TEXT("Skeleton")))			{ OutFamily = EAssetSourceFamilyType::SKELETON; return TRUE; }
			if (InFamily == EString(PE_TEXT("Animation")))			{ OutFamily = EAssetSourceFamilyType::ANIMATION; return TRUE; }
			if (InFamily == EString(PE_TEXT("StateMachine")))		{ OutFamily = EAssetSourceFamilyType::STATE_MACHINE; return TRUE; }
			return FALSE;
		}

		static BOOL32 IsSceneChildName(const EString& InAssetName, const EString& InPrefix)
		{
			if ((InPrefix.Length() == 0u) || (InAssetName.Length() < InPrefix.Length()))
			{
				return FALSE;
			}
			if (InAssetName.Left(InPrefix.Length()) != InPrefix)
			{
				return FALSE;
			}
			// <Prefix> or <Prefix>_<suffix>; a longer unrelated name is not a child.
			return ((InAssetName.Length() == InPrefix.Length())
				|| (InAssetName[InPrefix.Length()] == EString(PE_TEXT("_"))[0u])) ? TRUE : FALSE;
		}
	}

	namespace EAssetSourceRegistry
	{
		void Clear()
		{
			EScopedLock ScopedLock(GAssetSourceRegistryMutex);
			GAssetSourceEntries.Empty();
			GAssetSourceRegistryLoaded = FALSE;
		}

		void Initialize()
		{
			EScopedLock ScopedLock(GAssetSourceRegistryMutex);
			if (GAssetSourceRegistryLoaded != FALSE)
			{
				return;
			}
			GAssetSourceRegistryLoaded = TRUE;

			const EString RegistryPath(EBaseSettings::ENGINE_ASSET_SOURCE_REGISTRY);
			EString RegistryText;
			if (!EFileHelper::ReadFileAsString(RegistryPath, RegistryText))
			{
				PE_LOG_LOG(EString(PE_TEXT("[AssetSourceRegistry] No registry at '")) + RegistryPath
					+ EString(PE_TEXT("'; level load will only use assets that are already imported.")));
				return;
			}
			CJsonObject RootObject(RegistryText);
			if (!RootObject.IsValid())
			{
				PE_LOG_ERROR(EString(PE_TEXT("[AssetSourceRegistry] Invalid JSON: ")) + RegistryPath);
				return;
			}
			TArray<CJsonObject*> SourceObjects;
			if (!RootObject.GetObjectArrayField(EString(PE_TEXT("Sources")), SourceObjects))
			{
				PE_LOG_ERROR(EString(PE_TEXT("[AssetSourceRegistry] The registry carries no Sources array: ")) + RegistryPath);
				return;
			}
			for (UINT32 SourceIndex = 0u; SourceIndex < SourceObjects.Num<UINT32>(); SourceIndex++)
			{
				CJsonObject* SourceObject = SourceObjects[SourceIndex];
				if (!SourceObject)
				{
					continue;
				}
				EString FamilyName;
				EString AssetName;
				EString SourcePath;
				EString SourceName;
				EString SourceType;
				if (!SourceObject->GetStringField(EString(PE_TEXT("Family")), FamilyName)
					|| !SourceObject->GetStringField(EString(PE_TEXT("Asset")), AssetName)
					|| !SourceObject->GetStringField(EString(PE_TEXT("Path")), SourcePath))
				{
					PE_LOG_WARN(EString(PE_TEXT("[AssetSourceRegistry] Skipping an entry without Family/Asset/Path.")));
					continue;
				}
				EAssetSourceFamilyType Family = EAssetSourceFamilyType::MESH;
				if (!ParseFamily(FamilyName, Family))
				{
					PE_LOG_WARN(EString(PE_TEXT("[AssetSourceRegistry] Unknown family '")) + FamilyName + EString(PE_TEXT("'.")));
					continue;
				}

				FAssetSourceEntry Entry;
				Entry.Family = Family;
				Entry.AssetName = AssetName;
				if (Family == EAssetSourceFamilyType::TEXTURE_CUBE)
				{
					// A cube imports six faces in one request; the entry names them in face order
					// (+X, -X, +Y, -Y, +Z, -Z) and carries one shared source folder/type.
					TArray<EString> FaceNames;
					if ((!SourceObject->GetStringArrayField(EString(PE_TEXT("Names")), FaceNames))
						|| (FaceNames.Num<UINT32>() != 6u)
						|| (!SourceObject->GetStringField(EString(PE_TEXT("Type")), SourceType)))
					{
						PE_LOG_WARN(EString(PE_TEXT("[AssetSourceRegistry] A TextureCube entry needs six Names and a Type.")));
						continue;
					}
					const EString CubePath = EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + MakeTrailingSlashPath(SourcePath);
					for (UINT32 FaceIndex = 0u; FaceIndex < 6u; FaceIndex++)
					{
						Entry.CubeSource.ImportPaths.Add(CubePath);
						Entry.CubeSource.ImportNames.Add(FaceNames[FaceIndex]);
						Entry.CubeSource.ImportFileTypes.Add(SourceType);
					}
					BOOL8 bFallbackSRGB = FALSE;
					SourceObject->GetBoolField(EString(PE_TEXT("FallbackSRGB")), bFallbackSRGB);
					Entry.CubeSource.bFallbackSRGB = (bFallbackSRGB != FALSE) ? TRUE : FALSE;
					GAssetSourceEntries.Add(Entry);
					continue;
				}
				if (!SourceObject->GetStringField(EString(PE_TEXT("Name")), SourceName)
					|| !SourceObject->GetStringField(EString(PE_TEXT("Type")), SourceType))
				{
					PE_LOG_WARN(EString(PE_TEXT("[AssetSourceRegistry] Skipping an entry without Name/Type.")));
					continue;
				}
				Entry.ImportSource.ImportPath = EString(EBaseSettings::ENGINE_ASSET_DIRECTORY) + MakeTrailingSlashPath(SourcePath);
				Entry.ImportSource.ImportName = SourceName;
				Entry.ImportSource.ImportFileType = SourceType;
				BOOL8 bCombineSubmeshes = TRUE;
				BOOL8 bOptimizeMeshes = TRUE;
				BOOL8 bFallbackSRGB = FALSE;
				SourceObject->GetBoolField(EString(PE_TEXT("CombineSubmeshes")), bCombineSubmeshes);
				SourceObject->GetBoolField(EString(PE_TEXT("OptimizeMeshes")), bOptimizeMeshes);
				SourceObject->GetBoolField(EString(PE_TEXT("FallbackSRGB")), bFallbackSRGB);
				Entry.ImportSource.bIsCombineSubmeshes = (bCombineSubmeshes != FALSE) ? TRUE : FALSE;
				Entry.ImportSource.bIsOptimizeMeshes = (bOptimizeMeshes != FALSE) ? TRUE : FALSE;
				Entry.ImportSource.bFallbackSRGB = (bFallbackSRGB != FALSE) ? TRUE : FALSE;
				SourceObject->GetStringField(EString(PE_TEXT("OutputPrefix")), Entry.OutputPrefix);
				GAssetSourceEntries.Add(Entry);
			}
			PE_LOG_LOG(EString(PE_TEXT("[AssetSourceRegistry] Loaded ")) + ToString(GAssetSourceEntries.Num<UINT32>())
				+ EString(PE_TEXT(" source entries from ")) + RegistryPath);
		}

		BOOL32 IsInitialized()
		{
			EScopedLock ScopedLock(GAssetSourceRegistryMutex);
			return GAssetSourceRegistryLoaded;
		}

		const TArray<FAssetSourceEntry>& GetEntries()
		{
			Initialize();
			return GAssetSourceEntries;
		}

		BOOL32 Resolve(EAssetSourceFamilyType InFamily, const EString& InAssetName, FAssetImportSource& OutSource)
		{
			Initialize();
			for (UINT32 Index = 0u, Num = GAssetSourceEntries.Num<UINT32>(); Index < Num; Index++)
			{
				const FAssetSourceEntry& Entry = GAssetSourceEntries[Index];
				if ((Entry.Family == InFamily) && (Entry.AssetName == InAssetName))
				{
					OutSource = Entry.ImportSource;
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 ResolveMesh(const EString& InAssetName, FAssetImportSource& OutSource)
		{
			return Resolve(EAssetSourceFamilyType::MESH, InAssetName, OutSource);
		}

		BOOL32 ResolveTexture2D(const EString& InAssetName, FAssetImportSource& OutSource)
		{
			return Resolve(EAssetSourceFamilyType::TEXTURE_2D, InAssetName, OutSource);
		}

		BOOL32 ResolveTextureCube(const EString& InAssetName, FAssetImportCubeSource& OutSource)
		{
			Initialize();
			for (UINT32 Index = 0u, Num = GAssetSourceEntries.Num<UINT32>(); Index < Num; Index++)
			{
				const FAssetSourceEntry& Entry = GAssetSourceEntries[Index];
				if ((Entry.Family == EAssetSourceFamilyType::TEXTURE_CUBE) && (Entry.AssetName == InAssetName))
				{
					OutSource = Entry.CubeSource;
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 ResolveMeshSceneRoot(const EString& InAssetName, EString& OutRootName, FAssetImportSource& OutRootSource)
		{
			Initialize();
			const FAssetSourceEntry* BestEntry = nullptr;
			for (UINT32 Index = 0u, Num = GAssetSourceEntries.Num<UINT32>(); Index < Num; Index++)
			{
				const FAssetSourceEntry& Entry = GAssetSourceEntries[Index];
				if ((Entry.Family != EAssetSourceFamilyType::MESH) || (Entry.OutputPrefix.Length() == 0u))
				{
					continue;
				}
				if (!IsSceneChildName(InAssetName, Entry.OutputPrefix))
				{
					continue;
				}
				// A more specific root wins when one asset family sits inside another prefix.
				if ((!BestEntry) || (Entry.OutputPrefix.Length() > BestEntry->OutputPrefix.Length()))
				{
					BestEntry = &Entry;
				}
			}
			if (!BestEntry)
			{
				return FALSE;
			}
			OutRootName = BestEntry->AssetName;
			OutRootSource = BestEntry->ImportSource;
			return TRUE;
		}
	}

}	// namespace PigeonEngine
