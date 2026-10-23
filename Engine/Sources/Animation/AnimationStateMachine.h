#pragma once

#include <CoreMinimal.h>
#include <EngineCommon.h>

#include "AnimationTypes.h"

namespace PigeonEngine
{

	class EAnimInstance;

	enum EAnimStateMachineParameterType : UINT8
	{
		ANIM_PARAM_FLOAT	= 0,
		ANIM_PARAM_BOOL		= 1,
		ANIM_PARAM_TRIGGER	= 2,
		ANIM_PARAM_COUNT
	};

	enum EAnimStateMachineCompareType : UINT8
	{
		ANIM_COMPARE_GREATER		= 0,
		ANIM_COMPARE_GREATER_EQUAL	= 1,
		ANIM_COMPARE_LESS			= 2,
		ANIM_COMPARE_LESS_EQUAL		= 3,
		ANIM_COMPARE_EQUAL			= 4,
		ANIM_COMPARE_NOT_EQUAL		= 5,
		ANIM_COMPARE_TRIGGER		= 6,
		ANIM_COMPARE_COUNT
	};

	struct EAnimStateMachineParameter
	{
		EString							Name;
		EAnimStateMachineParameterType	Type = ANIM_PARAM_FLOAT;
		FLOAT							DefaultFloat = 0.0f;
		BOOL32							DefaultBool = FALSE;
	};

	struct EAnimStateMachineState
	{
		EString					Name;
		EString					ClipName;
		EAnimationLoopMode		LoopMode = ANIMATION_LOOP_REPEAT;
		FLOAT					PlayRate = 1.0f;
		FLOAT					BlendSeconds = 0.2f;
	};

	struct EAnimStateMachineCondition
	{
		INT32							ParameterIndex = -1;
		EAnimStateMachineCompareType	Compare = ANIM_COMPARE_GREATER;
		FLOAT							Value = 0.0f;
	};

	struct EAnimStateMachineTransition
	{
		// -1 means the transition can start from any state.
		INT32								FromStateIndex = -1;
		INT32								ToStateIndex = -1;
		TArray<EAnimStateMachineCondition>	Conditions;
		FLOAT								BlendSeconds = 0.2f;
		FLOAT								MinTimeInState = 0.0f;
		INT32								Priority = 0;
		BOOL32								bCanTransitionToSelf = FALSE;
	};

	/*
	* The parsed, runtime form of a state machine asset. The JSON text is the authoring
	* source; this object is the compiled graph with name lookups and indices ready, so
	* playback does not parse strings.
	*/
	class EAnimStateMachine : public EObjectBase, public EResourceInterface
	{
		CLASS_VIRTUAL_NOCOPY_BODY(EAnimStateMachine)

	public:
		virtual BOOL32	IsResourceValid()const override;
		virtual BOOL32	InitResource()override;
		virtual void	ReleaseResource()override;

	public:
		EString								Name;
		EString								SkeletonName;
		EString								AnimationAssetName;
		EString								SourceJson;
		TArray<EAnimStateMachineParameter>	Parameters;
		TArray<EAnimStateMachineState>		States;
		TArray<EAnimStateMachineTransition>	Transitions;
		INT32								EntryStateIndex = -1;
		TMap<EString, INT32>				ParameterMapping;
		TMap<EString, INT32>				StateMapping;

	public:
		void Reset();
		BOOL32 RebuildMappings();
		BOOL32 IsValid()const;
		PE_NODISCARD INT32 FindState(const EString& InName)const;
		PE_NODISCARD INT32 FindParameter(const EString& InName)const;
	};

	/*
	* Per-character runtime state for one state machine. Parameters are copied from the
	* machine's defaults on SetMachine; EnterState calls back into EAnimInstance::Play so the
	* state machine reuses the normal clip player and crossfade.
	*/
	class EAnimStateMachineInstance
	{
	public:
		BOOL32 SetMachine(const EAnimStateMachine* InMachine);
		void Clear();
		PE_NODISCARD const EAnimStateMachine* GetMachine()const;
		PE_NODISCARD INT32 GetCurrentStateIndex()const;
		PE_NODISCARD FLOAT GetTimeInState()const;

		void SetFloat(const EString& InName, FLOAT InValue);
		void SetBool(const EString& InName, BOOL32 InValue);
		void SetTrigger(const EString& InName);
		PE_NODISCARD FLOAT GetFloat(const EString& InName)const;
		PE_NODISCARD BOOL32 GetBool(const EString& InName)const;

		/*
		* Advances the state machine and enters a state when a transition fires. The caller
		* runs this before advancing the clip player, so a transition started here is sampled
		* in the same frame.
		*/
		BOOL32 Update(EAnimInstance& InAnimInstance, FLOAT InDeltaSeconds);

	private:
		BOOL32 EnterState(EAnimInstance& InAnimInstance, INT32 InStateIndex, FLOAT InBlendSeconds);
		PE_NODISCARD BOOL32 EvaluateCondition(const EAnimStateMachineCondition& InCondition)const;
		PE_NODISCARD BOOL32 EvaluateTransition(const EAnimStateMachineTransition& InTransition)const;
		void ConsumeTriggers(const EAnimStateMachineTransition& InTransition);

	private:
		const EAnimStateMachine*	Machine = nullptr;
		INT32						CurrentStateIndex = -1;
		FLOAT						TimeInState = 0.0f;
		TArray<FLOAT>				FloatValues;
		TArray<BOOL32>				BoolValues;
		TArray<BOOL32>				TriggerValues;
		BOOL32						bEntryPending = FALSE;
		// Missing-clip failures are a configuration error; warn once instead of every frame.
		BOOL32						bEnterStateFailureLogged = FALSE;
	};

}
