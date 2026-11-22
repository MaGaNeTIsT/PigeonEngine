#pragma once

#include <CoreMinimal.h>
#include <Asset/AssetLoadManager.h>

namespace PigeonEngine
{

	/*
	* Project level asset source registry.
	*
	* A level stores asset names, not source files. A fresh checkout therefore has the level and
	* the raw FBX/PNG sources, but not the imported PAssets under Build/Temp. Other engines solve
	* this by letting the editor rebuild its import cache from the sources; this registry is the
	* project side of that contract: one entry per output asset (or one scene root whose import
	* produces a family of assets), with the import options the importer needs.
	*
	* It is content, not code: Assets/AssetSources.json is committed, and the engine only reads it.
	* The runtime never imports; an editor build consults the registry from the family RequireLoad
	* entry points when the caller did not supply an import source of its own.
	*/
	enum class EAssetSourceFamilyType : UINT8
	{
		MESH = 0,
		SKINNED_MESH,
		TEXTURE_2D,
		TEXTURE_CUBE,
		SKELETON,
		ANIMATION,
		STATE_MACHINE,
		COUNT,
	};

	struct FAssetSourceEntry
	{
	public:
		EAssetSourceFamilyType	Family = EAssetSourceFamilyType::MESH;
		// Output asset name the registry answers for.
		EString					AssetName;
		// A scene root's import writes <OutputPrefix>, <OutputPrefix>_1, ... and a
		// <OutputPrefix>.Scene.json manifest. Empty for a single-asset entry. A request for a
		// missing asset that starts with this prefix imports the root first.
		EString					OutputPrefix;
		// Import source, relative to ENGINE_ASSET_DIRECTORY, plus the importer options.
		FAssetImportSource		ImportSource;
		// Cube textures import six faces in one call; their entry carries the face list here.
		FAssetImportCubeSource	CubeSource;
	};

	namespace EAssetSourceRegistry
	{
		// Reads ENGINE_ASSET_SOURCE_REGISTRY once. Safe to call from any thread; the first caller
		// loads the file, later callers see the result.
		void	Initialize();
		void	Clear();
		PE_NODISCARD BOOL32 IsInitialized();
		PE_NODISCARD const TArray<FAssetSourceEntry>& GetEntries();

		// Exact output-name lookup for one family.
		PE_NODISCARD BOOL32 Resolve(EAssetSourceFamilyType InFamily, const EString& InAssetName, FAssetImportSource& OutSource);
		PE_NODISCARD BOOL32 ResolveMesh(const EString& InAssetName, FAssetImportSource& OutSource);
		PE_NODISCARD BOOL32 ResolveTexture2D(const EString& InAssetName, FAssetImportSource& OutSource);
		PE_NODISCARD BOOL32 ResolveTextureCube(const EString& InAssetName, FAssetImportCubeSource& OutSource);
		/*
		* The scene-root fallback: InAssetName names a missing child (<Prefix>, <Prefix>_N, ...)
		* and a root entry with that OutputPrefix exists. OutRootName receives the root's asset
		* name and OutRootSource its import source, so the caller imports the root and then loads
		* the child.
		*/
		PE_NODISCARD BOOL32 ResolveMeshSceneRoot(const EString& InAssetName, EString& OutRootName, FAssetImportSource& OutRootSource);
	}

}	// namespace PigeonEngine
