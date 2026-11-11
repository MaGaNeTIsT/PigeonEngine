#include "AnimationDocumentManager.h"

#if _EDITOR_ONLY

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <AnimationMontageAsset/AnimationMontageAsset.h>
#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <AnimationAsset/AnimationAsset.h>
#include <SkeletonAsset/SkeletonAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include "ClipDocument.h"
#include "MontageDocument.h"
#include "NotifyDocument.h"
#include "SlotGroupDocument.h"
#include "StateMachineDocument.h"

namespace PigeonEngine
{

	FAnimationDocumentManager::~FAnimationDocumentManager()
	{
		CloseAll();
	}

	FStateMachineDocument* FAnimationDocumentManager::OpenStateMachine(const EString& InAssetFullPath
		, EString& OutError)
	{
		OutError = EString();
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		if (FAnimationDocument* Existing = FindByPath(NormalizedPath))
		{
			Active = Existing;
			return static_cast<FStateMachineDocument*>(Existing);
		}

		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			OutError = EString("Cannot parse state machine asset path: ") + NormalizedPath;
			return nullptr;
		}

		const EAnimStateMachineAsset* LoadedAsset = nullptr;
		if (!(EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(
			AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString("Failed to load state machine asset: ") + NormalizedPath;
			return nullptr;
		}

		FStateMachineDocument* Document = new FStateMachineDocument();
		if (!Document->Initialize(NormalizedPath, AssetName, LoadedAsset, OutError))
		{
			delete Document;
			return nullptr;
		}
		Documents.Add(Document);
		Active = Document;
		return Document;
	}

	FClipDocument* FAnimationDocumentManager::OpenClip(const EString& InAssetFullPath, EString& OutError)
	{
		OutError = EString();
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		if (FAnimationDocument* Existing = FindByPath(NormalizedPath))
		{
			Active = Existing;
			return static_cast<FClipDocument*>(Existing);
		}

		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			OutError = EString("Cannot parse animation asset path: ") + NormalizedPath;
			return nullptr;
		}

		const ESkeletonAnimationAsset* LoadedAsset = nullptr;
		if (!(EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString("Failed to load animation asset: ") + NormalizedPath;
			return nullptr;
		}
		FClipDocument* Document = new FClipDocument();
		if (!Document->Initialize(NormalizedPath, AssetName, LoadedAsset, OutError))
		{
			delete Document;
			return nullptr;
		}
		Documents.Add(Document);
		Active = Document;
		return Document;
	}

	FMontageDocument* FAnimationDocumentManager::OpenMontage(const EString& InAssetFullPath
		, EString& OutError)
	{
		OutError = EString();
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		if (FAnimationDocument* Existing = FindByPath(NormalizedPath))
		{
			Active = Existing;
			return static_cast<FMontageDocument*>(Existing);
		}

		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			OutError = EString("Cannot parse montage asset path: ") + NormalizedPath;
			return nullptr;
		}

		const EAnimMontageAsset* LoadedAsset = nullptr;
		if (!(EAnimMontageManager::GetManagerSingleton()->LoadMontageAsset(
			AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString("Failed to load montage asset: ") + NormalizedPath;
			return nullptr;
		}
		FMontageDocument* Document = new FMontageDocument();
		if (!Document->Initialize(NormalizedPath, AssetName, LoadedAsset, OutError))
		{
			delete Document;
			return nullptr;
		}
		Documents.Add(Document);
		Active = Document;
		return Document;
	}

	FNotifyDocument* FAnimationDocumentManager::OpenNotify(const EString& InAssetFullPath
		, EString& OutError)
	{
		OutError = EString();
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		if (FAnimationDocument* Existing = FindByPath(NormalizedPath))
		{
			Active = Existing;
			return static_cast<FNotifyDocument*>(Existing);
		}

		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			OutError = EString("Cannot parse notify asset path: ") + NormalizedPath;
			return nullptr;
		}

		const EAnimNotifyAsset* LoadedAsset = nullptr;
		if (!(EAnimNotifyManager::GetManagerSingleton()->LoadNotifyAsset(
			AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString("Failed to load notify asset: ") + NormalizedPath;
			return nullptr;
		}
		FNotifyDocument* Document = new FNotifyDocument();
		if (!Document->Initialize(NormalizedPath, AssetName, LoadedAsset, OutError))
		{
			delete Document;
			return nullptr;
		}
		Documents.Add(Document);
		Active = Document;
		return Document;
	}

	FSlotGroupDocument* FAnimationDocumentManager::OpenSlotGroups(const EString& InAssetFullPath
		, EString& OutError)
	{
		OutError = EString();
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		if (FAnimationDocument* Existing = FindByPath(NormalizedPath))
		{
			Active = Existing;
			return static_cast<FSlotGroupDocument*>(Existing);
		}

		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			OutError = EString("Cannot parse skeleton asset path: ") + NormalizedPath;
			return nullptr;
		}

		const ESkeletonAsset* LoadedAsset = nullptr;
		if (!(ESkeletonAssetManager::GetManagerSingleton()->LoadSkeletonAsset(
			AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			OutError = EString("Failed to load skeleton asset: ") + NormalizedPath;
			return nullptr;
		}
		FSlotGroupDocument* Document = new FSlotGroupDocument();
		if (!Document->Initialize(NormalizedPath, AssetName, LoadedAsset, OutError))
		{
			delete Document;
			return nullptr;
		}
		Documents.Add(Document);
		Active = Document;
		return Document;
	}

	FAnimationDocument* FAnimationDocumentManager::FindByPath(const EString& InAssetFullPath)const
	{
		const EString NormalizedPath = InAssetFullPath.Replace("\\", "/");
		for (INT32 DocumentIndex = 0; DocumentIndex < Documents.Num(); DocumentIndex++)
		{
			if (Documents[DocumentIndex]
				&& (Documents[DocumentIndex]->AssetPath == NormalizedPath))
			{
				return Documents[DocumentIndex];
			}
		}
		return nullptr;
	}

	FAnimationDocument* FAnimationDocumentManager::GetActive()const
	{
		return Active;
	}

	void FAnimationDocumentManager::SetActive(FAnimationDocument* InDocument)
	{
		if (InDocument && (Documents.Find(InDocument) != -1))
		{
			Active = InDocument;
		}
	}

	const TArray<FAnimationDocument*>& FAnimationDocumentManager::GetDocuments()const
	{
		return Documents;
	}

	void FAnimationDocumentManager::Close(FAnimationDocument* InDocument)
	{
		if (!InDocument)
		{
			return;
		}
		INT32 DocumentIndex = -1;
		if (!Documents.Find(InDocument, DocumentIndex))
		{
			return;
		}
		Documents.RemoveAt(DocumentIndex);
		if (Active == InDocument)
		{
			Active = ((Documents.Num() > 0)
				? Documents[EMath::Clamp(DocumentIndex - 1, 0, Documents.Num() - 1)]
				: nullptr);
		}
		delete InDocument;
	}

	void FAnimationDocumentManager::CloseAll()
	{
		for (INT32 DocumentIndex = 0; DocumentIndex < Documents.Num(); DocumentIndex++)
		{
			delete Documents[DocumentIndex];
		}
		Documents.Empty();
		Active = nullptr;
	}

}

#endif
