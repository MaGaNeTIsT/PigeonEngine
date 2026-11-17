#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <EngineCommon.h>
#include <Navigation/NavigationTypes.h>
#include <RecastNavigation/Headers/RecastNavigationProvider.h>

namespace PigeonEngine
{

	/*
	* Baked navigation mesh as stored on disk.
	*
	* The blob is the backend's own container and is never interpreted here: ProviderName records
	* which backend wrote it, and only that backend may read the bytes. The metadata next to the
	* blob exists so the editor and the log can say what a bake was made with without asking the
	* backend to parse anything.
	*
	* The bake settings are the Recast backend's, because a navigation mesh carries the voxel and
	* agent dimensions it was built with. A second backend would store its own settings behind a
	* new FormatVersion; the provider name is what guards the pair.
	*/
	struct FNavMeshAssetSettings
	{
	public:
		UINT32					FormatVersion = 0u;
		EString					ProviderName;
		ENavMeshBuildSettings	BuildSettings;
		EBoundAABB				Bounds = EBoundAABB(Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), FALSE);
		UINT32					PolygonCount = 0u;
		UINT32					TileCount = 0u;
	};

	// Cold data of a navigation mesh asset. No GPU resource: navigation lives entirely on the CPU.
	class ENavMeshData : public EObjectBase, public EResourceInterface
	{
	public:
		ENavMeshData();
		virtual ~ENavMeshData();

	public:
		virtual BOOL32	IsResourceValid() const override;
		virtual BOOL32	InitResource() override;
		virtual void	ReleaseResource() override;

	public:
		PE_NODISCARD const FNavMeshAssetSettings&	GetSettings() const;
		PE_NODISCARD const TArray<UINT8>&			GetBlob() const;
		// Filled by the manager while a file is loaded. Takes the bytes so a large mesh is not
		// copied a second time.
		void	SetContents(const FNavMeshAssetSettings& InSettings, TArray<UINT8>&& InBlob);

	private:
		FNavMeshAssetSettings	Settings;
		TArray<UINT8>			Blob;
	};

	class ENavMeshAsset : public TBaseAsset<ENavMeshData>
	{
	public:
		ENavMeshAsset(const EString& InAssetPath, const EString& InAssetName
#if ENGINE_EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~ENavMeshAsset();

	public:
		virtual BOOL32	InitResource() override;

	public:
		ENavMeshAsset() = delete;

		CLASS_REMOVE_COPY_BODY(ENavMeshAsset)
	};

	/*
	* Cache and file IO for navigation mesh assets.
	*
	* Loading is synchronous through LoadNavMeshAsset and queued through RequireLoadNavMeshAsset,
	* the same split the other families use. Saving goes through CreateNavMeshAsset, which refuses
	* to overwrite an existing file so a bake never silently replaces an authored one.
	*/
	class ENavMeshAssetManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, ENavMeshAsset>	ENavMeshAssetCache;

	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;

	public:
		// Cache lookup only; never touches the disk. Polling it every frame is cheap.
		PE_NODISCARD const ENavMeshAsset*	FindNavMeshAsset(const EString& InLoadPath, const EString& InLoadName);
		BOOL32	LoadNavMeshAsset(const EString& InLoadPath, const EString& InLoadName, const ENavMeshAsset*& OutAsset);
		BOOL32	RequireLoadNavMeshAsset(const EString& InLoadPath, const EString& InLoadName
			, const ENavMeshAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		// Writes a new asset and publishes it. Refuses to overwrite, so the caller has to delete or
		// rename first; that is what keeps an accidental bake from destroying the shipped mesh.
		BOOL32	CreateNavMeshAsset(const EString& InAssetName, const FNavMeshAssetSettings& InSettings
			, const TArray<UINT8>& InBlob, const EString& InSavePath
			, const ENavMeshAsset*& OutAsset, EString& OutError);
		void	ClearNavMeshAssets();

	private:
		ENavMeshData*	LoadNavMeshResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError) const;
		BOOL32			SaveNavMeshResource(const EString& InSavePath, const EString& InSaveName
			, const FNavMeshAssetSettings& InSettings, const TArray<UINT8>& InBlob) const;

	private:
		ENavMeshAssetCache	NavMeshAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(ENavMeshAssetManager)
	};

}  // namespace PigeonEngine
