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
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAnimStateMachineAsset();
	public:
		virtual BOOL32 InitResource()override;
	public:
		EAnimStateMachineAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EAnimStateMachineAsset)
	};

	class EAnimStateMachineManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EAnimStateMachineAsset>	EAnimStateMachineAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
#if _EDITOR_ONLY
		/*
		* Imports a JSON state machine source into a PAsset. The JSON is validated before it
		* is written; a file that fails to parse or references unknown states/parameters does
		* not replace an existing asset.
		*/
		BOOL32	ImportStateMachine(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimStateMachineAsset*& OutAsset);
		BOOL32	RequireLoadStateMachineAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimStateMachineAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		/*
		* Editor: parses InJsonText into the asset's existing resource. The asset object stays
		* the same so editor windows and runtime users do not hold a dangling pointer.
		*/
		BOOL32	RebuildStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText, EString& OutError);
		// Rebuilds and writes the PAsset back to the path it was loaded from.
		BOOL32	SaveStateMachineAsset(const EAnimStateMachineAsset* InAsset, const EString& InJsonText, EString& OutError);
	private:
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
