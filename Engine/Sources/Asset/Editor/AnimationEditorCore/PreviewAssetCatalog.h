#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>

namespace PigeonEngine
{

	class ESkinnedMeshAsset;
	class ESkeletonAsset;

	/*
	* Shared discovery/loading for preview meshes and skeletons. Asset manager caches keep
	* the returned pointers alive; the catalog itself only stores paths.
	*/
	class FPreviewAssetCatalog
	{
	public:
		void Refresh(const EString& InPreferredSkeleton);
		PE_NODISCARD const TArray<EString>& GetMeshPaths()const;
		PE_NODISCARD const TArray<EString>& GetSkeletonPaths()const;
		PE_NODISCARD const ESkinnedMeshAsset* LoadMesh(INT32 InIndex)const;
		PE_NODISCARD const ESkeletonAsset* LoadSkeleton(INT32 InIndex)const;
		PE_NODISCARD static INT32 FindSkeletonIndex(const TArray<EString>& InPaths, const EString& InSkeletonName);
	private:
		TArray<EString> MeshPaths;
		TArray<EString> SkeletonPaths;
	};

}

#endif
