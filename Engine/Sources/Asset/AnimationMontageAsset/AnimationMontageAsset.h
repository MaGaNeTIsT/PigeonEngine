#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <Animation/AnimationMontage.h>

namespace PigeonEngine
{

	class EAnimMontageAsset : public TBaseAsset<EAnimMontage>
	{
	public:
		EAnimMontageAsset(const EString& InAssetPath, const EString& InAssetName
#if ENGINE_EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAnimMontageAsset();
	public:
		virtual BOOL32 InitResource() override;
	public:
		EAnimMontageAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EAnimMontageAsset)
	};

	class EAnimMontageManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EAnimMontageAsset>	EAnimMontageAssetManager;
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
#if ENGINE_EDITOR_ONLY
		BOOL32	ImportMontage(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath);
		// Editor: creates a brand-new montage from in-memory JSON. Refuses to overwrite an
		// existing PAsset and publishes the validated asset into the cache on success.
		BOOL32	CreateMontageAsset(const EString& InAssetName, const EString& InJsonText
			, const EString& InSavePath, EString& OutError);
#endif
		BOOL32	LoadMontageAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimMontageAsset*& OutAsset);
		/*
		* Editor: loads a fresh montage from disk, swaps the cache entry and keeps the old
		* object alive for running instances. OutAsset receives the new object.
		*/
		BOOL32	ReloadMontageAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimMontageAsset*& OutAsset, EString& OutError);
		BOOL32	RequireLoadMontageAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimMontageAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		// Editor: parse into a temporary graph first, then replace the live one on success.
		BOOL32	RebuildMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError);
		// Parses JSON into a standalone montage owned by the caller; used by the editor draft.
		EAnimMontage* BuildTransientMontage(const EString& InJsonText, EString& OutError);
		BOOL32	ApplyMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText
			, const EAnimMontageAsset*& OutAsset, EString& OutError);
		BOOL32	SaveMontageAssetCopyOnWrite(const EAnimMontageAsset* InAsset, const EString& InJsonText
			, const EAnimMontageAsset*& OutAsset, EString& OutError);
		BOOL32	SaveMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError);
		// Reference index: every montage asset currently resident in the cache.
		void	GetLoadedAssets(TArray<const EAnimMontageAsset*>& OutAssets);
		// Re-applies standalone notify definitions to loaded montage events in place,
		// preserving event addresses and any active state windows.
		void	RefreshNotifyReferences();
	private:
		BOOL32	BuildAndPublishMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText
			, BOOL32 bInSaveToDisk, const EAnimMontageAsset*& OutAsset, EString& OutError);
		void	ClearMontages();
		void	ReleaseAllAssets();
	private:
		EAnimMontageAsset*	LoadMontageAsset(const EString& InLoadPath, const EString& InLoadName);
		EAnimMontage*		LoadMontageResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError);
		BOOL32				SaveMontageResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText);
		BOOL32				BuildMontage(EAnimMontage& OutMontage, const EString& InJsonText, EString& OutError);
	private:
		EAnimMontageAssetManager	MontageAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAnimMontageManager)
	};

}
