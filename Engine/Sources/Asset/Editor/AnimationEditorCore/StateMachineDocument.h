#pragma once

#if ENGINE_EDITOR_ONLY

#include <CoreMinimal.h>
#include <Animation/AnimationTypes.h>
#include "AnimationDocument.h"

namespace PigeonEngine
{

	class EAnimStateMachine;
	class EAnimStateMachineAsset;
	class ESkeletonAnimationAsset;
	class CJsonObject;

	/*
	* Editable state machine document. All structured edits mutate DraftJson, validate it by
	* building a transient EAnimStateMachine, and only then replace the graph the panel reads.
	* The live asset is never touched until Apply.
	*/
	class FStateMachineDocument final : public FAnimationDocument
	{
	public:
		~FStateMachineDocument() override;

	public:
		BOOL32 Initialize(const EString& InAssetPath, const EString& InAssetName
			, const EAnimStateMachineAsset* InAsset, EString& OutError);

		PE_NODISCARD EAnimStateMachine* GetDraftMachine()const;
		PE_NODISCARD const ESkeletonAnimationAsset* GetAnimationAsset()const;
		PE_NODISCARD const EAnimStateMachineAsset* GetStateMachineAsset()const;
		PE_NODISCARD EString BuildSerializedJson()const;

		BOOL32 CommitSourceText(const EString& InJsonText, EString& OutError);

		BOOL32 AddState(EString& OutError);
		BOOL32 DeleteState(const EString& InStateName, EString& OutError);
		BOOL32 RenameState(const EString& InOldName, const EString& InNewName, EString& OutError);
		BOOL32 SetEntryState(const EString& InStateName, EString& OutError);
		BOOL32 UpdateState(INT32 InStateIndex, const EString& InClipName
			, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InBlendSeconds, EString& OutError);

		BOOL32 AddTransition(const EString& InFromStateName, const EString& InToStateName, EString& OutError);
		BOOL32 DeleteTransition(INT32 InTransitionIndex, EString& OutError);
		BOOL32 UpdateTransition(INT32 InTransitionIndex, INT32 InPriority, FLOAT InBlendSeconds
			, FLOAT InMinTimeInState, BOOL8 bInCanTransitionToSelf, EString& OutError);

		BOOL32 AddParameter(EString& OutError);
		BOOL32 RemoveParameter(INT32 InParameterIndex, EString& OutError);
		BOOL32 RenameParameter(INT32 InParameterIndex, const EString& InNewName, EString& OutError);
		BOOL32 UpdateParameter(INT32 InParameterIndex, UINT8 InParameterType
			, FLOAT InDefaultFloat, BOOL8 InDefaultBool, EString& OutError);

		BOOL32 AddTransitionCondition(INT32 InTransitionIndex, INT32 InParameterIndex, EString& OutError);
		BOOL32 UpdateTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex
			, INT32 InParameterIndex, const EString& InOp, FLOAT InValue, EString& OutError);
		BOOL32 RemoveTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex, EString& OutError);

		virtual BOOL32 Apply(EString& OutError) override;
		virtual BOOL32 Save(EString& OutError) override;
		virtual BOOL32 Reload(EString& OutError) override;
		virtual BOOL32 Discard(EString& OutError) override;
		virtual void RefreshDerived() override;

	private:
		BOOL32 CommitJsonText(const EString& InJsonText, EString& OutError);
		BOOL32 MutateJson(const TFunction<void(CJsonObject&)>& InMutator, EString& OutError);
		BOOL32 UpdateArrayObject(const EString& InFieldName, INT32 InIndex
			, const TFunction<void(CJsonObject&)>& InMutator, EString& OutError);
		void ReleaseDraftMachine();
		void SyncAnimationAsset();
		void RebuildDiagnostics();
		void LoadNodePositions();
		void WriteNodePositionsToJson(CJsonObject& InRoot)const;

	private:
		const EAnimStateMachineAsset*	m_StateMachineAsset = nullptr;
		EAnimStateMachine*				m_DraftMachine = nullptr;
		const ESkeletonAnimationAsset*	m_AnimationAsset = nullptr;
	};

}

#endif
