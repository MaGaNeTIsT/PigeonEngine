#include "AnimationStateMachine.h"

#include "AnimInstance.h"

#include <Editor/EditorLogManager.h>

namespace PigeonEngine
{

	EAnimStateMachine::EAnimStateMachine()
	{
	}

	EAnimStateMachine::~EAnimStateMachine()
	{
	}

	BOOL32 EAnimStateMachine::IsResourceValid()const
	{
		return IsValid();
	}

	BOOL32 EAnimStateMachine::InitResource()
	{
		return TRUE;
	}

	void EAnimStateMachine::ReleaseResource()
	{
		Reset();
	}

	void EAnimStateMachine::Reset()
	{
		Name = EString();
		SkeletonName = EString();
		AnimationAssetName = EString();
		SourceJson = EString();
		Parameters.Empty();
		States.Empty();
		Transitions.Empty();
		EntryStateIndex = -1;
		ParameterMapping.Empty();
		StateMapping.Empty();
	}

	BOOL32 EAnimStateMachine::RebuildMappings()
	{
		ParameterMapping.Empty();
		StateMapping.Empty();

		for (INT32 ParameterIndex = 0; ParameterIndex < Parameters.Num(); ParameterIndex++)
		{
			const EString& ParameterName = Parameters[ParameterIndex].Name;
			if ((ParameterName.Length() <= 0u) || (ParameterMapping.ContainsKey(ParameterName)))
			{
				return FALSE;
			}
			ParameterMapping.Add(ParameterName, ParameterIndex);
		}

		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			const EString& StateName = States[StateIndex].Name;
			if ((StateName.Length() <= 0u) || (StateMapping.ContainsKey(StateName)))
			{
				return FALSE;
			}
			StateMapping.Add(StateName, StateIndex);
		}
		return TRUE;
	}

	BOOL32 EAnimStateMachine::IsValid()const
	{
		if ((States.Num() <= 0) || (EntryStateIndex < 0) || (EntryStateIndex >= States.Num()))
		{
			return FALSE;
		}
		if ((StateMapping.Num() != States.Num()) || (ParameterMapping.Num() != Parameters.Num()))
		{
			return FALSE;
		}
		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			if (States[StateIndex].ClipName.Length() <= 0u)
			{
				return FALSE;
			}
		}
		for (INT32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = Transitions[TransitionIndex];
			if ((Transition.FromStateIndex < -1) || (Transition.FromStateIndex >= States.Num()))
			{
				return FALSE;
			}
			if ((Transition.ToStateIndex < 0) || (Transition.ToStateIndex >= States.Num()))
			{
				return FALSE;
			}
			for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
			{
				const EAnimStateMachineCondition& Condition = Transition.Conditions[ConditionIndex];
				if ((Condition.ParameterIndex < 0) || (Condition.ParameterIndex >= Parameters.Num()))
				{
					return FALSE;
				}
			}
		}
		return TRUE;
	}

	INT32 EAnimStateMachine::FindState(const EString& InName)const
	{
		INT32 Result = -1;
		StateMapping.FindValue(InName, Result);
		return Result;
	}

	INT32 EAnimStateMachine::FindParameter(const EString& InName)const
	{
		INT32 Result = -1;
		ParameterMapping.FindValue(InName, Result);
		return Result;
	}

	BOOL32 EAnimStateMachineInstance::SetMachine(const EAnimStateMachine* InMachine)
	{
		Clear();
		if ((!InMachine) || (!InMachine->IsValid()))
		{
			return FALSE;
		}

		Machine = InMachine;
		FloatValues.SetNum(Machine->Parameters.Num());
		BoolValues.SetNum(Machine->Parameters.Num());
		TriggerValues.SetNum(Machine->Parameters.Num());
		for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
		{
			const EAnimStateMachineParameter& Parameter = Machine->Parameters[ParameterIndex];
			FloatValues[ParameterIndex] = Parameter.DefaultFloat;
			BoolValues[ParameterIndex] = Parameter.DefaultBool;
			TriggerValues[ParameterIndex] = FALSE;
		}
		CurrentStateIndex = -1;
		TimeInState = 0.0f;
		bEntryPending = TRUE;
		bEnterStateFailureLogged = FALSE;
		return TRUE;
	}

	void EAnimStateMachineInstance::Clear()
	{
		Machine = nullptr;
		CurrentStateIndex = -1;
		TimeInState = 0.0f;
		FloatValues.Empty();
		BoolValues.Empty();
		TriggerValues.Empty();
		bEntryPending = FALSE;
		bEnterStateFailureLogged = FALSE;
	}

	const EAnimStateMachine* EAnimStateMachineInstance::GetMachine()const
	{
		return Machine;
	}

	INT32 EAnimStateMachineInstance::GetCurrentStateIndex()const
	{
		return CurrentStateIndex;
	}

	FLOAT EAnimStateMachineInstance::GetTimeInState()const
	{
		return TimeInState;
	}

	void EAnimStateMachineInstance::SetFloat(const EString& InName, FLOAT InValue)
	{
		if (!Machine)
		{
			return;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InName);
		if ((ParameterIndex >= 0) && (ParameterIndex < FloatValues.Num())
			&& (Machine->Parameters[ParameterIndex].Type == ANIM_PARAM_FLOAT))
		{
			FloatValues[ParameterIndex] = InValue;
		}
	}

	void EAnimStateMachineInstance::SetBool(const EString& InName, BOOL32 InValue)
	{
		if (!Machine)
		{
			return;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InName);
		if ((ParameterIndex >= 0) && (ParameterIndex < BoolValues.Num())
			&& (Machine->Parameters[ParameterIndex].Type == ANIM_PARAM_BOOL))
		{
			BoolValues[ParameterIndex] = InValue;
		}
	}

	void EAnimStateMachineInstance::SetTrigger(const EString& InName)
	{
		if (!Machine)
		{
			return;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InName);
		if ((ParameterIndex >= 0) && (ParameterIndex < TriggerValues.Num())
			&& (Machine->Parameters[ParameterIndex].Type == ANIM_PARAM_TRIGGER))
		{
			TriggerValues[ParameterIndex] = TRUE;
		}
	}

	FLOAT EAnimStateMachineInstance::GetFloat(const EString& InName)const
	{
		if (!Machine)
		{
			return 0.0f;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InName);
		if ((ParameterIndex >= 0) && (ParameterIndex < FloatValues.Num()))
		{
			return FloatValues[ParameterIndex];
		}
		return 0.0f;
	}

	BOOL32 EAnimStateMachineInstance::GetBool(const EString& InName)const
	{
		if (!Machine)
		{
			return FALSE;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InName);
		if ((ParameterIndex >= 0) && (ParameterIndex < BoolValues.Num()))
		{
			return BoolValues[ParameterIndex];
		}
		return FALSE;
	}

	BOOL32 EAnimStateMachineInstance::Update(EAnimInstance& InAnimInstance, FLOAT InDeltaSeconds)
	{
		if ((!Machine) || (!Machine->IsValid()) || (!InAnimInstance.GetSkeletalMeshComponent()))
		{
			return FALSE;
		}

		if (bEntryPending)
		{
			if (EnterState(InAnimInstance, Machine->EntryStateIndex, 0.0f))
			{
				bEntryPending = FALSE;
				bEnterStateFailureLogged = FALSE;
				CurrentStateIndex = Machine->EntryStateIndex;
				TimeInState = 0.0f;
				return TRUE;
			}
			if (!bEnterStateFailureLogged)
			{
				PE_LOG_WARN(EString("State machine '") + Machine->Name + EString("' could not enter its entry state."));
				bEnterStateFailureLogged = TRUE;
			}
			return FALSE;
		}

		TimeInState += InDeltaSeconds;

		const EAnimStateMachineTransition* BestTransition = nullptr;
		for (INT32 TransitionIndex = 0; TransitionIndex < Machine->Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = Machine->Transitions[TransitionIndex];
			if ((Transition.FromStateIndex != -1) && (Transition.FromStateIndex != CurrentStateIndex))
			{
				continue;
			}
			if ((Transition.ToStateIndex == CurrentStateIndex) && (!Transition.bCanTransitionToSelf))
			{
				continue;
			}
			if (TimeInState < Transition.MinTimeInState)
			{
				continue;
			}
			if (!EvaluateTransition(Transition))
			{
				continue;
			}
			if ((!BestTransition) || (Transition.Priority > BestTransition->Priority))
			{
				BestTransition = &Transition;
			}
		}

		if (!BestTransition)
		{
			return FALSE;
		}

		if (!EnterState(InAnimInstance, BestTransition->ToStateIndex, BestTransition->BlendSeconds))
		{
			if (!bEnterStateFailureLogged)
			{
				PE_LOG_WARN(EString("State machine '") + Machine->Name + EString("' could not enter state index ")
					+ ToString(BestTransition->ToStateIndex) + EString("."));
				bEnterStateFailureLogged = TRUE;
			}
			return FALSE;
		}
		ConsumeTriggers(*BestTransition);
		CurrentStateIndex = BestTransition->ToStateIndex;
		TimeInState = 0.0f;
		bEnterStateFailureLogged = FALSE;
		return TRUE;
	}

	BOOL32 EAnimStateMachineInstance::EnterState(EAnimInstance& InAnimInstance, INT32 InStateIndex, FLOAT InBlendSeconds)
	{
		if ((!Machine) || (InStateIndex < 0) || (InStateIndex >= Machine->States.Num()))
		{
			return FALSE;
		}

		const EAnimStateMachineState& State = Machine->States[InStateIndex];
		if (!InAnimInstance.Play(State.ClipName, State.LoopMode, State.PlayRate, InBlendSeconds))
		{
			return FALSE;
		}
		PE_LOG_LOG(EString("State machine '") + Machine->Name + EString("' entered state '") + State.Name + EString("'."));
		return TRUE;
	}

	BOOL32 EAnimStateMachineInstance::EvaluateCondition(const EAnimStateMachineCondition& InCondition)const
	{
		if ((!Machine) || (InCondition.ParameterIndex < 0) || (InCondition.ParameterIndex >= Machine->Parameters.Num()))
		{
			return FALSE;
		}

		const EAnimStateMachineParameter& Parameter = Machine->Parameters[InCondition.ParameterIndex];
		if (Parameter.Type == ANIM_PARAM_FLOAT)
		{
			if (InCondition.ParameterIndex >= FloatValues.Num())
			{
				return FALSE;
			}
			const FLOAT ParameterValue = FloatValues[InCondition.ParameterIndex];
			switch (InCondition.Compare)
			{
			case ANIM_COMPARE_GREATER:			return ((ParameterValue > InCondition.Value) ? TRUE : FALSE);
			case ANIM_COMPARE_GREATER_EQUAL:	return ((ParameterValue >= InCondition.Value) ? TRUE : FALSE);
			case ANIM_COMPARE_LESS:				return ((ParameterValue < InCondition.Value) ? TRUE : FALSE);
			case ANIM_COMPARE_LESS_EQUAL:		return ((ParameterValue <= InCondition.Value) ? TRUE : FALSE);
			case ANIM_COMPARE_EQUAL:			return ((ParameterValue == InCondition.Value) ? TRUE : FALSE);
			case ANIM_COMPARE_NOT_EQUAL:		return ((ParameterValue != InCondition.Value) ? TRUE : FALSE);
			default:							return FALSE;
			}
		}
		if (Parameter.Type == ANIM_PARAM_BOOL)
		{
			if (InCondition.ParameterIndex >= BoolValues.Num())
			{
				return FALSE;
			}
			const BOOL32 ParameterValue = BoolValues[InCondition.ParameterIndex];
			const BOOL32 ExpectedValue = ((InCondition.Value != 0.0f) ? TRUE : FALSE);
			switch (InCondition.Compare)
			{
			case ANIM_COMPARE_EQUAL:		return ((ParameterValue == ExpectedValue) ? TRUE : FALSE);
			case ANIM_COMPARE_NOT_EQUAL:	return ((ParameterValue != ExpectedValue) ? TRUE : FALSE);
			default:						return FALSE;
			}
		}
		if (Parameter.Type == ANIM_PARAM_TRIGGER)
		{
			if (InCondition.ParameterIndex >= TriggerValues.Num())
			{
				return FALSE;
			}
			return (((InCondition.Compare == ANIM_COMPARE_TRIGGER) && (TriggerValues[InCondition.ParameterIndex] != FALSE)) ? TRUE : FALSE);
		}
		return FALSE;
	}

	BOOL32 EAnimStateMachineInstance::EvaluateTransition(const EAnimStateMachineTransition& InTransition)const
	{
		for (INT32 ConditionIndex = 0; ConditionIndex < InTransition.Conditions.Num(); ConditionIndex++)
		{
			if (!EvaluateCondition(InTransition.Conditions[ConditionIndex]))
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	void EAnimStateMachineInstance::ConsumeTriggers(const EAnimStateMachineTransition& InTransition)
	{
		if (!Machine)
		{
			return;
		}
		for (INT32 ConditionIndex = 0; ConditionIndex < InTransition.Conditions.Num(); ConditionIndex++)
		{
			const EAnimStateMachineCondition& Condition = InTransition.Conditions[ConditionIndex];
			if ((Condition.ParameterIndex >= 0) && (Condition.ParameterIndex < Machine->Parameters.Num())
				&& (Machine->Parameters[Condition.ParameterIndex].Type == ANIM_PARAM_TRIGGER)
				&& (Condition.ParameterIndex < TriggerValues.Num()))
			{
				TriggerValues[Condition.ParameterIndex] = FALSE;
			}
		}
	}

}
