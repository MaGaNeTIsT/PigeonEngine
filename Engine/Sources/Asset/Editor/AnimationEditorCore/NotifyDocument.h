#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class EAnimNotifyAsset;
	class EAnimNotifyDefinition;
	class CJsonObject;

	/*
	* Editable standalone notify definition. The draft JSON is validated through a transient
	* definition; Apply/Save publish a fresh asset so existing clips/montages keep their old
	* definition object until RefreshNotifyReferences updates them. This mirrors the Montage
	* and StateMachine documents and leaves the legacy Notify editor as a fallback.
	*/
	class FNotifyDocument final : public FAnimationDocument
	{
	public:
		~FNotifyDocument() override;

	public:
		BOOL32 Initialize(const EString& InAssetPath, const EString& InAssetName
			, const EAnimNotifyAsset* InAsset, EString& OutError);

		PE_NODISCARD const EAnimNotifyAsset* GetAsset()const;
		PE_NODISCARD const EAnimNotifyDefinition* GetDraftDefinition()const;

		BOOL32 CommitSourceText(const EString& InJsonText, EString& OutError);
		BOOL32 UpdateDefinition(const EString& InName, const EString& InDescription
			, const EString& InClassName, const EString& InPayloadJson
			, FLOAT InDefaultDuration, EString& OutError);

		virtual BOOL32 Apply(EString& OutError) override;
		virtual BOOL32 Save(EString& OutError) override;
		virtual BOOL32 Reload(EString& OutError) override;
		virtual BOOL32 Discard(EString& OutError) override;
		virtual void RefreshDerived() override;

	private:
		BOOL32 CommitJsonText(const EString& InJsonText, EString& OutError);
		void ReleaseDraftDefinition();
		void RebuildDiagnostics();

	private:
		const EAnimNotifyAsset*	m_Asset = nullptr;
		EAnimNotifyDefinition*	m_DraftDefinition = nullptr;
	};

}

#endif
