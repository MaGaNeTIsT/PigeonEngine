#pragma once

#include <CoreMinimal.h>
#include <BaseAsset.h>
#include <AssetLoadManager.h>
#include <Animation/AnimationNotify.h>

namespace PigeonEngine
{

	/*
	* A reusable notify definition stored in its own PAsset. Clips and montage sections
	* reference it by name; the definition owns the compiled notify object so every
	* referencing event shares one stateless instance.
	*/
	class EAnimNotifyDefinition : public EObjectBase, public EResourceInterface
	{
		CLASS_VIRTUAL_NOCOPY_BODY(EAnimNotifyDefinition)

	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;

	public:
		EString					Name;
		EString					Description;
		EString					NotifyClassName;
		BOOL32					bIsState = FALSE;
		FLOAT					DefaultDuration = 0.0f;
		EString					PayloadJson;
		EString					SourceJson;
		// Shared by every event that references this definition. Notify objects are
		// stateless, so one compiled instance is enough.
		TSharedPtr<EAnimNotify>	CachedNotify;

	public:
		void	Reset();
		BOOL32	IsValid()const;
	};

	/*
	* Copies a definition onto one event in place. The event keeps its time and any
	* explicitly authored duration/payload overrides; existing pointers to the event stay
	* valid, which is what makes runtime refresh safe.
	*/
	void ApplyNotifyDefinitionToEvent(FAnimNotifyEvent& InOutEvent, const EAnimNotifyDefinition& InDefinition);

	class EAnimNotifyAsset : public TBaseAsset<EAnimNotifyDefinition>
	{
	public:
		EAnimNotifyAsset(const EString& InAssetPath, const EString& InAssetName
#if _EDITOR_ONLY
			, const EString& InDebugName
#endif
		);
		virtual ~EAnimNotifyAsset();
	public:
		virtual BOOL32 InitResource()override;
	public:
		EAnimNotifyAsset() = delete;

		CLASS_REMOVE_COPY_BODY(EAnimNotifyAsset)
	};

	class EAnimNotifyManager : public EManagerBase
	{
	public:
		typedef TAssetManager<EString, EAnimNotifyAsset>	EAnimNotifyAssetManager;
	public:
		virtual void	Initialize()override;
		virtual void	ShutDown()override;
		virtual void	Uninit()override;
	public:
#if _EDITOR_ONLY
		BOOL32	ImportNotify(const EString& InAssetName, const EString& InJsonFullPathName, const EString& InSavePath);
#endif
		BOOL32	LoadNotifyAsset(const EString& InLoadPath, const EString& InLoadName, const EAnimNotifyAsset*& OutAsset);
		// Resolves a clip/montage event reference against the standard notify asset path.
		BOOL32	ResolveNotifyDefinition(const EString& InNotifyName, const EAnimNotifyDefinition*& OutDefinition);
		BOOL32	RebuildNotifyAsset(const EAnimNotifyAsset* InAsset, const EString& InJsonText, EString& OutError);
		BOOL32	SaveNotifyAsset(const EAnimNotifyAsset* InAsset, const EString& InJsonText, EString& OutError);
	private:
		void	ClearNotifies();
		void	ReleaseAllAssets();
	private:
		EAnimNotifyAsset*			LoadNotifyAsset(const EString& InLoadPath, const EString& InLoadName);
		EAnimNotifyDefinition*		LoadNotifyResource(const EString& InLoadPath, const EString& InLoadName, EString& OutError);
		BOOL32						SaveNotifyResource(const EString& InSavePath, const EString& InSaveName, const EString& InJsonText);
		BOOL32						BuildNotifyDefinition(EAnimNotifyDefinition& OutDefinition, const EString& InJsonText, EString& OutError);
	private:
		EAnimNotifyAssetManager	NotifyAssetManager;

		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EAnimNotifyManager)
	};

}
