#include "PreviewAssetCatalog.h"

#if _EDITOR_ONLY

#include <IO/FileHelper.h>
#include <MeshAsset/MeshAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>

namespace PigeonEngine
{

	namespace
	{
		EString NormalizeDirectory(const EString& InPath)
		{
			EString Result = InPath.Replace("\\", "/");
			if ((Result.Length() > 0u) && (Result[Result.LastIndex()] != '/'))
			{
				Result += "/";
			}
			return Result;
		}

		BOOL32 ReadAssetType(const EString& InPath, EAssetType& OutType)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InPath, HeaderData, sizeof(UINT32))) || (!HeaderData))
			{
				return FALSE;
			}
			OutType = static_cast<EAssetType>(*static_cast<const UINT32*>(HeaderData));
			delete[] static_cast<CHAR*>(HeaderData);
			return TRUE;
		}

		BOOL32 IsSkinnedMesh(const EString& InPath)
		{
			void* HeaderData = nullptr;
			if (!(EFileHelper::ReadFirstNumberOfBytesInFile(InPath, HeaderData
				, sizeof(UINT32) * 2u)) || (!HeaderData))
			{
				return FALSE;
			}
			const UINT32* HeaderWords = static_cast<const UINT32*>(HeaderData);
			const BOOL32 bResult = ((HeaderWords[0] == EAssetType::ASSET_TYPE_MESH)
				&& (HeaderWords[1] == EMeshType::MESH_TYPE_SKIN)) ? TRUE : FALSE;
			delete[] static_cast<CHAR*>(HeaderData);
			return bResult;
		}
	}

	void FPreviewAssetCatalog::Refresh(const EString& InPreferredSkeleton)
	{
		MeshPaths.Empty();
		SkeletonPaths.Empty();
		TArray<EString> Folders;
		TArray<EString> Files;
		EFileHelper::ScanDirectory(NormalizeDirectory(EBaseSettings::ENGINE_MESH_PATH)
			, Folders, Files, TRUE);
		for (INT32 FileIndex = 0; FileIndex < Files.Num(); FileIndex++)
		{
			const EString Path = Files[FileIndex].Replace("\\", "/");
			if ((Path.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE)) && (IsSkinnedMesh(Path)))
			{
				MeshPaths.Add(Path);
			}
		}
		Files.Empty();
		EFileHelper::ScanDirectory(NormalizeDirectory(EBaseSettings::ENGINE_SKELETON_PATH)
			, Folders, Files, TRUE);
		for (INT32 FileIndex = 0; FileIndex < Files.Num(); FileIndex++)
		{
			const EString Path = Files[FileIndex].Replace("\\", "/");
			if (!Path.Contains(EEngineSettings::ENGINE_ASSET_NAME_TYPE))
			{
				continue;
			}
			EAssetType AssetType = EAssetType::ASSET_TYPE_UNKNOWN;
			if ((ReadAssetType(Path, AssetType)) && (AssetType == EAssetType::ASSET_TYPE_SKELETON))
			{
				SkeletonPaths.Add(Path);
			}
		}
	}

	const TArray<EString>& FPreviewAssetCatalog::GetMeshPaths()const
	{
		return MeshPaths;
	}

	const TArray<EString>& FPreviewAssetCatalog::GetSkeletonPaths()const
	{
		return SkeletonPaths;
	}

	const ESkinnedMeshAsset* FPreviewAssetCatalog::LoadMesh(INT32 InIndex)const
	{
		if ((InIndex < 0) || (InIndex >= MeshPaths.Num()))
		{
			return nullptr;
		}
		const EString Name = EPath::GetFileNameWithoutExtension(MeshPaths[InIndex]);
		const EString Directory = NormalizeDirectory(EPath::GetFileFolderPath(MeshPaths[InIndex]));
		const ESkinnedMeshAsset* Asset = nullptr;
		EMeshAssetManager::GetManagerSingleton()->LoadSkinnedMeshAsset(Directory, Name, Asset);
		return Asset;
	}

	const ESkeletonAsset* FPreviewAssetCatalog::LoadSkeleton(INT32 InIndex)const
	{
		if ((InIndex < 0) || (InIndex >= SkeletonPaths.Num()))
		{
			return nullptr;
		}
		const EString Name = EPath::GetFileNameWithoutExtension(SkeletonPaths[InIndex]);
		const EString Directory = NormalizeDirectory(EPath::GetFileFolderPath(SkeletonPaths[InIndex]));
		const ESkeletonAsset* Asset = nullptr;
		ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(Directory, Name, Asset);
		return Asset;
	}

	INT32 FPreviewAssetCatalog::FindSkeletonIndex(const TArray<EString>& InPaths, const EString& InSkeletonName)
	{
		if (InSkeletonName.Length() <= 0u)
		{
			return -1;
		}
		for (INT32 SkeletonIndex = 0; SkeletonIndex < InPaths.Num(); SkeletonIndex++)
		{
			if (EPath::GetFileNameWithoutExtension(InPaths[SkeletonIndex]) == InSkeletonName)
			{
				return SkeletonIndex;
			}
		}
		return -1;
	}

}

#endif
