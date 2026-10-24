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
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAnimMontageAsset();
	public:
		virtual BOOL32 InitResource()override;
	public:
		EAnimMontageAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EAnimMontageAsset)
	};

	class EAnimMontageManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EAnimMontageAsset>	EAnimMontageAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
#if _EDITOR_ONLY
		BOOL32	ImportMontage(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadMontageAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimMontageAsset*& OutAsset);
		BOOL32	RequireLoadMontageAsset(const EString& InLoadPath, const EString& InLoadName
			, const EAnimMontageAsset*& OutAsset, EAssetLoadCompleteType InOnLoadComplete = nullptr);
		// Editor: parse into a temporary graph first, then replace the live one on success.
		BOOL32	RebuildMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError);
		BOOL32	SaveMontageAsset(const EAnimMontageAsset* InAsset, const EString& InJsonText, EString& OutError);
	private:
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
