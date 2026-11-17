#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <Animation/AnimationStateMachine.h>

namespace PigeonEngine
{

	class EAnimStateMachineAsset : public TBaseAsset<EAnimStateMachine>
	{
	public:
		EAnimStateMachineAsset(const EString& InAssetPath, const EString& InAssetName
#if ENGINE_EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAnimStateMachineAsset();
	public:
		virtual BOOL32 InitResource() override;
	public:
		EAnimStateMachineAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EAnimStateMachineAsset)
	};

	class EAnimStateMachineManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EAnimStateMachineAsset>	EAnimStateMachineAssetManager;
	public:
		virtual void	Initialize() override;
		virtual void	ShutDown() override;
		virtual void	Uninit() override;
	public:
#if ENGINE_EDITOR_ONLY
		/*
		* Imports a JSON state machine source into a PAsset. The JSON is validated before it
		* is written; a file that fails to parse or references unknown states/parameters does
		* not replace an existing asset.
		*/
		BOOL32	ImportStateMachine(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath);
		// Editor: creates a brand-new state machine from in-memory JSON. Refuses to overwrite
		// an existing PAsset and publishes the validated asset into the cache on success.
		BOOL32	CreateStateMachineAsset(const EString& InAssetName, const EString& InJsonText
			, const EString& InSavePath, EString& OutError);
#endif
		BOOL32	LoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimStateMachineAsset*& OutAsset);
		/*
		* Editor: loads a fresh state machine from disk, swaps the cache entry and keeps the old
		* object alive for running instances. OutAsset receives the new object.
		*/
		BOOL32	ReloadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimStateMachineAsset*& OutAsset, EString& OutError);
		BOOL32	RequireLoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimStateMachineAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		/*
		* Editor: parses InJsonText into the asset's existing resource. The asset object stays
		* the same so editor windows and runtime users do not hold a dangling pointer.
		*/
		BOOL32	RebuildStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText, EString& OutError);
		// Parses JSON into a standalone graph owned by the caller. Used by the editor draft
		// preview so structured edits can be validated without touching the live asset.
		EAnimStateMachine* BuildTransientStateMachine(const EString& InJsonText, EString& OutError);
		/*
		* Editor: builds a fresh asset and swaps the cache entry, retiring the old object for
		* running instances. Apply does not touch disk; the copy-on-write save variant writes
		* the PAsset only after the new graph has been built successfully.
		*/
		BOOL32	ApplyStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText
			, const EAnimStateMachineAsset*& OutAsset, EString& OutError);
		BOOL32	SaveStateMachineAssetCopyOnWrite(const EAnimStateMachineAsset* InAsset, const EString& InJsonText
			, const EAnimStateMachineAsset*& OutAsset, EString& OutError);
		// Rebuilds and writes the PAsset back to the path it was loaded from.
		BOOL32	SaveStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText, EString& OutError);
		// Reference index: every state machine asset currently resident in the cache.
		void	GetLoadedAssets(TArray<const EAnimStateMachineAsset*>& OutAssets);
	private:
		BOOL32	BuildAndPublishStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText
			, BOOL32 bInSaveToDisk, const EAnimStateMachineAsset*& OutAsset, EString& OutError);
		void	ClearStateMachines();
		void	ReleaseAllAssets();
	private:
		EAnimStateMachineAsset*	LoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName);
		EAnimStateMachine*		LoadStateMachineResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError);
		BOOL32					SaveStateMachineResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText);
		BOOL32					BuildStateMachine(EAnimStateMachine& OutMachine, const EString& InJsonText, EString& OutError);
	private:
		EAnimStateMachineAssetManager	StateMachineAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAnimStateMachineManager)
	};

}
