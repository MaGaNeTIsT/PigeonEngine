#include "StateMachineDocument.h"

#if ENGINE_EDITOR_ONLY

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <AnimationAsset/AnimationAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Config/EngineConfig.h>
#include <RapidJson/Headers/JsonObject.h>

namespace PigeonEngine
{

	namespace
	{
		const CHAR* GetLoopModeText(const EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			case ANIMATION_LOOP_REPEAT:
			default:
				return "Repeat";
			}
		}

		const CHAR* GetParameterTypeText(const EAnimStateMachineParameterType InType)
		{
			switch (InType)
			{
			case ANIM_PARAM_BOOL:
				return "bool";
			case ANIM_PARAM_TRIGGER:
				return "trigger";
			case ANIM_PARAM_FLOAT:
			default:
				return "float";
			}
		}

		const CHAR* GetCompareOpText(const EAnimStateMachineCompareType InCompare)
		{
			switch (InCompare)
			{
			case ANIM_COMPARE_GREATER_EQUAL:
				return ">=";
			case ANIM_COMPARE_LESS:
				return "<";
			case ANIM_COMPARE_LESS_EQUAL:
				return "<=";
			case ANIM_COMPARE_EQUAL:
				return "==";
			case ANIM_COMPARE_NOT_EQUAL:
				return "!=";
			case ANIM_COMPARE_TRIGGER:
				return "trigger";
			case ANIM_COMPARE_GREATER:
			default:
				return ">";
			}
		}

		BOOL32 IsCompareTypeValidForParameter(const EAnimStateMachineParameterType InType
			, const EAnimStateMachineCompareType InCompare)
		{
			switch (InType)
			{
			case ANIM_PARAM_BOOL:
				return (((InCompare == ANIM_COMPARE_EQUAL) || (InCompare == ANIM_COMPARE_NOT_EQUAL)) ? TRUE : FALSE);
			case ANIM_PARAM_TRIGGER:
				return ((InCompare == ANIM_COMPARE_TRIGGER) ? TRUE : FALSE);
			case ANIM_PARAM_FLOAT:
			default:
				return ((InCompare != ANIM_COMPARE_TRIGGER) ? TRUE : FALSE);
			}
		}

		void GetSafeConditionOpAndValue(const EAnimStateMachineParameter& InParameter
			, EString& OutOp, FLOAT& OutValue)
		{
			switch (InParameter.Type)
			{
			case ANIM_PARAM_BOOL:
				OutOp = EString("!=");
				OutValue = ((InParameter.DefaultBool != FALSE) ? 0.0f : 1.0f);
				break;
			case ANIM_PARAM_TRIGGER:
				OutOp = EString("trigger");
				OutValue = 0.0f;
				break;
			case ANIM_PARAM_FLOAT:
			default:
				OutOp = EString(">");
				OutValue = 1000000000.0f;
				break;
			}
		}

		struct FJsonObjectArrayOwner
		{
			explicit FJsonObjectArrayOwner(TArray<CJsonObject*>& InObjects) : Objects(InObjects) {}
			~FJsonObjectArrayOwner()
			{
				for (INT32 Index = 0; Index < Objects.Num(); Index++)
				{
					delete Objects[Index];
				}
				Objects.Empty();
			}
			TArray<CJsonObject*>& Objects;
		};

		EString GetJsonObjectString(CJsonObject& InObject, const EString& InFieldName)
		{
			EString Result;
			InObject.GetStringField(InFieldName, Result);
			return Result;
		}

		BOOL32 JsonStateNameExists(const TArray<CJsonObject*>& InStates, const EString& InName)
		{
			for (INT32 StateIndex = 0; StateIndex < InStates.Num(); StateIndex++)
			{
				if ((InStates[StateIndex])
					&& (GetJsonObjectString(*InStates[StateIndex], EString("Name")) == InName))
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 JsonParameterNameExists(const TArray<CJsonObject*>& InParameters, const EString& InName)
		{
			for (INT32 ParameterIndex = 0; ParameterIndex < InParameters.Num(); ParameterIndex++)
			{
				if ((InParameters[ParameterIndex])
					&& (GetJsonObjectString(*InParameters[ParameterIndex], EString("Name")) == InName))
				{
					return TRUE;
				}
			}
			return FALSE;
		}
	}

	FStateMachineDocument::~FStateMachineDocument()
	{
		ReleaseDraftMachine();
	}

	BOOL32 FStateMachineDocument::Initialize(const EString& InAssetPath, const EString& InAssetName
		, const EAnimStateMachineAsset* InAsset, EString& OutError)
	{
		OutError = EString();
		if (!InAsset)
		{
			OutError = EString("No state machine asset was supplied.");
			return FALSE;
		}
		const EAnimStateMachine* Machine = InAsset->GetStoragedResource();
		if (!Machine)
		{
			OutError = EString("State machine asset has no parsed resource.");
			return FALSE;
		}

		Kind = EAnimationDocumentKind::STATE_MACHINE;
		AssetPath = InAssetPath.Replace("\\", "/");
		AssetDirectory = EPath::GetFileFolderPath(AssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		AssetName = InAssetName;
		DisplayName = InAssetName;
		PublishedAsset = InAsset;
		m_StateMachineAsset = InAsset;
		DraftJson = Machine->SourceJson;
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		LastError = EString();
		LoadNodePositions();
		if (!CommitJsonText(DraftJson, OutError))
		{
			return FALSE;
		}
		Commands.Clear();
		Selection.Clear();
		return TRUE;
	}

	EAnimStateMachine* FStateMachineDocument::GetDraftMachine()const
	{
		return m_DraftMachine;
	}

	const ESkeletonAnimationAsset* FStateMachineDocument::GetAnimationAsset()const
	{
		return m_AnimationAsset;
	}

	const EAnimStateMachineAsset* FStateMachineDocument::GetStateMachineAsset()const
	{
		return m_StateMachineAsset;
	}

	EString FStateMachineDocument::BuildSerializedJson()const
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			return DraftJson;
		}
		WriteNodePositionsToJson(Root);
		return Root.ToString();
	}

	BOOL32 FStateMachineDocument::CommitSourceText(const EString& InJsonText, EString& OutError)
	{
		return CommitJsonText(InJsonText, OutError);
	}

	BOOL32 FStateMachineDocument::CommitJsonText(const EString& InJsonText, EString& OutError)
	{
		OutError = EString();
		EAnimStateMachine* NewMachine = EAnimStateMachineManager::GetManagerSingleton()
			->BuildTransientStateMachine(InJsonText, OutError);
		if (!NewMachine)
		{
			LastError = OutError;
			return FALSE;
		}
		if (NewMachine->Name.Length() <= 0u)
		{
			NewMachine->Name = AssetName;
		}

		ReleaseDraftMachine();
		m_DraftMachine = NewMachine;
		DraftJson = InJsonText;
		LastError = EString();
		SyncAnimationAsset();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::MutateJson(const TFunction<void(CJsonObject&)>& InMutator, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		InMutator(Root);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FStateMachineDocument::UpdateArrayObject(const EString& InFieldName, INT32 InIndex
		, const TFunction<void(CJsonObject&)>& InMutator, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Objects;
		if (!(Root.GetObjectArrayField(InFieldName, Objects)))
		{
			OutError = EString("State machine JSON has no '") + InFieldName + EString("' array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Objects);
		if ((InIndex < 0) || (InIndex >= Objects.Num()) || (!Objects[InIndex]))
		{
			OutError = EString("State machine array index is out of range: ") + InFieldName;
			LastError = OutError;
			return FALSE;
		}
		InMutator(*Objects[InIndex]);
		Root.SetObjectArrayField(InFieldName, Objects);
		return CommitJsonText(Root.ToString(), OutError);
	}

	void FStateMachineDocument::ReleaseDraftMachine()
	{
		if (m_DraftMachine)
		{
			delete m_DraftMachine;
			m_DraftMachine = nullptr;
		}
	}

	void FStateMachineDocument::SyncAnimationAsset()
	{
		m_AnimationAsset = nullptr;
		if ((!m_DraftMachine) || (m_DraftMachine->AnimationAssetName.Length() <= 0u))
		{
			return;
		}
		const ESkeletonAnimationAsset* LoadedAnimation = nullptr;
		if (EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			EBaseSettings::ENGINE_ANIMATION_PATH, m_DraftMachine->AnimationAssetName, LoadedAnimation))
		{
			m_AnimationAsset = LoadedAnimation;
		}
	}

	void FStateMachineDocument::LoadNodePositions()
	{
		NodePositions.Empty();
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			return;
		}
		TArray<CJsonObject*> States;
		if (!(Root.GetObjectArrayField(PE_TEXT("States"), States)))
		{
			return;
		}
		FJsonObjectArrayOwner Owner(States);
		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			CJsonObject* State = States[StateIndex];
			if (!State)
			{
				continue;
			}
			const EString Name = GetJsonObjectString(*State, PE_TEXT("Name"));
			if (Name.Length() <= 0u)
			{
				continue;
			}
			FLOAT NodeX = 0.0f;
			FLOAT NodeY = 0.0f;
			if (State->GetFloatField(PE_TEXT("NodeX"), NodeX)
				&& State->GetFloatField(PE_TEXT("NodeY"), NodeY))
			{
				NodePositions.Add(Name, ImVec2(NodeX, NodeY));
			}
		}
	}

	void FStateMachineDocument::WriteNodePositionsToJson(CJsonObject& InRoot)const
	{
		TArray<CJsonObject*> States;
		if (!(InRoot.GetObjectArrayField(PE_TEXT("States"), States)))
		{
			return;
		}
		FJsonObjectArrayOwner Owner(States);
		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			CJsonObject* State = States[StateIndex];
			if (!State)
			{
				continue;
			}
			const EString Name = GetJsonObjectString(*State, PE_TEXT("Name"));
			const ImVec2* Position = NodePositions.FindValueAsPtr(Name);
			if (Position)
			{
				State->SetFloatField(PE_TEXT("NodeX"), Position->x);
				State->SetFloatField(PE_TEXT("NodeY"), Position->y);
			}
		}
		InRoot.SetObjectArrayField(PE_TEXT("States"), States);
	}

	void FStateMachineDocument::RebuildDiagnostics()
	{
		Diagnostics.Clear();
		if (!m_DraftMachine)
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
				, ((LastError.Length() > 0u) ? LastError : EString("State machine draft is not valid."))
				, EString(), -1, -1, EString(), EEditorSelectionType::MACHINE);
			return;
		}
		if ((m_DraftMachine->EntryStateIndex < 0)
			|| (m_DraftMachine->EntryStateIndex >= m_DraftMachine->States.Num()))
		{
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, EString("No valid entry state is defined.")
				, EString("MACHINE"), -1, -1, EString(), EEditorSelectionType::MACHINE);
		}

		TMap<EString, INT32> StateNames;
		for (INT32 StateIndex = 0; StateIndex < m_DraftMachine->States.Num(); StateIndex++)
		{
			const EAnimStateMachineState& State = m_DraftMachine->States[StateIndex];
			if (StateNames.ContainsKey(State.Name))
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
					, EString("Duplicate state name: ") + State.Name, State.Name, StateIndex
					, -1, EString(), EEditorSelectionType::STATE);
			}
			else
			{
				StateNames.Add(State.Name, StateIndex);
			}

			if (m_AnimationAsset)
			{
				const ESkeletonAnimation* Animation = m_AnimationAsset->GetStoragedResource();
				if ((Animation) && (State.ClipName.Length() > 0u) && (!Animation->GetAnimationClip(State.ClipName)))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
						, EString("State clip is not present in animation asset: ") + State.ClipName
						, State.Name, StateIndex, -1, EString(), EEditorSelectionType::STATE);
				}
			}
			else
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
					, EString("Animation asset is not loaded: ") + m_DraftMachine->AnimationAssetName
					, EString("MACHINE"), -1, -1, EString(), EEditorSelectionType::MACHINE);
			}
		}

		TMap<EString, INT32> ParameterNames;
		for (INT32 ParameterIndex = 0; ParameterIndex < m_DraftMachine->Parameters.Num(); ParameterIndex++)
		{
			const EAnimStateMachineParameter& Parameter = m_DraftMachine->Parameters[ParameterIndex];
			if (ParameterNames.ContainsKey(Parameter.Name))
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
					, EString("Duplicate parameter name: ") + Parameter.Name, Parameter.Name, ParameterIndex
					, -1, EString(), EEditorSelectionType::PARAMETER);
			}
			else
			{
				ParameterNames.Add(Parameter.Name, ParameterIndex);
			}
		}

		for (INT32 TransitionIndex = 0; TransitionIndex < m_DraftMachine->Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = m_DraftMachine->Transitions[TransitionIndex];
			for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
			{
				const EAnimStateMachineCondition& Condition = Transition.Conditions[ConditionIndex];
				if ((Condition.ParameterIndex < 0)
					|| (Condition.ParameterIndex >= m_DraftMachine->Parameters.Num()))
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
						, EString("Transition condition references an unknown parameter.")
						, EString("TRANSITION"), TransitionIndex, ConditionIndex, EString()
						, EEditorSelectionType::CONDITION);
					continue;
				}
				const EAnimStateMachineParameter& Parameter =
					m_DraftMachine->Parameters[Condition.ParameterIndex];
				if (IsCompareTypeValidForParameter(Parameter.Type, Condition.Compare) == FALSE)
				{
					Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR
						, EString("Operator '") + EString(GetCompareOpText(Condition.Compare))
						+ EString("' is invalid for parameter type ") + EString(GetParameterTypeText(Parameter.Type))
						, Parameter.Name, TransitionIndex, ConditionIndex, EString()
						, EEditorSelectionType::CONDITION);
				}
			}
		}

		TArray<BOOL32> Reachable;
		Reachable.SetNum(m_DraftMachine->States.Num());
		for (INT32 StateIndex = 0; StateIndex < Reachable.Num(); StateIndex++)
		{
			Reachable[StateIndex] = FALSE;
		}
		TArray<INT32> Queue;
		if ((m_DraftMachine->EntryStateIndex >= 0)
			&& (m_DraftMachine->EntryStateIndex < m_DraftMachine->States.Num()))
		{
			Reachable[m_DraftMachine->EntryStateIndex] = TRUE;
			Queue.Add(m_DraftMachine->EntryStateIndex);
		}
		for (INT32 QueueIndex = 0; QueueIndex < Queue.Num(); QueueIndex++)
		{
			const INT32 FromIndex = Queue[QueueIndex];
			for (INT32 TransitionIndex = 0; TransitionIndex < m_DraftMachine->Transitions.Num(); TransitionIndex++)
			{
				const EAnimStateMachineTransition& Transition = m_DraftMachine->Transitions[TransitionIndex];
				if ((Transition.FromStateIndex >= 0) && (Transition.FromStateIndex != FromIndex))
				{
					continue;
				}
				if ((Transition.ToStateIndex < 0) || (Transition.ToStateIndex >= Reachable.Num()))
				{
					continue;
				}
				if (Reachable[Transition.ToStateIndex] == FALSE)
				{
					Reachable[Transition.ToStateIndex] = TRUE;
					Queue.Add(Transition.ToStateIndex);
				}
			}
		}
		for (INT32 StateIndex = 0; StateIndex < Reachable.Num(); StateIndex++)
		{
			if (Reachable[StateIndex] == FALSE)
			{
				Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING
					, EString("State is unreachable from the entry: ") + m_DraftMachine->States[StateIndex].Name
					, m_DraftMachine->States[StateIndex].Name, StateIndex, -1, EString()
					, EEditorSelectionType::STATE);
			}
		}
	}

	void FStateMachineDocument::RefreshDerived()
	{
		EString Error;
		if (!CommitJsonText(DraftJson, Error))
		{
			LastError = Error;
			Diagnostics.Clear();
			Diagnostics.Add(EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR, Error, EString(), -1, -1, EString()
				, EEditorSelectionType::MACHINE);
		}
	}

	BOOL32 FStateMachineDocument::Apply(EString& OutError)
	{
		const EString Json = BuildSerializedJson();
		const EAnimStateMachineAsset* NewAsset = nullptr;
		if (!EAnimStateMachineManager::GetManagerSingleton()->ApplyStateMachineAsset(
			m_StateMachineAsset, Json, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_StateMachineAsset = NewAsset;
		PublishedAsset = NewAsset;
		DraftJson = Json;
		AppliedJson = Json;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::Save(EString& OutError)
	{
		const EString Json = BuildSerializedJson();
		const EAnimStateMachineAsset* NewAsset = nullptr;
		if (!EAnimStateMachineManager::GetManagerSingleton()->SaveStateMachineAssetCopyOnWrite(
			m_StateMachineAsset, Json, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		m_StateMachineAsset = NewAsset;
		PublishedAsset = NewAsset;
		DraftJson = Json;
		AppliedJson = Json;
		BaselineJson = Json;
		LastError = EString();
		RebuildDiagnostics();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::Reload(EString& OutError)
	{
		const EAnimStateMachineAsset* NewAsset = nullptr;
		if (!EAnimStateMachineManager::GetManagerSingleton()->ReloadStateMachineAsset(
			AssetDirectory, AssetName, NewAsset, OutError))
		{
			LastError = OutError;
			return FALSE;
		}
		const EAnimStateMachine* Machine = (NewAsset ? NewAsset->GetStoragedResource() : nullptr);
		if (!Machine)
		{
			OutError = EString("Reloaded state machine has no resource.");
			LastError = OutError;
			return FALSE;
		}
		m_StateMachineAsset = NewAsset;
		PublishedAsset = NewAsset;
		DraftJson = Machine->SourceJson;
		BaselineJson = DraftJson;
		AppliedJson = DraftJson;
		LoadNodePositions();
		if (!CommitJsonText(DraftJson, OutError))
		{
			return FALSE;
		}
		Commands.Clear();
		Selection.Clear();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::Discard(EString& OutError)
	{
		// Reloading from disk is the strongest discard: it also replaces any draft asset
		// that was already published to the live cache.
		return Reload(OutError);
	}

	BOOL32 FStateMachineDocument::AddState(EString& OutError)
	{
		if ((!m_DraftMachine) || (m_DraftMachine->States.Num() <= 0))
		{
			OutError = EString("Cannot add a state to an empty machine.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> States;
		Root.GetObjectArrayField(PE_TEXT("States"), States);
		FJsonObjectArrayOwner Owner(States);
		if (States.Num() <= 0)
		{
			OutError = EString("State machine JSON has no states array.");
			LastError = OutError;
			return FALSE;
		}

		INT32 Suffix = States.Num();
		EString NewName;
		do
		{
			NewName = EString("State") + ToString(Suffix++);
		} while (JsonStateNameExists(States, NewName));

		EString ClipName;
		if (States[0])
		{
			States[0]->GetStringField(PE_TEXT("Clip"), ClipName);
		}
		CJsonObject* NewState = new CJsonObject();
		NewState->SetStringField(PE_TEXT("Name"), NewName);
		NewState->SetStringField(PE_TEXT("Clip"), ClipName);
		NewState->SetStringField(PE_TEXT("Loop"), EString("Repeat"));
		NewState->SetFloatField(PE_TEXT("Rate"), 1.0f);
		NewState->SetFloatField(PE_TEXT("Blend"), 0.2f);
		States.Add(NewState);
		Root.SetObjectArrayField(PE_TEXT("States"), States);

		EString EntryName;
		if ((States.Num() == 1)
			&& ((!Root.GetStringField(PE_TEXT("Entry"), EntryName)) || (EntryName.Length() == 0u)))
		{
			Root.SetStringField(PE_TEXT("Entry"), NewName);
		}

		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Type = EEditorSelectionType::STATE;
		Selection.ObjectId = NewName;
		Selection.ObjectIndex = -1;
		return TRUE;
	}

	BOOL32 FStateMachineDocument::DeleteState(const EString& InStateName, EString& OutError)
	{
		if ((!m_DraftMachine) || (m_DraftMachine->States.Num() <= 1))
		{
			OutError = EString("Cannot delete the last state.");
			LastError = OutError;
			return FALSE;
		}
		const EString DeletedStateName = InStateName;
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> States;
		Root.GetObjectArrayField(PE_TEXT("States"), States);
		FJsonObjectArrayOwner StateOwner(States);
		INT32 RemovedStateIndex = -1;
		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			if ((States[StateIndex]) && (GetJsonObjectString(*States[StateIndex], PE_TEXT("Name")) == DeletedStateName))
			{
				RemovedStateIndex = StateIndex;
				break;
			}
		}
		if (RemovedStateIndex < 0)
		{
			OutError = EString("State not found in JSON: ") + DeletedStateName;
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* RemovedState = States[RemovedStateIndex];
		States.RemoveAt(RemovedStateIndex);
		delete RemovedState;

		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		TArray<CJsonObject*> KeptTransitions;
		for (INT32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); TransitionIndex++)
		{
			CJsonObject* Transition = Transitions[TransitionIndex];
			if (!Transition)
			{
				continue;
			}
			if ((GetJsonObjectString(*Transition, PE_TEXT("From")) == DeletedStateName)
				|| (GetJsonObjectString(*Transition, PE_TEXT("To")) == DeletedStateName))
			{
				continue;
			}
			KeptTransitions.Add(Transition);
		}
		Root.SetObjectArrayField(PE_TEXT("States"), States);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), KeptTransitions);

		EString EntryName;
		if ((!Root.GetStringField(PE_TEXT("Entry"), EntryName))
			|| (EntryName == DeletedStateName) || (EntryName.Length() == 0u))
		{
			EString FirstStateName;
			if ((States.Num() > 0) && (States[0]))
			{
				States[0]->GetStringField(PE_TEXT("Name"), FirstStateName);
			}
			if (FirstStateName.Length() > 0u)
			{
				Root.SetStringField(PE_TEXT("Entry"), FirstStateName);
			}
		}

		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		NodePositions.Remove(DeletedStateName);
		Selection.Clear();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::RenameState(const EString& InOldName, const EString& InNewName, EString& OutError)
	{
		if (InNewName.Length() <= 0u)
		{
			OutError = EString("State name cannot be empty.");
			LastError = OutError;
			return FALSE;
		}
		if (InOldName == InNewName)
		{
			return TRUE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> States;
		if (!(Root.GetObjectArrayField(PE_TEXT("States"), States)))
		{
			OutError = EString("State machine JSON has no states array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner StateOwner(States);

		BOOL32 bFoundOldName = FALSE;
		for (INT32 StateIndex = 0; StateIndex < States.Num(); StateIndex++)
		{
			if (!States[StateIndex])
			{
				continue;
			}
			const EString StateName = GetJsonObjectString(*States[StateIndex], PE_TEXT("Name"));
			if ((StateName == InNewName) && (StateName != InOldName))
			{
				OutError = EString("State already exists: ") + InNewName;
				LastError = OutError;
				return FALSE;
			}
			if (StateName == InOldName)
			{
				States[StateIndex]->SetStringField(PE_TEXT("Name"), InNewName);
				bFoundOldName = TRUE;
			}
		}
		if (bFoundOldName == FALSE)
		{
			OutError = EString("State not found: ") + InOldName;
			LastError = OutError;
			return FALSE;
		}
		Root.SetObjectArrayField(PE_TEXT("States"), States);

		EString EntryName;
		if (Root.GetStringField(PE_TEXT("Entry"), EntryName) && (EntryName == InOldName))
		{
			Root.SetStringField(PE_TEXT("Entry"), InNewName);
		}

		TArray<CJsonObject*> Transitions;
		const BOOL8 bHasTransitions = Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		if (bHasTransitions != FALSE)
		{
			for (INT32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); TransitionIndex++)
			{
				CJsonObject* Transition = Transitions[TransitionIndex];
				if (!Transition)
				{
					continue;
				}
				if (GetJsonObjectString(*Transition, PE_TEXT("From")) == InOldName)
				{
					Transition->SetStringField(PE_TEXT("From"), InNewName);
				}
				if (GetJsonObjectString(*Transition, PE_TEXT("To")) == InOldName)
				{
					Transition->SetStringField(PE_TEXT("To"), InNewName);
				}
			}
			Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		}

		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		ImVec2* StoredPosition = NodePositions.FindValueAsPtr(InOldName);
		if (StoredPosition)
		{
			const ImVec2 Position = *StoredPosition;
			NodePositions.Remove(InOldName);
			NodePositions.Add(InNewName, Position);
		}
		if ((Selection.Is(EEditorSelectionType::STATE)) && (Selection.ObjectId == InOldName))
		{
			Selection.ObjectId = InNewName;
		}
		return TRUE;
	}

	BOOL32 FStateMachineDocument::SetEntryState(const EString& InStateName, EString& OutError)
	{
		if (InStateName.Length() <= 0u)
		{
			OutError = EString("No entry state selected.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		Root.SetStringField(PE_TEXT("Entry"), InStateName);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FStateMachineDocument::UpdateState(INT32 InStateIndex, const EString& InClipName
		, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InBlendSeconds, EString& OutError)
	{
		const EString LoopText(GetLoopModeText(InLoopMode));
		return UpdateArrayObject(PE_TEXT("States"), InStateIndex
			, [InClipName, LoopText, InPlayRate, InBlendSeconds](CJsonObject& InState)
			{
				InState.SetStringField(PE_TEXT("Clip"), InClipName);
				InState.SetStringField(PE_TEXT("Loop"), LoopText);
				InState.SetFloatField(PE_TEXT("Rate"), InPlayRate);
				InState.SetFloatField(PE_TEXT("Blend"), InBlendSeconds);
			}, OutError);
	}

	BOOL32 FStateMachineDocument::AddTransition(const EString& InFromStateName
		, const EString& InToStateName, EString& OutError)
	{
		if ((!m_DraftMachine) || (InToStateName.Length() <= 0u))
		{
			OutError = EString("A transition needs a target state.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);

		TArray<CJsonObject*> Conditions;
		FJsonObjectArrayOwner ConditionOwner(Conditions);
		if (m_DraftMachine->Parameters.Num() > 0)
		{
			const EAnimStateMachineParameter& Parameter = m_DraftMachine->Parameters[0];
			EString DefaultOp;
			FLOAT DefaultValue = 0.0f;
			GetSafeConditionOpAndValue(Parameter, DefaultOp, DefaultValue);
			CJsonObject* Condition = new CJsonObject();
			Condition->SetStringField(PE_TEXT("Param"), Parameter.Name);
			Condition->SetStringField(PE_TEXT("Op"), DefaultOp);
			Condition->SetFloatField(PE_TEXT("Value"), DefaultValue);
			Conditions.Add(Condition);
		}

		CJsonObject* NewTransition = new CJsonObject();
		NewTransition->SetStringField(PE_TEXT("From"), InFromStateName);
		NewTransition->SetStringField(PE_TEXT("To"), InToStateName);
		NewTransition->SetIntField(PE_TEXT("Priority"), 0);
		NewTransition->SetFloatField(PE_TEXT("Blend"), 0.2f);
		NewTransition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		Transitions.Add(NewTransition);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);

		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Type = EEditorSelectionType::TRANSITION;
		Selection.ObjectIndex = m_DraftMachine ? (m_DraftMachine->Transitions.Num() - 1) : -1;
		return TRUE;
	}

	BOOL32 FStateMachineDocument::DeleteTransition(INT32 InTransitionIndex, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner Owner(Transitions);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= Transitions.Num()))
		{
			OutError = EString("Transition index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Removed = Transitions[InTransitionIndex];
		Transitions.RemoveAt(InTransitionIndex);
		delete Removed;
		Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		if (Selection.Is(EEditorSelectionType::TRANSITION))
		{
			Selection.Clear();
		}
		return TRUE;
	}

	BOOL32 FStateMachineDocument::UpdateTransition(INT32 InTransitionIndex, INT32 InPriority
		, FLOAT InBlendSeconds, FLOAT InMinTimeInState, BOOL8 bInCanTransitionToSelf, EString& OutError)
	{
		return UpdateArrayObject(PE_TEXT("Transitions"), InTransitionIndex
			, [InPriority, InBlendSeconds, InMinTimeInState, bInCanTransitionToSelf](CJsonObject& InTransition)
			{
				InTransition.SetIntField(PE_TEXT("Priority"), InPriority);
				InTransition.SetFloatField(PE_TEXT("Blend"), InBlendSeconds);
				InTransition.SetFloatField(PE_TEXT("MinTime"), InMinTimeInState);
				InTransition.SetBoolField(PE_TEXT("CanSelf"), bInCanTransitionToSelf);
			}, OutError);
	}

	BOOL32 FStateMachineDocument::AddParameter(EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		FJsonObjectArrayOwner Owner(Parameters);

		INT32 Suffix = Parameters.Num();
		EString NewName;
		do
		{
			NewName = EString("Param") + ToString(Suffix++);
		} while (JsonParameterNameExists(Parameters, NewName));

		CJsonObject* NewParameter = new CJsonObject();
		NewParameter->SetStringField(PE_TEXT("Name"), NewName);
		NewParameter->SetStringField(PE_TEXT("Type"), EString("float"));
		NewParameter->SetFloatField(PE_TEXT("Default"), 0.0f);
		Parameters.Add(NewParameter);
		Root.SetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Type = EEditorSelectionType::PARAMETER;
		Selection.ObjectId = NewName;
		return TRUE;
	}

	BOOL32 FStateMachineDocument::RemoveParameter(INT32 InParameterIndex, EString& OutError)
	{
		if ((!m_DraftMachine) || (InParameterIndex < 0)
			|| (InParameterIndex >= m_DraftMachine->Parameters.Num()))
		{
			OutError = EString("Parameter index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		const EString ParameterName = m_DraftMachine->Parameters[InParameterIndex].Name;
		for (INT32 TransitionIndex = 0; TransitionIndex < m_DraftMachine->Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = m_DraftMachine->Transitions[TransitionIndex];
			for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
			{
				const INT32 ParameterRef = Transition.Conditions[ConditionIndex].ParameterIndex;
				if ((ParameterRef >= 0) && (ParameterRef < m_DraftMachine->Parameters.Num())
					&& (m_DraftMachine->Parameters[ParameterRef].Name == ParameterName))
				{
					OutError = EString("Parameter is used by a transition condition: ") + ParameterName;
					LastError = OutError;
					return FALSE;
				}
			}
		}

		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		if (!(Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters)))
		{
			OutError = EString("State machine JSON has no parameters array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner Owner(Parameters);
		if ((InParameterIndex < 0) || (InParameterIndex >= Parameters.Num()) || (!Parameters[InParameterIndex]))
		{
			OutError = EString("Parameter index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Removed = Parameters[InParameterIndex];
		Parameters.RemoveAt(InParameterIndex);
		delete Removed;
		Root.SetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		Selection.Clear();
		return TRUE;
	}

	BOOL32 FStateMachineDocument::RenameParameter(INT32 InParameterIndex, const EString& InNewName, EString& OutError)
	{
		if ((!m_DraftMachine) || (InParameterIndex < 0)
			|| (InParameterIndex >= m_DraftMachine->Parameters.Num()))
		{
			OutError = EString("Parameter index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		if (InNewName.Length() <= 0u)
		{
			OutError = EString("Parameter name cannot be empty.");
			LastError = OutError;
			return FALSE;
		}
		const EString OldName = m_DraftMachine->Parameters[InParameterIndex].Name;
		if (OldName == InNewName)
		{
			return TRUE;
		}
		for (INT32 ParameterIndex = 0; ParameterIndex < m_DraftMachine->Parameters.Num(); ParameterIndex++)
		{
			if ((ParameterIndex != InParameterIndex)
				&& (m_DraftMachine->Parameters[ParameterIndex].Name == InNewName))
			{
				OutError = EString("Parameter already exists: ") + InNewName;
				LastError = OutError;
				return FALSE;
			}
		}

		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		if (!(Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters)))
		{
			OutError = EString("State machine JSON has no parameters array.");
			LastError = OutError;
			return FALSE;
		}
		FJsonObjectArrayOwner ParameterOwner(Parameters);
		if ((InParameterIndex >= Parameters.Num()) || (!Parameters[InParameterIndex]))
		{
			OutError = EString("Parameter index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		Parameters[InParameterIndex]->SetStringField(PE_TEXT("Name"), InNewName);
		Root.SetObjectArrayField(PE_TEXT("Parameters"), Parameters);

		TArray<CJsonObject*> Transitions;
		const BOOL8 bHasTransitions = Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		if (bHasTransitions != FALSE)
		{
			for (INT32 TransitionIndex = 0; TransitionIndex < Transitions.Num(); TransitionIndex++)
			{
				CJsonObject* Transition = Transitions[TransitionIndex];
				if (!Transition)
				{
					continue;
				}
				TArray<CJsonObject*> Conditions;
				if (!(Transition->GetObjectArrayField(PE_TEXT("Conditions"), Conditions)))
				{
					continue;
				}
				FJsonObjectArrayOwner ConditionOwner(Conditions);
				for (INT32 ConditionIndex = 0; ConditionIndex < Conditions.Num(); ConditionIndex++)
				{
					if ((Conditions[ConditionIndex])
						&& (GetJsonObjectString(*Conditions[ConditionIndex], PE_TEXT("Param")) == OldName))
					{
						Conditions[ConditionIndex]->SetStringField(PE_TEXT("Param"), InNewName);
					}
				}
				Transition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
			}
			Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		}

		if (!CommitJsonText(Root.ToString(), OutError))
		{
			return FALSE;
		}
		if ((Selection.Is(EEditorSelectionType::PARAMETER)) && (Selection.ObjectId == OldName))
		{
			Selection.ObjectId = InNewName;
		}
		return TRUE;
	}

	BOOL32 FStateMachineDocument::UpdateParameter(INT32 InParameterIndex, UINT8 InParameterType
		, FLOAT InDefaultFloat, BOOL8 InDefaultBool, EString& OutError)
	{
		const EAnimStateMachineParameterType ParameterType =
			static_cast<EAnimStateMachineParameterType>(InParameterType);
		const EString TypeText(GetParameterTypeText(ParameterType));
		return UpdateArrayObject(PE_TEXT("Parameters"), InParameterIndex
			, [ParameterType, TypeText, InDefaultFloat, InDefaultBool](CJsonObject& InParameter)
			{
				InParameter.SetStringField(PE_TEXT("Type"), TypeText);
				if (ParameterType == ANIM_PARAM_FLOAT)
				{
					InParameter.SetFloatField(PE_TEXT("Default"), InDefaultFloat);
				}
				else
				{
					InParameter.SetBoolField(PE_TEXT("Default"), InDefaultBool);
				}
			}, OutError);
	}

	BOOL32 FStateMachineDocument::AddTransitionCondition(INT32 InTransitionIndex
		, INT32 InParameterIndex, EString& OutError)
	{
		if ((!m_DraftMachine) || (InTransitionIndex < 0)
			|| (InTransitionIndex >= m_DraftMachine->Transitions.Num())
			|| (InParameterIndex < 0) || (InParameterIndex >= m_DraftMachine->Parameters.Num()))
		{
			OutError = EString("Transition or parameter index is invalid.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= Transitions.Num())
			|| (!Transitions[InTransitionIndex]))
		{
			OutError = EString("Transition index is out of range.");
			LastError = OutError;
			return FALSE;
		}

		CJsonObject* Transition = Transitions[InTransitionIndex];
		TArray<CJsonObject*> Conditions;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		FJsonObjectArrayOwner ConditionOwner(Conditions);
		const EAnimStateMachineParameter& Parameter = m_DraftMachine->Parameters[InParameterIndex];
		EString DefaultOp;
		FLOAT DefaultValue = 0.0f;
		GetSafeConditionOpAndValue(Parameter, DefaultOp, DefaultValue);
		CJsonObject* Condition = new CJsonObject();
		Condition->SetStringField(PE_TEXT("Param"), Parameter.Name);
		Condition->SetStringField(PE_TEXT("Op"), DefaultOp);
		Condition->SetFloatField(PE_TEXT("Value"), DefaultValue);
		Conditions.Add(Condition);
		Transition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FStateMachineDocument::UpdateTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex
		, INT32 InParameterIndex, const EString& InOp, FLOAT InValue, EString& OutError)
	{
		if ((!m_DraftMachine) || (InTransitionIndex < 0)
			|| (InTransitionIndex >= m_DraftMachine->Transitions.Num())
			|| (InParameterIndex < 0) || (InParameterIndex >= m_DraftMachine->Parameters.Num()))
		{
			OutError = EString("Transition or parameter index is invalid.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= Transitions.Num())
			|| (!Transitions[InTransitionIndex]))
		{
			OutError = EString("Transition index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Transition = Transitions[InTransitionIndex];
		TArray<CJsonObject*> Conditions;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		FJsonObjectArrayOwner ConditionOwner(Conditions);
		if ((InConditionIndex < 0) || (InConditionIndex >= Conditions.Num())
			|| (!Conditions[InConditionIndex]))
		{
			OutError = EString("Condition index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Condition = Conditions[InConditionIndex];
		Condition->SetStringField(PE_TEXT("Param"), m_DraftMachine->Parameters[InParameterIndex].Name);
		Condition->SetStringField(PE_TEXT("Op"), InOp);
		Condition->SetFloatField(PE_TEXT("Value"), InValue);
		Transition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		return CommitJsonText(Root.ToString(), OutError);
	}

	BOOL32 FStateMachineDocument::RemoveTransitionCondition(INT32 InTransitionIndex
		, INT32 InConditionIndex, EString& OutError)
	{
		CJsonObject Root(DraftJson);
		if (!Root.IsValid())
		{
			OutError = EString("State machine JSON cannot be parsed.");
			LastError = OutError;
			return FALSE;
		}
		TArray<CJsonObject*> Transitions;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		FJsonObjectArrayOwner TransitionOwner(Transitions);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= Transitions.Num())
			|| (!Transitions[InTransitionIndex]))
		{
			OutError = EString("Transition index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Transition = Transitions[InTransitionIndex];
		TArray<CJsonObject*> Conditions;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		FJsonObjectArrayOwner ConditionOwner(Conditions);
		if ((InConditionIndex < 0) || (InConditionIndex >= Conditions.Num()))
		{
			OutError = EString("Condition index is out of range.");
			LastError = OutError;
			return FALSE;
		}
		CJsonObject* Removed = Conditions[InConditionIndex];
		Conditions.RemoveAt(InConditionIndex);
		delete Removed;
		Transition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		return CommitJsonText(Root.ToString(), OutError);
	}

}

#endif
