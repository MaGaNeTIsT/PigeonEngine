#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Base/DataStructure/Container/Array.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class FStateMachineDocument;
	class FClipDocument;
	class FMontageDocument;
	class FNotifyDocument;
	class FSlotGroupDocument;

	/*
	* Owns every open animation document. The workspace talks to documents through this table
	* so closing/activating a tab never deletes an object another panel is still using.
	*/
	class FAnimationDocumentManager
	{
	public:
		~FAnimationDocumentManager();

	public:
		FStateMachineDocument* OpenStateMachine(const EString& InAssetFullPath, EString& OutError);
		FClipDocument* OpenClip(const EString& InAssetFullPath, EString& OutError);
		FMontageDocument* OpenMontage(const EString& InAssetFullPath, EString& OutError);
		FNotifyDocument* OpenNotify(const EString& InAssetFullPath, EString& OutError);
		FSlotGroupDocument* OpenSlotGroups(const EString& InAssetFullPath, EString& OutError);
		PE_NODISCARD FAnimationDocument* FindByPath(const EString& InAssetFullPath)const;
		PE_NODISCARD FAnimationDocument* GetActive()const;
		void SetActive(FAnimationDocument* InDocument);
		PE_NODISCARD const TArray<FAnimationDocument*>& GetDocuments()const;
		void Close(FAnimationDocument* InDocument);
		void CloseAll();

	private:
		TArray<FAnimationDocument*> Documents;
		FAnimationDocument* Active = nullptr;
	};

}

#endif
