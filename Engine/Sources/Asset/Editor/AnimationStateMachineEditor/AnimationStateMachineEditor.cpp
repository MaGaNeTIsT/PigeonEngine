#include "AnimationStateMachineEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <AnimationAsset/AnimationAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Editor/EditorLogManager.h>
#include <RapidJson/Headers/JsonObject.h>
#include <cmath>
#include <cfloat>
#include "../EditorUi.h"

namespace PigeonEngine
{

	namespace
	{
		// The editor gives the author room to rewrite the JSON without reallocating the
		// ImGui buffer every frame.
		constexpr INT32 StateMachineEditorBufferHeadroom = 16 * 1024;

		const CHAR* GetGraphLoopModeLabel(const EAnimationLoopMode InLoopMode)
		{
			switch (InLoopMode)
			{
			case ANIMATION_LOOP_ONCE:
				return "Once";
			case ANIMATION_LOOP_REPEAT:
				return "Repeat";
			case ANIMATION_LOOP_PINGPONG:
				return "PingPong";
			case ANIMATION_LOOP_CLAMP:
				return "Clamp";
			default:
				return "Unknown";
			}
		}

		const CHAR* GetParameterTypeLabel(const EAnimStateMachineParameterType InType)
		{
			switch (InType)
			{
			case ANIM_PARAM_FLOAT:
				return "Float";
			case ANIM_PARAM_BOOL:
				return "Bool";
			case ANIM_PARAM_TRIGGER:
				return "Trigger";
			default:
				return "Unknown";
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
			default:
				return "float";
			}
		}

		void FillNameBuffer(TArray<CHAR>& OutBuffer, const EString& InName)
		{
			OutBuffer.SetNum(InName.Length() + 1);
			::memset(OutBuffer.GetData(), 0, static_cast<size_t>(OutBuffer.Num()));
			if (InName.Length() > 0u)
			{
				::memcpy_s(OutBuffer.GetData(), static_cast<rsize_t>(OutBuffer.Num()), *InName, InName.Length());
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

		/*
		* A newly created condition should not fire before the author has chosen a value.
		* Float uses an impossible threshold; Bool compares against the opposite of its
		* default; Trigger is false until the graph raises it.
		*/
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

		FLOAT DistanceToSegmentSquared(const ImVec2& InPoint, const ImVec2& InA, const ImVec2& InB)
		{
			const FLOAT ABx = InB.x - InA.x;
			const FLOAT ABy = InB.y - InA.y;
			const FLOAT LengthSquared = ABx * ABx + ABy * ABy;
			if (LengthSquared <= 0.0001f)
			{
				const FLOAT Dx = InPoint.x - InA.x;
				const FLOAT Dy = InPoint.y - InA.y;
				return Dx * Dx + Dy * Dy;
			}
			FLOAT T = ((InPoint.x - InA.x) * ABx + (InPoint.y - InA.y) * ABy) / LengthSquared;
			T = ((T < 0.0f) ? 0.0f : ((T > 1.0f) ? 1.0f : T));
			const FLOAT Dx = InPoint.x - (InA.x + ABx * T);
			const FLOAT Dy = InPoint.y - (InA.y + ABy * T);
			return Dx * Dx + Dy * Dy;
		}

		// GetObjectArrayField hands out heap copies; this guard is the single owner for the
		// duration of one edit. New objects added to an array are owned here too.
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
				if ((InStates[StateIndex]) && (GetJsonObjectString(*InStates[StateIndex], EString("Name")) == InName))
				{
					return TRUE;
				}
			}
			return FALSE;
		}
	}

	void EAnimStateMachineEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_AnimationAsset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
		m_NodePositions.Empty();
		m_SelectedStateIndex = 0;
		m_SelectedTransitionIndex = -1;
		m_TransitionFromIndex = -1;
		m_TransitionToIndex = 0;
	}

	void EAnimStateMachineEditor::OpenStateMachineAsset(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		if (m_Document.bDirty)
		{
			if (NormalizedAssetPath == m_OpenedAssetFullPath)
			{
				m_Document.bReloadPending = TRUE;
			}
			else
			{
				m_Document.bOpenPending = TRUE;
				m_Document.PendingOpenPath = InAssetFullPath;
			}
			return;
		}
		OpenStateMachineAssetNow(InAssetFullPath);
	}

	void EAnimStateMachineEditor::OpenStateMachineAssetNow(const EString& InAssetFullPath)
	{
		m_ShowEditor = TRUE;
		m_LastError = EString();

		const EString NormalizedAssetPath = InAssetFullPath.Replace("\\", "/");
		const EString AssetName = EPath::GetFileNameWithoutExtension(NormalizedAssetPath);
		EString AssetDirectory = EPath::GetFileFolderPath(NormalizedAssetPath);
		if ((AssetDirectory.Length() > 0u) && (AssetDirectory[AssetDirectory.LastIndex()] != '/'))
		{
			AssetDirectory += "/";
		}
		if ((AssetName.Length() <= 0u) || (AssetDirectory.Length() <= 0u))
		{
			m_LastError = EString("State machine editor cannot parse asset path: ") + NormalizedAssetPath;
			return;
		}

		const EAnimStateMachineAsset* LoadedAsset = nullptr;
		if (!(EAnimStateMachineManager::GetManagerSingleton()->LoadStateMachineAsset(AssetDirectory, AssetName, LoadedAsset)) || (!LoadedAsset))
		{
			m_LastError = EString("State machine editor failed to load asset: ") + NormalizedAssetPath;
			return;
		}

		m_Asset = LoadedAsset;
		m_OpenedAssetFullPath = NormalizedAssetPath;
		m_SelectedStateIndex = 0;
		m_SelectedTransitionIndex = -1;
		m_TransitionFromIndex = -1;
		m_TransitionToIndex = 0;
		LoadJsonBufferFromAsset();
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if (Machine)
		{
			LoadNodePositionsFromJson(*Machine);
			AutoLayoutGraph(*Machine);
			RefreshAnimationAsset(*Machine);
		}
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Loaded.");
		m_bStatusSucceeded = TRUE;
	}

	void EAnimStateMachineEditor::CaptureBaseline()
	{
		m_BaselineJson = EString();
		const EAnimStateMachine* Machine = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (Machine)
		{
			m_BaselineJson = Machine->SourceJson;
		}
	}

	BOOL32 EAnimStateMachineEditor::DiscardChanges()
	{
		if (m_Asset && (m_BaselineJson.Length() > 0u))
		{
			EString Error;
			if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, m_BaselineJson, Error))
			{
				m_LastError = Error;
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
				return FALSE;
			}
		}
		ReplaceJsonBuffer(m_BaselineJson);
		m_NodePositions.Empty();
		const EAnimStateMachine* Machine = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if (Machine)
		{
			LoadNodePositionsFromJson(*Machine);
			AutoLayoutGraph(*Machine);
			RefreshAnimationAsset(*Machine);
		}
		m_Document.bDirty = FALSE;
		m_StatusMessage = EString("Discarded unsaved changes.");
		m_bStatusSucceeded = TRUE;
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::SaveStateMachineAsset(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No state machine asset is open.");
			return FALSE;
		}
		WriteNodePositionsToJson();
		const EString JsonText(m_JsonBuffer.GetData());
		if (!(EAnimStateMachineManager::GetManagerSingleton()->SaveStateMachineAsset(m_Asset, JsonText, OutError)))
		{
			return FALSE;
		}
		m_BaselineJson = JsonText;
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::ReloadStateMachineAssetNow(EString& OutError)
	{
		OutError = EString();
		if (!m_Asset)
		{
			OutError = EString("No state machine asset is open.");
			return FALSE;
		}

		const EAnimStateMachineAsset* FreshAsset = nullptr;
		if (!(EAnimStateMachineManager::GetManagerSingleton()->ReloadStateMachineAsset(
			m_Asset->GetAssetPath(), m_Asset->GetAssetName(), FreshAsset, OutError)) || (!FreshAsset))
		{
			return FALSE;
		}

		m_Asset = FreshAsset;
		m_SelectedStateIndex = 0;
		m_SelectedTransitionIndex = -1;
		m_TransitionFromIndex = -1;
		m_TransitionToIndex = 0;
		LoadJsonBufferFromAsset();
		m_NodePositions.Empty();
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if (Machine)
		{
			LoadNodePositionsFromJson(*Machine);
			AutoLayoutGraph(*Machine);
			RefreshAnimationAsset(*Machine);
		}
		CaptureBaseline();
		m_Document.ClearPendingAction();
		m_Document.bDirty = FALSE;
		return TRUE;
	}

	void EAnimStateMachineEditor::HandleDocumentAction()
	{
		const EEditorUnsavedChoiceType Choice = DrawEditorUnsavedChangesPopup("##state_machine_editor_unsaved"
			, (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : nullptr), m_Document);
		if (Choice == EEditorUnsavedChoiceType::NONE)
		{
			return;
		}

		const BOOL8 bWasClose = m_Document.bClosePending;
		const BOOL8 bWasOpen = m_Document.bOpenPending;
		const BOOL8 bWasReload = m_Document.bReloadPending;
		const EString PendingPath = m_Document.PendingOpenPath;

		if (Choice == EEditorUnsavedChoiceType::CANCEL)
		{
			m_Document.ClearPendingAction();
			return;
		}
		if (Choice == EEditorUnsavedChoiceType::SAVE)
		{
			EString SaveError;
			if (!SaveStateMachineAsset(SaveError))
			{
				m_LastError = SaveError;
				m_StatusMessage = SaveError;
				m_bStatusSucceeded = FALSE;
				return;
			}
		}
		else
		{
			if (!DiscardChanges())
			{
				return;
			}
		}

		m_Document.ClearPendingAction();
		if (bWasClose)
		{
			m_ShowEditor = FALSE;
			return;
		}
		if (bWasReload)
		{
			EString ReloadError;
			if (!ReloadStateMachineAssetNow(ReloadError))
			{
				m_LastError = ReloadError;
				m_StatusMessage = ReloadError;
				m_bStatusSucceeded = FALSE;
			}
			else
			{
				m_StatusMessage = EString("Reloaded from disk.");
				m_bStatusSucceeded = TRUE;
			}
			return;
		}
		if (bWasOpen)
		{
			OpenStateMachineAssetNow(PendingPath);
		}
	}

	void EAnimStateMachineEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		HandleDocumentAction();
		if (!m_ShowEditor)
		{
			return;
		}

		// The editor is usually opened on the right half of the viewport. Anchor it to the
		// work area and clamp the size so old ini layouts cannot push the condition table's
		// Operator/Value columns off-screen.
		const ImGuiViewport* MainViewport = ImGui::GetMainViewport();
		const ImVec2 WorkPosition = MainViewport->WorkPos;
		const ImVec2 WorkSize = MainViewport->WorkSize;
		const FLOAT WindowWidth = EMath::Max(320.0f, EMath::Min(900.0f, WorkSize.x - 40.0f));
		const FLOAT WindowHeight = EMath::Max(360.0f, EMath::Min(820.0f, WorkSize.y - 40.0f));
		const ImVec2 WindowPosition(WorkPosition.x + WorkSize.x - WindowWidth - 20.0f
			, WorkPosition.y + 20.0f);
		ImGui::SetNextWindowSize(ImVec2(WindowWidth, WindowHeight), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(WindowPosition, ImGuiCond_FirstUseEver);
		bool bWindowOpen = true;
		ImGui::Begin("State Machine Editor", &bWindowOpen, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				if (m_Document.bDirty)
				{
					m_Document.bReloadPending = TRUE;
				}
				else
				{
					EString ReloadError;
					if (!ReloadStateMachineAssetNow(ReloadError))
					{
						m_LastError = ReloadError;
						m_StatusMessage = ReloadError;
						m_bStatusSucceeded = FALSE;
					}
					else
					{
						m_StatusMessage = EString("Reloaded from disk.");
						m_bStatusSucceeded = TRUE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				if (m_Asset)
				{
					WriteNodePositionsToJson();
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
						m_Document.bDirty = TRUE;
						m_StatusMessage = EString("Applied to the live asset. Save to write it to disk.");
						m_bStatusSucceeded = TRUE;
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					EString Error;
					if (SaveStateMachineAsset(Error))
					{
						m_LastError = EString();
						m_StatusMessage = EString("Saved.");
						m_bStatusSucceeded = TRUE;
					}
					else
					{
						m_LastError = Error;
						m_StatusMessage = Error;
						m_bStatusSucceeded = FALSE;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Reset Layout"))
			{
				m_NodePositions.Empty();
				if (m_Asset)
				{
					const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
					if (Machine)
					{
						AutoLayoutGraph(*Machine);
					}
				}
				m_Document.bDirty = TRUE;
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No state machine selected."));
			if (m_Document.bDirty)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
			}

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
			}
			if (m_StatusMessage.Length() > 0u)
			{
				const ImVec4 StatusColor = (m_bStatusSucceeded != FALSE)
					? ImVec4(0.45f, 0.85f, 0.45f, 1.0f)
					: ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
				ImGui::TextColored(StatusColor, "%s", *m_StatusMessage);
			}

			ImGui::Separator();
			if (m_Asset)
			{
				const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
				if (Machine)
				{
					ImGui::Text("Name: %s", *Machine->Name);
					ImGui::Text("Skeleton: %s", *Machine->SkeletonName);
					ImGui::Text("Animation: %s", *Machine->AnimationAssetName);
					ImGui::Text("States: %d  Parameters: %d  Transitions: %d",
						Machine->States.Num(), Machine->Parameters.Num(), Machine->Transitions.Num());

					DrawStructureControls(*Machine);
					ImGui::Separator();
					ImGui::TextDisabled("Drag nodes to move them; Apply/Save writes nodeX/nodeY into the JSON.");
					ImGui::BeginChild("StateMachineGraph", ImVec2(0.0f, 340.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
					DrawGraph(*Machine);
					ImGui::EndChild();
				}
			}

			if (m_JsonBuffer.Num() > 0)
			{
				if (ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput))
				{
					m_Document.bDirty = TRUE;
				}
			}
		}
		ImGui::End();

		if (!bWindowOpen)
		{
			if (m_Document.bDirty)
			{
				m_Document.bClosePending = TRUE;
			}
			else
			{
				m_ShowEditor = FALSE;
			}
		}
	}

	void EAnimStateMachineEditor::DrawGraph(const EAnimStateMachine& InMachine)
	{
		if (InMachine.States.Num() <= 0)
		{
			ImGui::TextDisabled("No states.");
			return;
		}

		// A state added by the JSON may not have a position yet; give it a slot before
		// drawing or letting the user drag it.
		AutoLayoutGraph(InMachine);

		// Context menu actions rebuild the live graph, so they are collected while the arrays
		// are still being walked and executed after the loop.
		enum class EGraphPendingActionType : UINT8
		{
			NONE = 0,
			ADD_STATE,
			DELETE_STATE,
			SET_ENTRY,
			ADD_TRANSITION,
			RESET_LAYOUT
		};
		EGraphPendingActionType PendingAction = EGraphPendingActionType::NONE;
		EString PendingStateName;
		EString PendingFromStateName;
		EString PendingToStateName;
		auto ExecutePendingAction = [&]() -> BOOL32
		{
			switch (PendingAction)
			{
			case EGraphPendingActionType::ADD_STATE:
				AddState(InMachine);
				return TRUE;
			case EGraphPendingActionType::DELETE_STATE:
				DeleteState(PendingStateName);
				return TRUE;
			case EGraphPendingActionType::SET_ENTRY:
				SetEntryState(PendingStateName);
				return TRUE;
			case EGraphPendingActionType::ADD_TRANSITION:
				if (AddTransition(PendingFromStateName, PendingToStateName))
				{
					const EAnimStateMachine* Rebuilt = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
					if (Rebuilt)
					{
						m_SelectedTransitionIndex = Rebuilt->Transitions.Num() - 1;
					}
				}
				return TRUE;
			case EGraphPendingActionType::RESET_LAYOUT:
				m_NodePositions.Empty();
				AutoLayoutGraph(InMachine);
				m_Document.bDirty = TRUE;
				return TRUE;
			default:
				return FALSE;
			}
		};

		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		constexpr FLOAT NodeWidth = 190.0f;
		constexpr FLOAT NodeHeight = 62.0f;
		const ImVec2 NodeSize(NodeWidth, NodeHeight);

		auto GetNodePosition = [this](const EString& InName) -> ImVec2
		{
			const ImVec2* Stored = m_NodePositions.FindValueAsPtr(InName);
			return (Stored ? *Stored : ImVec2(40.0f, 80.0f));
		};
		auto MakeScreenPosition = [&Origin](const ImVec2& InLocal) -> ImVec2
		{
			return ImVec2(Origin.x + InLocal.x, Origin.y + InLocal.y);
		};
		auto DrawArrow = [DrawList](const ImVec2& InFrom, const ImVec2& InTo, const ImU32 InColor
			, const FLOAT InThickness)
		{
			DrawList->AddLine(InFrom, InTo, InColor, InThickness);
			const FLOAT DeltaX = InTo.x - InFrom.x;
			const FLOAT DeltaY = InTo.y - InFrom.y;
			const FLOAT Length = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
			if (Length > 0.001f)
			{
				const FLOAT DirectionX = DeltaX / Length;
				const FLOAT DirectionY = DeltaY / Length;
				const ImVec2 Base(InTo.x - DirectionX * 10.0f, InTo.y - DirectionY * 10.0f);
				const ImVec2 SideA(Base.x - DirectionY * 5.0f, Base.y + DirectionX * 5.0f);
				const ImVec2 SideB(Base.x + DirectionY * 5.0f, Base.y - DirectionX * 5.0f);
				DrawList->AddTriangleFilled(InTo, SideA, SideB, InColor);
			}
		};
		struct FGraphEdgeSegment
		{
			ImVec2 A;
			ImVec2 B;
			INT32 TransitionIndex;
		};
		TArray<FGraphEdgeSegment> EdgeSegments;

		BOOL32 bHasAnyTransition = FALSE;
		for (INT32 TransitionIndex = 0; TransitionIndex < InMachine.Transitions.Num(); TransitionIndex++)
		{
			if (InMachine.Transitions[TransitionIndex].FromStateIndex < 0)
			{
				bHasAnyTransition = TRUE;
				break;
			}
		}
		const ImVec2 AnyLocal(40.0f, 12.0f);
		const ImVec2 AnySize(100.0f, 32.0f);

		// Edges first so nodes cover their endpoints.
		const ImU32 EdgeColor = IM_COL32(170, 180, 200, 190);
		for (INT32 TransitionIndex = 0; TransitionIndex < InMachine.Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = InMachine.Transitions[TransitionIndex];
			if ((Transition.ToStateIndex < 0) || (Transition.ToStateIndex >= InMachine.States.Num()))
			{
				continue;
			}
			const BOOL32 bSelectedEdge = ((TransitionIndex == m_SelectedTransitionIndex) ? TRUE : FALSE);
			const ImU32 CurrentEdgeColor = (bSelectedEdge != FALSE)
				? IM_COL32(255, 205, 90, 255) : EdgeColor;
			const FLOAT CurrentEdgeThickness = ((bSelectedEdge != FALSE) ? 3.0f : 2.0f);
			const ImVec2 ToLocal = GetNodePosition(InMachine.States[Transition.ToStateIndex].Name);
			const ImVec2 To(Origin.x + ToLocal.x, Origin.y + ToLocal.y + NodeHeight * 0.5f);

			ImVec2 From;
			if (Transition.FromStateIndex < 0)
			{
				if (!bHasAnyTransition)
				{
					continue;
				}
				From = ImVec2(Origin.x + AnyLocal.x + AnySize.x, Origin.y + AnyLocal.y + AnySize.y * 0.5f);
			}
			else
			{
				if (Transition.FromStateIndex >= InMachine.States.Num())
				{
					continue;
				}
				const ImVec2 FromLocal = GetNodePosition(InMachine.States[Transition.FromStateIndex].Name);
				if (Transition.FromStateIndex == Transition.ToStateIndex)
				{
					const ImVec2 Top(Origin.x + FromLocal.x + NodeWidth * 0.5f, Origin.y + FromLocal.y);
					const ImVec2 LoopLeft(Top.x - 18.0f, Top.y - 26.0f);
					const ImVec2 LoopRight(Top.x + 26.0f, Top.y - 26.0f);
					DrawList->AddLine(Top, LoopLeft, CurrentEdgeColor, CurrentEdgeThickness);
					DrawList->AddLine(LoopLeft, LoopRight, CurrentEdgeColor, CurrentEdgeThickness);
					DrawArrow(LoopRight, Top, CurrentEdgeColor, CurrentEdgeThickness);
					FGraphEdgeSegment TopToLoopLeft = { Top, LoopLeft, TransitionIndex };
					FGraphEdgeSegment LoopLeftToLoopRight = { LoopLeft, LoopRight, TransitionIndex };
					FGraphEdgeSegment LoopRightToTop = { LoopRight, Top, TransitionIndex };
					EdgeSegments.Add(TopToLoopLeft);
					EdgeSegments.Add(LoopLeftToLoopRight);
					EdgeSegments.Add(LoopRightToTop);
					CHAR SelfLabel[48] = "";
					::sprintf_s(SelfLabel, sizeof(SelfLabel), "p%d c%d", Transition.Priority, Transition.Conditions.Num());
					DrawList->AddText(ImVec2(LoopLeft.x, LoopLeft.y - 16.0f), CurrentEdgeColor, SelfLabel);
					continue;
				}
				From = ImVec2(Origin.x + FromLocal.x + NodeWidth, Origin.y + FromLocal.y + NodeHeight * 0.5f);
			}

			DrawArrow(From, To, CurrentEdgeColor, CurrentEdgeThickness);
			FGraphEdgeSegment FromToEdge = { From, To, TransitionIndex };
			EdgeSegments.Add(FromToEdge);
			CHAR Label[64] = "";
			::sprintf_s(Label, sizeof(Label), "p%d c%d", Transition.Priority, Transition.Conditions.Num());
			DrawList->AddText(ImVec2((From.x + To.x) * 0.5f, (From.y + To.y) * 0.5f - 14.0f), CurrentEdgeColor, Label);
		}

		if (bHasAnyTransition)
		{
			const ImVec2 AnyPosition = MakeScreenPosition(AnyLocal);
			DrawList->AddRectFilled(AnyPosition, ImVec2(AnyPosition.x + AnySize.x, AnyPosition.y + AnySize.y)
				, IM_COL32(55, 55, 70, 255), 4.0f);
			DrawList->AddRect(AnyPosition, ImVec2(AnyPosition.x + AnySize.x, AnyPosition.y + AnySize.y)
				, IM_COL32(120, 120, 150, 255), 4.0f);
			DrawList->AddText(ImVec2(AnyPosition.x + 8.0f, AnyPosition.y + 7.0f)
				, IM_COL32(210, 210, 230, 255), "Any State");
		}

		FLOAT MaxX = (bHasAnyTransition ? (AnyLocal.x + AnySize.x) : 0.0f);
		FLOAT MaxY = (bHasAnyTransition ? (AnyLocal.y + AnySize.y) : 0.0f);
		for (INT32 StateIndex = 0; StateIndex < InMachine.States.Num(); StateIndex++)
		{
			const EAnimStateMachineState& State = InMachine.States[StateIndex];
			ImVec2* StoredPosition = m_NodePositions.FindValueAsPtr(State.Name);
			const ImVec2 Local = (StoredPosition ? *StoredPosition : ImVec2(40.0f, 80.0f));
			const ImVec2 Position = MakeScreenPosition(Local);
			MaxX = EMath::Max(MaxX, Local.x + NodeWidth);
			MaxY = EMath::Max(MaxY, Local.y + NodeHeight);

			ImGui::PushID(StateIndex);
			ImGui::SetCursorScreenPos(Position);
			ImGui::InvisibleButton("##graph_node", NodeSize);
			const BOOL32 bHovered = ImGui::IsItemHovered();
			if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
			{
				m_SelectedStateIndex = StateIndex;
			}
			if (ImGui::BeginPopupContextItem("##state_menu"))
			{
				m_SelectedStateIndex = StateIndex;
				if (ImGui::MenuItem("Set as Entry", nullptr, false, (InMachine.EntryStateIndex != StateIndex)))
				{
					PendingAction = EGraphPendingActionType::SET_ENTRY;
					PendingStateName = State.Name;
				}
				if (ImGui::MenuItem("Rename"))
				{
					m_bFocusStateName = TRUE;
				}
				if (ImGui::MenuItem("Delete State", nullptr, false, (InMachine.States.Num() > 1)))
				{
					PendingAction = EGraphPendingActionType::DELETE_STATE;
					PendingStateName = State.Name;
				}
				ImGui::Separator();
				if (ImGui::BeginMenu("Create Transition From This State"))
				{
					for (INT32 TargetIndex = 0; TargetIndex < InMachine.States.Num(); TargetIndex++)
					{
						if (TargetIndex == StateIndex)
						{
							continue;
						}
						if (ImGui::MenuItem(*InMachine.States[TargetIndex].Name))
						{
							PendingAction = EGraphPendingActionType::ADD_TRANSITION;
							PendingFromStateName = State.Name;
							PendingToStateName = InMachine.States[TargetIndex].Name;
						}
					}
					ImGui::EndMenu();
				}
				if (ImGui::BeginMenu("Create Transition To This State"))
				{
					if (ImGui::MenuItem("Any"))
					{
						PendingAction = EGraphPendingActionType::ADD_TRANSITION;
						PendingFromStateName = EString();
						PendingToStateName = State.Name;
					}
					for (INT32 SourceIndex = 0; SourceIndex < InMachine.States.Num(); SourceIndex++)
					{
						if (SourceIndex == StateIndex)
						{
							continue;
						}
						if (ImGui::MenuItem(*InMachine.States[SourceIndex].Name))
						{
							PendingAction = EGraphPendingActionType::ADD_TRANSITION;
							PendingFromStateName = InMachine.States[SourceIndex].Name;
							PendingToStateName = State.Name;
						}
					}
					ImGui::EndMenu();
				}
				ImGui::EndPopup();
			}
			if ((ImGui::IsItemActive()) && (StoredPosition))
			{
				const ImVec2 MouseDelta = ImGui::GetIO().MouseDelta;
				StoredPosition->x += MouseDelta.x;
				StoredPosition->y += MouseDelta.y;
				m_Document.bDirty = TRUE;
			}

			const BOOL32 bEntry = ((InMachine.EntryStateIndex == StateIndex) ? TRUE : FALSE);
			const ImU32 BodyColor = IM_COL32(44, 46, 54, 255);
			const ImU32 HeaderColor = (bEntry ? IM_COL32(50, 120, 80, 255) : IM_COL32(55, 80, 125, 255));
			DrawList->AddRectFilled(Position, ImVec2(Position.x + NodeWidth, Position.y + NodeHeight), BodyColor, 4.0f);
			DrawList->AddRectFilled(Position, ImVec2(Position.x + NodeWidth, Position.y + 22.0f), HeaderColor, 4.0f);
			DrawList->AddRect(Position, ImVec2(Position.x + NodeWidth, Position.y + NodeHeight)
				, IM_COL32(100, 105, 120, 255), 4.0f);
			DrawList->AddText(ImVec2(Position.x + 8.0f, Position.y + 4.0f)
				, IM_COL32(235, 235, 240, 255), *State.Name);
			DrawList->AddText(ImVec2(Position.x + 8.0f, Position.y + 26.0f)
				, IM_COL32(190, 200, 210, 255), *State.ClipName);
			CHAR Detail[96] = "";
			::sprintf_s(Detail, sizeof(Detail), "%s  rate %.2f%s"
				, GetGraphLoopModeLabel(State.LoopMode), State.PlayRate, (bEntry ? "  ENTRY" : ""));
			DrawList->AddText(ImVec2(Position.x + 8.0f, Position.y + 44.0f)
				, IM_COL32(160, 170, 185, 255), Detail);

			if (bHovered)
			{
				ImGui::SetTooltip("%s\nClip: %s\nLoop: %s\nPlay rate: %.2f"
					, *State.Name, *State.ClipName, GetGraphLoopModeLabel(State.LoopMode), State.PlayRate);
			}
			ImGui::PopID();
		}

		// Edges are drawn without widgets, so they need an explicit hit test. Only handle
		// the click when no node/item consumed it; this keeps node clicks and edge clicks
		// independent without adding invisible buttons over the whole canvas.
		if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)
			&& (!ImGui::IsAnyItemHovered()) && (!ImGui::IsAnyItemActive()))
		{
			const ImVec2 MousePosition = ImGui::GetIO().MousePos;
			FLOAT BestDistanceSquared = 8.0f * 8.0f;
			INT32 BestTransitionIndex = -1;
			for (INT32 SegmentIndex = 0; SegmentIndex < EdgeSegments.Num(); SegmentIndex++)
			{
				const FGraphEdgeSegment& Segment = EdgeSegments[SegmentIndex];
				const FLOAT DistanceSquared = DistanceToSegmentSquared(MousePosition, Segment.A, Segment.B);
				if (DistanceSquared < BestDistanceSquared)
				{
					BestDistanceSquared = DistanceSquared;
					BestTransitionIndex = Segment.TransitionIndex;
				}
			}
			if (BestTransitionIndex >= 0)
			{
				m_SelectedTransitionIndex = BestTransitionIndex;
			}
		}

		if (ExecutePendingAction() != FALSE)
		{
			return;
		}

		if (ImGui::BeginPopupContextWindow("##state_machine_graph_bg"
			, ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			if (ImGui::MenuItem("Add State"))
			{
				PendingAction = EGraphPendingActionType::ADD_STATE;
			}
			if (ImGui::MenuItem("Reset Layout"))
			{
				PendingAction = EGraphPendingActionType::RESET_LAYOUT;
			}
			if ((m_SelectedStateIndex >= 0) && (m_SelectedStateIndex < InMachine.States.Num()))
			{
				const EString SelectedStateName = InMachine.States[m_SelectedStateIndex].Name;
				ImGui::Separator();
				if (ImGui::MenuItem("Create Any -> Selected State"))
				{
					PendingAction = EGraphPendingActionType::ADD_TRANSITION;
					PendingFromStateName = EString();
					PendingToStateName = SelectedStateName;
				}
			}
			ImGui::EndPopup();
		}
		if (ExecutePendingAction() != FALSE)
		{
			return;
		}

		// Reserve the content extents so the child can scroll if nodes are dragged outward.
		ImGui::SetCursorScreenPos(Origin);
		ImGui::Dummy(ImVec2(EMath::Max(MaxX + 60.0f, 320.0f), EMath::Max(MaxY + 60.0f, 300.0f)));
	}

	void EAnimStateMachineEditor::LoadNodePositionsFromJson(const EAnimStateMachine& InMachine)
	{
		m_NodePositions.Empty();
		if ((m_JsonBuffer.Num() <= 0) || (InMachine.States.Num() <= 0))
		{
			return;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			return;
		}
		TArray<CJsonObject*> StateObjects;
		Root.GetObjectArrayField(PE_TEXT("States"), StateObjects);
		FJsonObjectArrayOwner StateOwner(StateObjects);
		for (INT32 StateIndex = 0; StateIndex < StateObjects.Num(); StateIndex++)
		{
			CJsonObject* StateObject = StateObjects[StateIndex];
			if (!StateObject)
			{
				continue;
			}
			EString StateName;
			FLOAT NodeX = 0.0f;
			FLOAT NodeY = 0.0f;
			if (StateObject->GetStringField(PE_TEXT("Name"), StateName)
				&& StateObject->GetFloatField(PE_TEXT("NodeX"), NodeX)
				&& StateObject->GetFloatField(PE_TEXT("NodeY"), NodeY)
				&& (StateName.Length() > 0u))
			{
				m_NodePositions.Add(StateName, ImVec2(NodeX, NodeY));
			}
		}
	}

	void EAnimStateMachineEditor::AutoLayoutGraph(const EAnimStateMachine& InMachine)
	{
		for (INT32 StateIndex = 0; StateIndex < InMachine.States.Num(); StateIndex++)
		{
			const EString& StateName = InMachine.States[StateIndex].Name;
			if (!m_NodePositions.ContainsKey(StateName))
			{
				const FLOAT Column = static_cast<FLOAT>(StateIndex % 3);
				const FLOAT Row = static_cast<FLOAT>(StateIndex / 3);
				m_NodePositions.Add(StateName, ImVec2(40.0f + Column * 260.0f, 80.0f + Row * 150.0f));
			}
		}
	}

	void EAnimStateMachineEditor::WriteNodePositionsToJson()
	{
		if (m_JsonBuffer.Num() <= 0)
		{
			return;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			return;
		}
		TArray<CJsonObject*> StateObjects;
		Root.GetObjectArrayField(PE_TEXT("States"), StateObjects);
		FJsonObjectArrayOwner StateOwner(StateObjects);
		for (INT32 StateIndex = 0; StateIndex < StateObjects.Num(); StateIndex++)
		{
			CJsonObject* StateObject = StateObjects[StateIndex];
			if (!StateObject)
			{
				continue;
			}
			EString StateName;
			if (!StateObject->GetStringField(PE_TEXT("Name"), StateName))
			{
				continue;
			}
			const ImVec2* Position = m_NodePositions.FindValueAsPtr(StateName);
			if (!Position)
			{
				continue;
			}
			StateObject->SetFloatField(PE_TEXT("NodeX"), Position->x);
			StateObject->SetFloatField(PE_TEXT("NodeY"), Position->y);
		}
		Root.SetObjectArrayField(PE_TEXT("States"), StateObjects);
		ReplaceJsonBuffer(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::UpdateArrayObject(const EString& InFieldName, INT32 InIndex
		, const TFunction<void(CJsonObject&)>& InMutator)
	{
		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Objects;
		if (!(Root.GetObjectArrayField(InFieldName, Objects)))
		{
			m_LastError = EString(PE_TEXT("State machine JSON has no '")) + InFieldName + EString(PE_TEXT("' array."));
			return FALSE;
		}
		FJsonObjectArrayOwner ObjectOwner(Objects);
		if ((InIndex < 0) || (InIndex >= Objects.Num()) || (!Objects[InIndex]))
		{
			m_LastError = EString(PE_TEXT("State machine array index is out of range: ")) + InFieldName;
			return FALSE;
		}
		InMutator(*Objects[InIndex]);
		Root.SetObjectArrayField(InFieldName, Objects);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::UpdateParameter(INT32 InParameterIndex, UINT8 InParameterType
		, FLOAT InDefaultFloat, BOOL8 InDefaultBool)
	{
		const EAnimStateMachineParameterType ParameterType = static_cast<EAnimStateMachineParameterType>(InParameterType);
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
			});
	}

	BOOL32 EAnimStateMachineEditor::AddParameter()
	{
		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		FJsonObjectArrayOwner ParameterOwner(Parameters);

		INT32 Suffix = Parameters.Num();
		EString NewName;
		BOOL32 bNameExists = TRUE;
		while (bNameExists != FALSE)
		{
			bNameExists = FALSE;
			NewName = EString(PE_TEXT("Param")) + ToString(Suffix++);
			for (INT32 ParameterIndex = 0; ParameterIndex < Parameters.Num(); ParameterIndex++)
			{
				if ((Parameters[ParameterIndex]) && (GetJsonObjectString(*Parameters[ParameterIndex], PE_TEXT("Name")) == NewName))
				{
					bNameExists = TRUE;
					break;
				}
			}
		}

		CJsonObject* NewParameter = new CJsonObject();
		NewParameter->SetStringField(PE_TEXT("Name"), NewName);
		NewParameter->SetStringField(PE_TEXT("Type"), EString(PE_TEXT("float")));
		NewParameter->SetFloatField(PE_TEXT("Default"), 0.0f);
		Parameters.Add(NewParameter);
		Root.SetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::RemoveParameter(INT32 InParameterIndex)
	{
		const EAnimStateMachine* Machine = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if ((!Machine) || (InParameterIndex < 0) || (InParameterIndex >= Machine->Parameters.Num()))
		{
			return FALSE;
		}
		const EString& ParameterName = Machine->Parameters[InParameterIndex].Name;
		for (INT32 TransitionIndex = 0; TransitionIndex < Machine->Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = Machine->Transitions[TransitionIndex];
			for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
			{
				const INT32 ParameterRef = Transition.Conditions[ConditionIndex].ParameterIndex;
				if ((ParameterRef >= 0) && (ParameterRef < Machine->Parameters.Num())
					&& (Machine->Parameters[ParameterRef].Name == ParameterName))
				{
					m_LastError = EString(PE_TEXT("Parameter '")) + ParameterName
						+ EString(PE_TEXT("' is used by a transition condition."));
					return FALSE;
				}
			}
		}

		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		if (!(Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters)))
		{
			return FALSE;
		}
		FJsonObjectArrayOwner ParameterOwner(Parameters);
		if ((InParameterIndex < 0) || (InParameterIndex >= Parameters.Num()) || (!Parameters[InParameterIndex]))
		{
			return FALSE;
		}
		delete Parameters[InParameterIndex];
		Parameters.RemoveAt(InParameterIndex);
		Root.SetObjectArrayField(PE_TEXT("Parameters"), Parameters);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::UpdateState(INT32 InStateIndex, const EString& InClipName
		, EAnimationLoopMode InLoopMode, FLOAT InPlayRate, FLOAT InBlendSeconds)
	{
		const EString LoopText(GetGraphLoopModeLabel(InLoopMode));
		return UpdateArrayObject(PE_TEXT("States"), InStateIndex
			, [InClipName, LoopText, InPlayRate, InBlendSeconds](CJsonObject& InState)
			{
				InState.SetStringField(PE_TEXT("Clip"), InClipName);
				InState.SetStringField(PE_TEXT("Loop"), LoopText);
				InState.SetFloatField(PE_TEXT("Rate"), InPlayRate);
				InState.SetFloatField(PE_TEXT("Blend"), InBlendSeconds);
			});
	}

	BOOL32 EAnimStateMachineEditor::RenameState(const EString& InOldName, const EString& InNewName)
	{
		if (InNewName.Length() <= 0u)
		{
			m_LastError = EString("State name cannot be empty.");
			return FALSE;
		}
		if (InOldName == InNewName)
		{
			return TRUE;
		}

		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> States;
		if (!(Root.GetObjectArrayField(PE_TEXT("States"), States)))
		{
			m_LastError = EString("State machine JSON has no states array.");
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
				m_LastError = EString(PE_TEXT("State '")) + InNewName + EString(PE_TEXT("' already exists."));
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
			m_LastError = EString(PE_TEXT("State '")) + InOldName + EString(PE_TEXT("' was not found."));
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

		if (!CommitJsonText(Root.ToString()))
		{
			return FALSE;
		}
		ImVec2* StoredPosition = m_NodePositions.FindValueAsPtr(InOldName);
		if (StoredPosition)
		{
			const ImVec2 Position = *StoredPosition;
			m_NodePositions.Remove(InOldName);
			m_NodePositions.Add(InNewName, Position);
		}
		m_StateNameBufferIndex = -1;
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::RenameParameter(INT32 InParameterIndex, const EString& InNewName)
	{
		const EAnimStateMachine* Machine = (m_Asset ? m_Asset->GetStoragedResource() : nullptr);
		if ((!Machine) || (InParameterIndex < 0) || (InParameterIndex >= Machine->Parameters.Num()))
		{
			return FALSE;
		}
		if (InNewName.Length() <= 0u)
		{
			m_LastError = EString("Parameter name cannot be empty.");
			return FALSE;
		}
		const EString OldName = Machine->Parameters[InParameterIndex].Name;
		if (OldName == InNewName)
		{
			return TRUE;
		}
		for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
		{
			if ((ParameterIndex != InParameterIndex) && (Machine->Parameters[ParameterIndex].Name == InNewName))
			{
				m_LastError = EString(PE_TEXT("Parameter '")) + InNewName + EString(PE_TEXT("' already exists."));
				return FALSE;
			}
		}

		CJsonObject Root(EString(m_JsonBuffer.GetData()));
		if (!Root.IsValid())
		{
			m_LastError = EString("State machine JSON is not a valid object.");
			return FALSE;
		}
		TArray<CJsonObject*> Parameters;
		if (!(Root.GetObjectArrayField(PE_TEXT("Parameters"), Parameters)))
		{
			m_LastError = EString("State machine JSON has no parameters array.");
			return FALSE;
		}
		FJsonObjectArrayOwner ParameterOwner(Parameters);
		if ((InParameterIndex >= Parameters.Num()) || (!Parameters[InParameterIndex]))
		{
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
					if ((Conditions[ConditionIndex]) && (GetJsonObjectString(*Conditions[ConditionIndex], PE_TEXT("Param")) == OldName))
					{
						Conditions[ConditionIndex]->SetStringField(PE_TEXT("Param"), InNewName);
					}
				}
				Transition->SetObjectArrayField(PE_TEXT("Conditions"), Conditions);
			}
			Root.SetObjectArrayField(PE_TEXT("Transitions"), Transitions);
		}

		if (!CommitJsonText(Root.ToString()))
		{
			return FALSE;
		}
		m_ParameterNameBuffers.Empty();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::UpdateTransition(INT32 InTransitionIndex, INT32 InPriority, FLOAT InBlendSeconds
		, FLOAT InMinTimeInState, BOOL8 bInCanTransitionToSelf)
	{
		return UpdateArrayObject(PE_TEXT("Transitions"), InTransitionIndex
			, [InPriority, InBlendSeconds, InMinTimeInState, bInCanTransitionToSelf](CJsonObject& InTransition)
			{
				InTransition.SetIntField(PE_TEXT("Priority"), InPriority);
				InTransition.SetFloatField(PE_TEXT("Blend"), InBlendSeconds);
				InTransition.SetFloatField(PE_TEXT("MinTime"), InMinTimeInState);
				InTransition.SetBoolField(PE_TEXT("CanSelf"), bInCanTransitionToSelf);
			});
	}

	void EAnimStateMachineEditor::RefreshAnimationAsset(const EAnimStateMachine& InMachine)
	{
		m_AnimationAsset = nullptr;
		if (InMachine.AnimationAssetName.Length() <= 0u)
		{
			return;
		}
		const ESkeletonAnimationAsset* LoadedAnimation = nullptr;
		if (EAnimationManager::GetManagerSingleton()->LoadSkeletonAnimationAsset(
			EBaseSettings::ENGINE_ANIMATION_PATH, InMachine.AnimationAssetName, LoadedAnimation))
		{
			m_AnimationAsset = LoadedAnimation;
		}
	}

	BOOL32 EAnimStateMachineEditor::DrawParameterControls(const EAnimStateMachine& InMachine)
	{
		if (!ImGui::TreeNodeEx("Parameters", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return FALSE;
		}
		if (ImGui::Button("Add Parameter"))
		{
			if (AddParameter())
			{
				ImGui::TreePop();
				return TRUE;
			}
		}
		if (InMachine.Parameters.Num() <= 0)
		{
			ImGui::TextDisabled("No parameters.");
			ImGui::TreePop();
			return FALSE;
		}

		if (m_ParameterNameBuffers.Num() != InMachine.Parameters.Num())
		{
			m_ParameterNameBuffers.SetNum(InMachine.Parameters.Num());
			for (INT32 BufferIndex = 0; BufferIndex < InMachine.Parameters.Num(); BufferIndex++)
			{
				FillNameBuffer(m_ParameterNameBuffers[BufferIndex], InMachine.Parameters[BufferIndex].Name);
			}
		}
		ImGui::TextDisabled("Edit a name and press Enter to rename.");

		const EAnimStateMachineParameterType ParameterTypes[3] = {
			ANIM_PARAM_FLOAT,
			ANIM_PARAM_BOOL,
			ANIM_PARAM_TRIGGER
		};
		INT32 PendingRemoveParameterIndex = -1;
		for (INT32 ParameterIndex = 0; ParameterIndex < InMachine.Parameters.Num(); ParameterIndex++)
		{
			const EAnimStateMachineParameter& Parameter = InMachine.Parameters[ParameterIndex];
			ImGui::PushID(ParameterIndex);
			ImGui::Separator();
			ImGui::SetNextItemWidth(180.0f);
			TArray<CHAR>& NameBuffer = m_ParameterNameBuffers[ParameterIndex];
			if (m_FocusParameterIndex == ParameterIndex)
			{
				ImGui::SetKeyboardFocusHere();
				m_FocusParameterIndex = -1;
			}
			ImGui::InputText("Name", NameBuffer.GetData(), static_cast<int>(NameBuffer.Num()));
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				const EString NewName(NameBuffer.GetData());
				if (RenameParameter(ParameterIndex, NewName))
				{
					ImGui::PopID();
					ImGui::TreePop();
					return TRUE;
				}
			}
			if (ImGui::BeginPopupContextItem("##parameter_menu"))
			{
				if (ImGui::MenuItem("Rename"))
				{
					m_FocusParameterIndex = ParameterIndex;
				}
				if (ImGui::MenuItem("Remove Parameter"))
				{
					PendingRemoveParameterIndex = ParameterIndex;
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginCombo("Type", GetParameterTypeLabel(Parameter.Type)))
			{
				for (UINT32 TypeIndex = 0u; TypeIndex < 3u; TypeIndex++)
				{
					const BOOL32 bSelected = (Parameter.Type == ParameterTypes[TypeIndex]);
					if (ImGui::Selectable(GetParameterTypeLabel(ParameterTypes[TypeIndex]), bSelected))
					{
						if (UpdateParameter(ParameterIndex, static_cast<UINT8>(ParameterTypes[TypeIndex]), 0.0f, FALSE))
						{
							ImGui::EndCombo();
							ImGui::PopID();
							ImGui::TreePop();
							return TRUE;
						}
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			if (Parameter.Type == ANIM_PARAM_FLOAT)
			{
				FLOAT DefaultValue = Parameter.DefaultFloat;
				ImGui::SetNextItemWidth(120.0f);
				ImGui::InputFloat("Default", &DefaultValue, 0.0f, 0.0f, "%.3f");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					if (UpdateParameter(ParameterIndex, static_cast<UINT8>(Parameter.Type), DefaultValue, Parameter.DefaultBool))
					{
						ImGui::PopID();
						ImGui::TreePop();
						return TRUE;
					}
				}
			}
			else if (Parameter.Type == ANIM_PARAM_BOOL)
			{
				bool bDefaultValue = (Parameter.DefaultBool != FALSE);
				if (ImGui::Checkbox("Default", &bDefaultValue))
				{
					if (UpdateParameter(ParameterIndex, static_cast<UINT8>(Parameter.Type), Parameter.DefaultFloat
						, (bDefaultValue ? TRUE : FALSE)))
					{
						ImGui::PopID();
						ImGui::TreePop();
						return TRUE;
					}
				}
			}

			if (ImGui::Button("Remove Parameter"))
			{
				if (RemoveParameter(ParameterIndex))
				{
					ImGui::PopID();
					ImGui::TreePop();
					return TRUE;
				}
			}
			ImGui::PopID();
		}
		if (PendingRemoveParameterIndex >= 0)
		{
			if (RemoveParameter(PendingRemoveParameterIndex))
			{
				ImGui::TreePop();
				return TRUE;
			}
		}
		ImGui::TreePop();
		return FALSE;
	}

	BOOL32 EAnimStateMachineEditor::DrawStateProperties(const EAnimStateMachine& InMachine)
	{
		if ((m_SelectedStateIndex < 0) || (m_SelectedStateIndex >= InMachine.States.Num()))
		{
			return FALSE;
		}
		FImGuiIdScope StatePropertiesScope("StateProperties");
		const EAnimStateMachineState& State = InMachine.States[m_SelectedStateIndex];
		ImGui::Separator();
		if ((m_StateNameBufferIndex != m_SelectedStateIndex) || (m_StateNameBuffer.Num() <= 0))
		{
			FillNameBuffer(m_StateNameBuffer, State.Name);
			m_StateNameBufferIndex = m_SelectedStateIndex;
		}
		if (m_bFocusStateName != FALSE)
		{
			ImGui::SetKeyboardFocusHere();
			m_bFocusStateName = FALSE;
		}
		ImGui::SetNextItemWidth(200.0f);
		ImGui::InputText("Name", m_StateNameBuffer.GetData(), static_cast<int>(m_StateNameBuffer.Num()));
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			const EString NewName(m_StateNameBuffer.GetData());
			if (RenameState(State.Name, NewName))
			{
				return TRUE;
			}
		}
		ImGui::TextDisabled("State '%s'", *State.Name);

		const CHAR* CurrentClipLabel = ((State.ClipName.Length() > 0u) ? *State.ClipName : "None");
		if (ImGui::BeginCombo("Clip", CurrentClipLabel))
		{
			const ESkeletonAnimation* Animation = (m_AnimationAsset ? m_AnimationAsset->GetStoragedResource() : nullptr);
			if (Animation)
			{
				for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
				{
					const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
					if (!Clip)
					{
						continue;
					}
					const BOOL32 bSelected = (State.ClipName == Clip->ClipName);
					if (ImGui::Selectable(*Clip->ClipName, bSelected))
					{
						if (UpdateState(m_SelectedStateIndex, Clip->ClipName, State.LoopMode, State.PlayRate, State.BlendSeconds))
						{
							ImGui::EndCombo();
							return TRUE;
						}
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			}
			else
			{
				ImGui::TextDisabled("Animation asset '%s' is not loaded.", *InMachine.AnimationAssetName);
			}
			ImGui::EndCombo();
		}

		const EAnimationLoopMode LoopModes[4] = {
			EAnimationLoopMode::ANIMATION_LOOP_ONCE,
			EAnimationLoopMode::ANIMATION_LOOP_REPEAT,
			EAnimationLoopMode::ANIMATION_LOOP_PINGPONG,
			EAnimationLoopMode::ANIMATION_LOOP_CLAMP
		};
		if (ImGui::BeginCombo("Loop", GetGraphLoopModeLabel(State.LoopMode)))
		{
			for (UINT32 ModeIndex = 0u; ModeIndex < 4u; ModeIndex++)
			{
				const BOOL32 bSelected = (State.LoopMode == LoopModes[ModeIndex]);
				if (ImGui::Selectable(GetGraphLoopModeLabel(LoopModes[ModeIndex]), bSelected))
				{
					if (UpdateState(m_SelectedStateIndex, State.ClipName, LoopModes[ModeIndex], State.PlayRate, State.BlendSeconds))
					{
						ImGui::EndCombo();
						return TRUE;
					}
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		FLOAT PlayRate = State.PlayRate;
		if (ImGui::SliderFloat("Rate", &PlayRate, 0.01f, 4.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp))
		{
			if (UpdateState(m_SelectedStateIndex, State.ClipName, State.LoopMode, PlayRate, State.BlendSeconds))
			{
				return TRUE;
			}
		}
		FLOAT BlendSeconds = State.BlendSeconds;
		if (ImGui::SliderFloat("Blend##state", &BlendSeconds, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			if (UpdateState(m_SelectedStateIndex, State.ClipName, State.LoopMode, State.PlayRate, BlendSeconds))
			{
				return TRUE;
			}
		}
		return FALSE;
	}

	void EAnimStateMachineEditor::DrawStructureControls(const EAnimStateMachine& InMachine)
	{
		if (DrawParameterControls(InMachine))
		{
			return;
		}
		if (InMachine.States.Num() <= 0)
		{
			ImGui::TextDisabled("No states; fix the JSON before adding one.");
			return;
		}

		m_SelectedStateIndex = EMath::Clamp(m_SelectedStateIndex, 0, InMachine.States.Num() - 1);
		const EString& SelectedStateName = InMachine.States[m_SelectedStateIndex].Name;

		if (ImGui::Button("Add State"))
		{
			AddState(InMachine);
			// The live States array may have been rebuilt; stop using this frame's
			// references and redraw from the new graph next frame.
			return;
		}
		ImGui::SameLine();
		if (ImGui::BeginCombo("Selected State", *SelectedStateName))
		{
			for (INT32 StateIndex = 0; StateIndex < InMachine.States.Num(); StateIndex++)
			{
				const BOOL8 bSelected = (StateIndex == m_SelectedStateIndex);
				if (ImGui::Selectable(*InMachine.States[StateIndex].Name, bSelected))
				{
					m_SelectedStateIndex = StateIndex;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Set Entry"))
		{
			SetEntryState(SelectedStateName);
			return;
		}
		ImGui::SameLine();
		if (ImGui::Button("Delete State"))
		{
			if (InMachine.States.Num() <= 1)
			{
				m_LastError = EString("Cannot delete the last state.");
			}
			else
			{
				DeleteState(SelectedStateName);
			}
			return;
		}

		if (DrawStateProperties(InMachine))
		{
			return;
		}

		ImGui::Separator();
		ImGui::TextDisabled("Create Transition: choose From (Any allowed) and To.");
		m_TransitionFromIndex = EMath::Clamp(m_TransitionFromIndex, -1, InMachine.States.Num() - 1);
		m_TransitionToIndex = EMath::Clamp(m_TransitionToIndex, 0, InMachine.States.Num() - 1);
		const EString FromLabel = ((m_TransitionFromIndex < 0) ? EString("Any") : InMachine.States[m_TransitionFromIndex].Name);
		const EString& ToLabel = InMachine.States[m_TransitionToIndex].Name;
		BOOL32 bAddTransition = FALSE;
		if (ImGui::BeginTable("##create_transition", 3
			, ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings))
		{
			ImGui::TableSetupColumn("From", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("To", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 130.0f);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##From", *FromLabel))
			{
				const BOOL8 bAnySelected = (m_TransitionFromIndex < 0);
				if (ImGui::Selectable("Any", bAnySelected))
				{
					m_TransitionFromIndex = -1;
				}
				if (bAnySelected)
				{
					ImGui::SetItemDefaultFocus();
				}
				for (INT32 StateIndex = 0; StateIndex < InMachine.States.Num(); StateIndex++)
				{
					const BOOL8 bSelected = (StateIndex == m_TransitionFromIndex);
					if (ImGui::Selectable(*InMachine.States[StateIndex].Name, bSelected))
					{
						m_TransitionFromIndex = StateIndex;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::TableSetColumnIndex(1);
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##To", *ToLabel))
			{
				for (INT32 StateIndex = 0; StateIndex < InMachine.States.Num(); StateIndex++)
				{
					const BOOL8 bSelected = (StateIndex == m_TransitionToIndex);
					if (ImGui::Selectable(*InMachine.States[StateIndex].Name, bSelected))
					{
						m_TransitionToIndex = StateIndex;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			ImGui::TableSetColumnIndex(2);
			if (ImGui::Button("Add Transition", ImVec2(-FLT_MIN, 0.0f)))
			{
				bAddTransition = TRUE;
			}
			ImGui::EndTable();
		}
		if (bAddTransition != FALSE)
		{
			const EString FromName = ((m_TransitionFromIndex < 0) ? EString() : InMachine.States[m_TransitionFromIndex].Name);
			AddTransition(FromName, InMachine.States[m_TransitionToIndex].Name);
			return;
		}

		ImGui::BeginChild("TransitionList", ImVec2(0.0f, 100.0f), true);
		INT32 RemoveTransitionIndex = -1;
		for (INT32 TransitionIndex = 0; TransitionIndex < InMachine.Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = InMachine.Transitions[TransitionIndex];
			const EString TransitionFrom = ((Transition.FromStateIndex < 0)
				? EString("Any") : InMachine.States[Transition.FromStateIndex].Name);
			const EString TransitionTo = ((Transition.ToStateIndex >= 0) && (Transition.ToStateIndex < InMachine.States.Num())
				? InMachine.States[Transition.ToStateIndex].Name : EString("?"));
			ImGui::PushID(TransitionIndex);
			ImGui::Text("%s -> %s  | p%d | cond %d", *TransitionFrom, *TransitionTo
				, Transition.Priority, Transition.Conditions.Num());
			if (ImGui::BeginPopupContextItem("##transition_menu"))
			{
				if (ImGui::MenuItem("Select"))
				{
					m_SelectedTransitionIndex = TransitionIndex;
				}
				if (ImGui::MenuItem("Delete"))
				{
					RemoveTransitionIndex = TransitionIndex;
				}
				ImGui::EndPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Select"))
			{
				m_SelectedTransitionIndex = TransitionIndex;
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
			{
				RemoveTransitionIndex = TransitionIndex;
			}
			ImGui::PopID();
		}
		ImGui::EndChild();
		if (RemoveTransitionIndex >= 0)
		{
			DeleteTransition(RemoveTransitionIndex);
			m_SelectedTransitionIndex = -1;
			return;
		}

		DrawTransitionConditions(InMachine);
	}

	void EAnimStateMachineEditor::DrawTransitionConditions(const EAnimStateMachine& InMachine)
	{
		if (InMachine.Transitions.Num() <= 0)
		{
			return;
		}
		m_SelectedTransitionIndex = EMath::Clamp(m_SelectedTransitionIndex, -1, InMachine.Transitions.Num() - 1);
		if (m_SelectedTransitionIndex < 0)
		{
			ImGui::TextDisabled("Select a transition above or click an edge in the graph to edit its conditions.");
			return;
		}

		FImGuiIdScope TransitionPropertiesScope("TransitionProperties");
		const EAnimStateMachineTransition& Transition = InMachine.Transitions[m_SelectedTransitionIndex];
		const EString FromName = ((Transition.FromStateIndex < 0)
			? EString("Any") : InMachine.States[Transition.FromStateIndex].Name);
		const EString ToName = ((Transition.ToStateIndex >= 0) && (Transition.ToStateIndex < InMachine.States.Num())
			? InMachine.States[Transition.ToStateIndex].Name : EString("?"));
		ImGui::Separator();
		ImGui::Text("Conditions: %s -> %s", *FromName, *ToName);
		INT32 Priority = Transition.Priority;
		ImGui::SetNextItemWidth(120.0f);
		ImGui::InputInt("Priority", &Priority);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			if (UpdateTransition(m_SelectedTransitionIndex, Priority, Transition.BlendSeconds
				, Transition.MinTimeInState, Transition.bCanTransitionToSelf))
			{
				return;
			}
		}
		FLOAT BlendSeconds = Transition.BlendSeconds;
		if (ImGui::SliderFloat("Blend##transition", &BlendSeconds, 0.0f, 2.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			if (UpdateTransition(m_SelectedTransitionIndex, Transition.Priority, BlendSeconds
				, Transition.MinTimeInState, Transition.bCanTransitionToSelf))
			{
				return;
			}
		}
		FLOAT MinTimeInState = Transition.MinTimeInState;
		if (ImGui::SliderFloat("Min Time", &MinTimeInState, 0.0f, 5.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			if (UpdateTransition(m_SelectedTransitionIndex, Transition.Priority, Transition.BlendSeconds
				, MinTimeInState, Transition.bCanTransitionToSelf))
			{
				return;
			}
		}
		bool bCanTransitionToSelf = (Transition.bCanTransitionToSelf != FALSE);
		if (ImGui::Checkbox("Can Transition To Self", &bCanTransitionToSelf))
		{
			if (UpdateTransition(m_SelectedTransitionIndex, Transition.Priority, Transition.BlendSeconds
				, Transition.MinTimeInState, (bCanTransitionToSelf ? TRUE : FALSE)))
			{
				return;
			}
		}

		if (InMachine.Parameters.Num() <= 0)
		{
			ImGui::TextDisabled("This machine has no parameters; add a parameter before creating conditions.");
		}
		else if (Transition.Conditions.Num() <= 0)
		{
			ImGui::TextDisabled("No conditions: this transition is unconditional.");
		}

		// A condition edit rebuilds the live machine, so collect exactly one pending action
		// while drawing and execute it after the table. This keeps every row readable in the
		// same frame without touching stale Transition/Condition references.
		enum class EConditionPendingActionType : UINT8
		{
			NONE = 0,
			UPDATE,
			REMOVE,
			ADD
		};
		EConditionPendingActionType PendingAction = EConditionPendingActionType::NONE;
		INT32 PendingConditionIndex = -1;
		INT32 PendingParameterIndex = -1;
		EString PendingOp;
		FLOAT PendingValue = 0.0f;

		if (InMachine.Parameters.Num() > 0)
		{
			const ImGuiTableFlags ConditionTableFlags = ImGuiTableFlags_SizingStretchProp
				| ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoSavedSettings;
			if (ImGui::BeginTable("##conditions", 4, ConditionTableFlags))
			{
				ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Operator", ImGuiTableColumnFlags_WidthFixed, 90.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 120.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 76.0f);
				ImGui::TableHeadersRow();

				for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
				{
					const EAnimStateMachineCondition& Condition = Transition.Conditions[ConditionIndex];
					const EAnimStateMachineParameter* Parameter =
						(((Condition.ParameterIndex >= 0) && (Condition.ParameterIndex < InMachine.Parameters.Num()))
							? &InMachine.Parameters[Condition.ParameterIndex] : nullptr);
					const EString ParameterName = (Parameter ? Parameter->Name : EString("None"));
					const EAnimStateMachineParameterType ParameterType =
						(Parameter ? Parameter->Type : ANIM_PARAM_FLOAT);
					const CHAR* OpText = GetCompareOpText(Condition.Compare);
					ImGui::PushID(ConditionIndex);

					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##Param", *ParameterName))
					{
						for (INT32 ParameterIndex = 0; ParameterIndex < InMachine.Parameters.Num(); ParameterIndex++)
						{
							ImGui::PushID(ParameterIndex);
							const BOOL8 bSelected = (ParameterIndex == Condition.ParameterIndex);
							if (ImGui::Selectable(*InMachine.Parameters[ParameterIndex].Name, bSelected)
								&& (PendingAction == EConditionPendingActionType::NONE))
							{
								const EAnimStateMachineParameter& NewParameter = InMachine.Parameters[ParameterIndex];
								PendingAction = EConditionPendingActionType::UPDATE;
								PendingConditionIndex = ConditionIndex;
								PendingParameterIndex = ParameterIndex;
								if ((Parameter) && (Parameter->Type == NewParameter.Type)
									&& (IsCompareTypeValidForParameter(NewParameter.Type, Condition.Compare) != FALSE))
								{
									PendingOp = EString(OpText);
									PendingValue = Condition.Value;
								}
								else
								{
									GetSafeConditionOpAndValue(NewParameter, PendingOp, PendingValue);
								}
							}
							if (bSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
							ImGui::PopID();
						}
						ImGui::EndCombo();
					}

					ImGui::TableSetColumnIndex(1);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##Op", OpText))
					{
						const CHAR* CompareOps[6] = { ">", ">=", "<", "<=", "==", "!=" };
						const EAnimStateMachineCompareType CompareTypes[6] = {
							ANIM_COMPARE_GREATER,
							ANIM_COMPARE_GREATER_EQUAL,
							ANIM_COMPARE_LESS,
							ANIM_COMPARE_LESS_EQUAL,
							ANIM_COMPARE_EQUAL,
							ANIM_COMPARE_NOT_EQUAL
						};
						const CHAR* ParameterTypeOps[2] = { "==", "!=" };
						const EAnimStateMachineCompareType ParameterTypeCompares[2] = {
							ANIM_COMPARE_EQUAL,
							ANIM_COMPARE_NOT_EQUAL
						};
						const CHAR* TriggerOp = "trigger";
						const EAnimStateMachineCompareType TriggerCompare = ANIM_COMPARE_TRIGGER;
						INT32 CompareOpCount = 6;
						const CHAR** VisibleCompareOps = CompareOps;
						const EAnimStateMachineCompareType* VisibleCompareTypes = CompareTypes;
						if (ParameterType == ANIM_PARAM_BOOL)
						{
							CompareOpCount = 2;
							VisibleCompareOps = ParameterTypeOps;
							VisibleCompareTypes = ParameterTypeCompares;
						}
						else if (ParameterType == ANIM_PARAM_TRIGGER)
						{
							CompareOpCount = 1;
							VisibleCompareOps = &TriggerOp;
							VisibleCompareTypes = &TriggerCompare;
						}
						for (INT32 CompareOpIndex = 0; CompareOpIndex < CompareOpCount; CompareOpIndex++)
						{
							const BOOL8 bSelected = (Condition.Compare == VisibleCompareTypes[CompareOpIndex]);
							if (ImGui::Selectable(VisibleCompareOps[CompareOpIndex], bSelected)
								&& (PendingAction == EConditionPendingActionType::NONE))
							{
								PendingAction = EConditionPendingActionType::UPDATE;
								PendingConditionIndex = ConditionIndex;
								PendingParameterIndex = Condition.ParameterIndex;
								PendingOp = EString(VisibleCompareOps[CompareOpIndex]);
								PendingValue = Condition.Value;
							}
							if (bSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::TableSetColumnIndex(2);
					if (ParameterType == ANIM_PARAM_FLOAT)
					{
						FLOAT Value = Condition.Value;
						ImGui::SetNextItemWidth(-FLT_MIN);
						ImGui::InputFloat("##Value", &Value, 0.0f, 0.0f, "%.3f");
						if ((ImGui::IsItemDeactivatedAfterEdit() != false)
							&& (PendingAction == EConditionPendingActionType::NONE))
						{
							PendingAction = EConditionPendingActionType::UPDATE;
							PendingConditionIndex = ConditionIndex;
							PendingParameterIndex = Condition.ParameterIndex;
							PendingOp = EString(OpText);
							PendingValue = Value;
						}
					}
					else if (ParameterType == ANIM_PARAM_BOOL)
					{
						bool bValue = (Condition.Value != 0.0f);
						if ((ImGui::Checkbox("##Value", &bValue) != false)
							&& (PendingAction == EConditionPendingActionType::NONE))
						{
							PendingAction = EConditionPendingActionType::UPDATE;
							PendingConditionIndex = ConditionIndex;
							PendingParameterIndex = Condition.ParameterIndex;
							PendingOp = EString(OpText);
							PendingValue = (bValue ? 1.0f : 0.0f);
						}
					}
					else
					{
						ImGui::TextDisabled("(trigger)");
					}

					ImGui::TableSetColumnIndex(3);
					if ((ImGui::Button("Remove", ImVec2(-FLT_MIN, 0.0f)) != false)
						&& (PendingAction == EConditionPendingActionType::NONE))
					{
						PendingAction = EConditionPendingActionType::REMOVE;
						PendingConditionIndex = ConditionIndex;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}

		if (InMachine.Parameters.Num() > 0)
		{
			if (ImGui::Button("Add Condition"))
			{
				ImGui::OpenPopup("##add_condition_popup");
			}
			if (ImGui::BeginPopup("##add_condition_popup"))
			{
				ImGui::TextDisabled("Choose a parameter to add:");
				for (INT32 ParameterIndex = 0; ParameterIndex < InMachine.Parameters.Num(); ParameterIndex++)
				{
					ImGui::PushID(ParameterIndex);
					if (ImGui::MenuItem(*InMachine.Parameters[ParameterIndex].Name)
						&& (PendingAction == EConditionPendingActionType::NONE))
					{
						PendingAction = EConditionPendingActionType::ADD;
						PendingParameterIndex = ParameterIndex;
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}
		}
		else
		{
			ImGui::BeginDisabled();
			ImGui::Button("Add Condition");
			ImGui::EndDisabled();
			if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			{
				ImGui::SetTooltip("Add a parameter first.");
			}
		}

		switch (PendingAction)
		{
		case EConditionPendingActionType::UPDATE:
			UpdateTransitionCondition(m_SelectedTransitionIndex, PendingConditionIndex
				, PendingParameterIndex, PendingOp, PendingValue);
			break;
		case EConditionPendingActionType::REMOVE:
			RemoveTransitionCondition(m_SelectedTransitionIndex, PendingConditionIndex);
			break;
		case EConditionPendingActionType::ADD:
			AddTransitionCondition(m_SelectedTransitionIndex, PendingParameterIndex);
			break;
		default:
			break;
		}
	}

	BOOL32 EAnimStateMachineEditor::AddTransitionCondition(INT32 InTransitionIndex, INT32 InParameterIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if ((!Machine) || (InTransitionIndex < 0) || (InTransitionIndex >= Machine->Transitions.Num())
			|| (InParameterIndex < 0) || (InParameterIndex >= Machine->Parameters.Num()))
		{
			m_LastError = EString("Transition or parameter index is invalid.");
			return FALSE;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= TransitionObjects.Num()) || (!TransitionObjects[InTransitionIndex]))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}

		CJsonObject* Transition = TransitionObjects[InTransitionIndex];
		TArray<CJsonObject*> ConditionObjects;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		FJsonObjectArrayOwner ConditionOwner(ConditionObjects);
		const EAnimStateMachineParameter& Parameter = Machine->Parameters[InParameterIndex];
		EString DefaultOp;
		FLOAT DefaultValue = 0.0f;
		GetSafeConditionOpAndValue(Parameter, DefaultOp, DefaultValue);
		CJsonObject* Condition = new CJsonObject();
		Condition->SetStringField(PE_TEXT("Param"), Parameter.Name);
		Condition->SetStringField(PE_TEXT("Op"), DefaultOp);
		Condition->SetFloatField(PE_TEXT("Value"), DefaultValue);
		ConditionObjects.Add(Condition);

		Transition->SetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::UpdateTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex
		, INT32 InParameterIndex, const EString& InOp, FLOAT InValue)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if ((!Machine) || (InTransitionIndex < 0) || (InTransitionIndex >= Machine->Transitions.Num())
			|| (InParameterIndex < 0) || (InParameterIndex >= Machine->Parameters.Num()))
		{
			m_LastError = EString("Transition or parameter index is invalid.");
			return FALSE;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= TransitionObjects.Num()) || (!TransitionObjects[InTransitionIndex]))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}

		CJsonObject* Transition = TransitionObjects[InTransitionIndex];
		TArray<CJsonObject*> ConditionObjects;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		FJsonObjectArrayOwner ConditionOwner(ConditionObjects);
		if ((InConditionIndex < 0) || (InConditionIndex >= ConditionObjects.Num()) || (!ConditionObjects[InConditionIndex]))
		{
			m_LastError = EString("JSON condition index is invalid.");
			return FALSE;
		}

		CJsonObject* Condition = ConditionObjects[InConditionIndex];
		Condition->SetStringField(PE_TEXT("Param"), Machine->Parameters[InParameterIndex].Name);
		Condition->SetStringField(PE_TEXT("Op"), InOp);
		Condition->SetFloatField(PE_TEXT("Value"), InValue);

		Transition->SetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::RemoveTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= TransitionObjects.Num()) || (!TransitionObjects[InTransitionIndex]))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}

		CJsonObject* Transition = TransitionObjects[InTransitionIndex];
		TArray<CJsonObject*> ConditionObjects;
		Transition->GetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		FJsonObjectArrayOwner ConditionOwner(ConditionObjects);
		if ((InConditionIndex < 0) || (InConditionIndex >= ConditionObjects.Num()))
		{
			m_LastError = EString("JSON condition index is invalid.");
			return FALSE;
		}

		CJsonObject* Removed = ConditionObjects[InConditionIndex];
		ConditionObjects.RemoveAt(InConditionIndex);
		delete Removed;

		Transition->SetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::AddState(const EAnimStateMachine& InMachine)
	{
		if ((!m_Asset) || (InMachine.States.Num() <= 0))
		{
			m_LastError = EString("Cannot add a state to an empty machine.");
			return FALSE;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> StateObjects;
		Root.GetObjectArrayField(PE_TEXT("States"), StateObjects);
		FJsonObjectArrayOwner StateOwner(StateObjects);
		if (StateObjects.Num() <= 0)
		{
			m_LastError = EString("JSON has no states array.");
			return FALSE;
		}

		INT32 Suffix = StateObjects.Num();
		EString NewName;
		do
		{
			NewName = EString("State") + ToString(Suffix++);
		} while (JsonStateNameExists(StateObjects, NewName));

		EString ClipName;
		if (StateObjects[0])
		{
			StateObjects[0]->GetStringField(PE_TEXT("Clip"), ClipName);
		}
		CJsonObject* NewState = new CJsonObject();
		NewState->SetStringField(PE_TEXT("Name"), NewName);
		NewState->SetStringField(PE_TEXT("Clip"), ClipName);
		NewState->SetStringField(PE_TEXT("Loop"), EString("Repeat"));
		NewState->SetFloatField(PE_TEXT("Rate"), 1.0f);
		NewState->SetFloatField(PE_TEXT("Blend"), 0.2f);
		StateObjects.Add(NewState);

		Root.SetObjectArrayField(PE_TEXT("States"), StateObjects);
		EString EntryName;
		if ((StateObjects.Num() == 1)
			&& ((!Root.GetStringField(PE_TEXT("Entry"), EntryName)) || (EntryName.Length() == 0u)))
		{
			Root.SetStringField(PE_TEXT("Entry"), NewName);
		}

		if (!CommitJsonText(Root.ToString()))
		{
			return FALSE;
		}
		const EAnimStateMachine* Rebuilt = m_Asset->GetStoragedResource();
		m_SelectedStateIndex = (Rebuilt ? Rebuilt->FindState(NewName) : 0);
		m_TransitionFromIndex = -1;
		m_TransitionToIndex = 0;
		m_LastError = EString();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::DeleteState(const EString& InStateName)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if ((!Machine) || (Machine->States.Num() <= 1))
		{
			m_LastError = EString("Cannot delete the last state.");
			return FALSE;
		}
		// The input may alias an element of Machine->States; keep a value copy because the
		// rebuild replaces that array before we touch the name again.
		const EString DeletedStateName = InStateName;

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> StateObjects;
		Root.GetObjectArrayField(PE_TEXT("States"), StateObjects);
		FJsonObjectArrayOwner StateOwner(StateObjects);
		INT32 RemovedStateIndex = -1;
		for (INT32 StateIndex = 0; StateIndex < StateObjects.Num(); StateIndex++)
		{
			if ((StateObjects[StateIndex]) && (GetJsonObjectString(*StateObjects[StateIndex], EString("Name")) == DeletedStateName))
			{
				RemovedStateIndex = StateIndex;
				break;
			}
		}
		if (RemovedStateIndex < 0)
		{
			m_LastError = EString("State not found in JSON: ") + DeletedStateName;
			return FALSE;
		}
		CJsonObject* RemovedState = StateObjects[RemovedStateIndex];
		StateObjects.RemoveAt(RemovedStateIndex);
		delete RemovedState;

		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);
		TArray<CJsonObject*> KeptTransitions;
		for (INT32 TransitionIndex = 0; TransitionIndex < TransitionObjects.Num(); TransitionIndex++)
		{
			CJsonObject* Transition = TransitionObjects[TransitionIndex];
			if (!Transition)
			{
				continue;
			}
			if ((GetJsonObjectString(*Transition, EString("From")) == DeletedStateName)
				|| (GetJsonObjectString(*Transition, EString("To")) == DeletedStateName))
			{
				continue;
			}
			KeptTransitions.Add(Transition);
		}
		Root.SetObjectArrayField(PE_TEXT("States"), StateObjects);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), KeptTransitions);

		EString EntryName;
		if ((!Root.GetStringField(PE_TEXT("Entry"), EntryName))
			|| (EntryName == DeletedStateName) || (EntryName.Length() == 0u))
		{
			EString FirstStateName;
			if ((StateObjects.Num() > 0) && (StateObjects[0]))
			{
				StateObjects[0]->GetStringField(PE_TEXT("Name"), FirstStateName);
			}
			if (FirstStateName.Length() > 0u)
			{
				Root.SetStringField(PE_TEXT("Entry"), FirstStateName);
			}
		}

		if (!CommitJsonText(Root.ToString()))
		{
			return FALSE;
		}
		m_NodePositions.Remove(DeletedStateName);
		m_SelectedStateIndex = 0;
		m_SelectedTransitionIndex = -1;
		m_TransitionFromIndex = -1;
		m_TransitionToIndex = 0;
		m_LastError = EString();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::AddTransition(const EString& InFromStateName, const EString& InToStateName)
	{
		if ((!m_Asset) || (InToStateName.Length() <= 0u))
		{
			m_LastError = EString("A transition needs a target state.");
			return FALSE;
		}

		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);

		TArray<CJsonObject*> ConditionObjects;
		FJsonObjectArrayOwner ConditionOwner(ConditionObjects);
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if ((Machine) && (Machine->Parameters.Num() > 0))
		{
			const EAnimStateMachineParameter& Parameter = Machine->Parameters[0];
			EString DefaultOp;
			FLOAT DefaultValue = 0.0f;
			GetSafeConditionOpAndValue(Parameter, DefaultOp, DefaultValue);
			CJsonObject* Condition = new CJsonObject();
			Condition->SetStringField(PE_TEXT("Param"), Parameter.Name);
			Condition->SetStringField(PE_TEXT("Op"), DefaultOp);
			Condition->SetFloatField(PE_TEXT("Value"), DefaultValue);
			ConditionObjects.Add(Condition);
		}

		CJsonObject* NewTransition = new CJsonObject();
		NewTransition->SetStringField(PE_TEXT("From"), InFromStateName);
		NewTransition->SetStringField(PE_TEXT("To"), InToStateName);
		NewTransition->SetIntField(PE_TEXT("Priority"), 0);
		NewTransition->SetFloatField(PE_TEXT("Blend"), 0.2f);
		NewTransition->SetObjectArrayField(PE_TEXT("Conditions"), ConditionObjects);
		TransitionObjects.Add(NewTransition);
		Root.SetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);

		if (!CommitJsonText(Root.ToString()))
		{
			return FALSE;
		}
		m_LastError = ((ConditionObjects.Num() > 0)
			? EString() : EString("Transition added without conditions; edit the JSON before it runs."));
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::DeleteTransition(INT32 InTransitionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		TArray<CJsonObject*> TransitionObjects;
		Root.GetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		FJsonObjectArrayOwner TransitionOwner(TransitionObjects);
		if ((InTransitionIndex < 0) || (InTransitionIndex >= TransitionObjects.Num()))
		{
			m_LastError = EString("Transition index is invalid.");
			return FALSE;
		}

		CJsonObject* Removed = TransitionObjects[InTransitionIndex];
		TransitionObjects.RemoveAt(InTransitionIndex);
		delete Removed;
		Root.SetObjectArrayField(PE_TEXT("Transitions"), TransitionObjects);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::SetEntryState(const EString& InStateName)
	{
		if ((!m_Asset) || (InStateName.Length() <= 0u))
		{
			m_LastError = EString("No entry state selected.");
			return FALSE;
		}
		CJsonObject Root(m_JsonBuffer.GetData());
		if (!Root.IsValid())
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		Root.SetStringField(PE_TEXT("Entry"), InStateName);
		return CommitJsonText(Root.ToString());
	}

	BOOL32 EAnimStateMachineEditor::CommitJsonText(const EString& InJsonText)
	{
		EString Error;
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, InJsonText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(InJsonText);
		m_LastError = EString();
		m_Document.bDirty = TRUE;
		return TRUE;
	}

	void EAnimStateMachineEditor::ReplaceJsonBuffer(const EString& InJsonText)
	{
		m_JsonBuffer.Empty();
		const INT32 Length = static_cast<INT32>(InJsonText.Length()) + StateMachineEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(Length);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(Length));
		if (InJsonText.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(Length), *InJsonText, InJsonText.Length());
		}
	}

	void EAnimStateMachineEditor::LoadJsonBufferFromAsset()
	{
		m_JsonBuffer.Empty();
		if (!m_Asset)
		{
			return;
		}
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if (!Machine)
		{
			return;
		}

		const EString& SourceJson = Machine->SourceJson;
		const INT32 BufferLength = static_cast<INT32>(SourceJson.Length()) + StateMachineEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(BufferLength);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(BufferLength));
		if (SourceJson.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(BufferLength), *SourceJson, SourceJson.Length());
		}
	}

}

#endif
