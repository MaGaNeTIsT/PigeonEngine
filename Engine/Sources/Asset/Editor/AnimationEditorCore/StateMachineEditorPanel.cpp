#include "StateMachineEditorPanel.h"

#if ENGINE_EDITOR_ONLY

#include <AnimationAsset/AnimationAsset.h>
#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <cmath>
#include <cfloat>

namespace PigeonEngine
{

	namespace
	{
		const CHAR* GetLoopModeLabel(const EAnimationLoopMode InLoopMode)
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

		const CHAR* GetParameterTypeLabel(const EAnimStateMachineParameterType InType)
		{
			switch (InType)
			{
			case ANIM_PARAM_BOOL:
				return "Bool";
			case ANIM_PARAM_TRIGGER:
				return "Trigger";
			case ANIM_PARAM_FLOAT:
			default:
				return "Float";
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

		INT32 FindStateIndex(const EAnimStateMachine* InMachine, const EString& InName)
		{
			if (!InMachine)
			{
				return -1;
			}
			return InMachine->FindState(InName);
		}

		INT32 FindParameterIndex(const EAnimStateMachine* InMachine, const EString& InName)
		{
			if (!InMachine)
			{
				return -1;
			}
			return InMachine->FindParameter(InName);
		}
	}

	void FStateMachineEditorPanel::Draw(FStateMachineDocument& InDocument)
	{
		DrawToolbar(InDocument);
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s"
				, ((InDocument.LastError.Length() > 0u) ? *InDocument.LastError : "State machine draft is invalid."));
		}
		else
		{
			DrawLayout(InDocument);
		}
		DrawBottomPanel(InDocument);
	}

	void FStateMachineEditorPanel::DrawToolbar(FStateMachineDocument& InDocument)
	{
		if (ImGui::Button("Apply"))
		{
			EString Error;
			if (InDocument.Apply(Error))
			{
				m_StatusMessage = EString("Applied draft to the live asset.");
				m_bStatusSucceeded = TRUE;
				InDocument.LastError = EString();
				m_SourceDocument = nullptr;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			EString Error;
			if (InDocument.Save(Error))
			{
				m_StatusMessage = EString("Saved to disk.");
				m_bStatusSucceeded = TRUE;
				InDocument.LastError = EString();
				m_SourceDocument = nullptr;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
		{
			if (InDocument.IsDirty())
			{
				ImGui::OpenPopup("##sm_reload_confirm");
			}
			else
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					InDocument.LastError = EString();
					ResetEditorBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
			}
		}
		if (ImGui::BeginPopupModal("##sm_reload_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Reload will discard unsaved changes.");
			EString Error;
			if (ImGui::Button("Save and Reload"))
			{
				if (InDocument.Save(Error) && InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Saved and reloaded.");
					m_bStatusSucceeded = TRUE;
					ResetEditorBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard and Reload"))
			{
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Discarded and reloaded.");
					m_bStatusSucceeded = TRUE;
					ResetEditorBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(InDocument.Commands.CanUndo() == FALSE);
		if (ImGui::Button("Undo"))
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Undo(InDocument.CaptureSnapshot(), Snapshot))
			{
				InDocument.RestoreSnapshot(Snapshot);
				ResetEditorBuffers();
				m_StatusMessage = EString("Undo.");
				m_bStatusSucceeded = TRUE;
			}
		}
		ImGui::EndDisabled();
		ImGui::SameLine();
		ImGui::BeginDisabled(InDocument.Commands.CanRedo() == FALSE);
		if (ImGui::Button("Redo"))
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Redo(InDocument.CaptureSnapshot(), Snapshot))
			{
				InDocument.RestoreSnapshot(Snapshot);
				ResetEditorBuffers();
				m_StatusMessage = EString("Redo.");
				m_bStatusSucceeded = TRUE;
			}
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (InDocument.IsDirty())
		{
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.25f, 1.0f), "UNSAVED");
		}
		else if (InDocument.IsApplied() == FALSE)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.45f, 1.0f), "NOT APPLIED");
		}
		else
		{
			ImGui::TextDisabled("SYNCED");
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", *InDocument.AssetPath);

		if (InDocument.LastError.Length() > 0u)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *InDocument.LastError);
		}
		if (m_StatusMessage.Length() > 0u)
		{
			const ImVec4 Color = (m_bStatusSucceeded != FALSE)
				? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			ImGui::TextColored(Color, "%s", *m_StatusMessage);
		}
		ImGui::Separator();
	}

	void FStateMachineEditorPanel::DrawLayout(FStateMachineDocument& InDocument)
	{
		const ImGuiTableFlags LayoutFlags = ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV
			| ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
		if (ImGui::BeginTable("##sm_layout", 3, LayoutFlags))
		{
			ImGui::TableSetupColumn("##left", ImGuiTableColumnFlags_WidthFixed, 250.0f);
			ImGui::TableSetupColumn("##center", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("##right", ImGuiTableColumnFlags_WidthFixed, 350.0f);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			if (ImGui::BeginChild("##sm_left", ImVec2(0.0f, 0.0f), false))
			{
				DrawLeftPanel(InDocument);
			}
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(1);
			if (ImGui::BeginChild("##sm_center", ImVec2(0.0f, 0.0f), false))
			{
				if (ImGui::BeginTabBar("##sm_center_tabs"))
				{
					if (ImGui::BeginTabItem("Graph"))
					{
						DrawGraph(InDocument);
						ImGui::EndTabItem();
					}
					if (ImGui::BeginTabItem("Preview"))
					{
						m_PreviewPanel.Draw(InDocument);
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
			}
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(2);
			if (ImGui::BeginChild("##sm_right", ImVec2(0.0f, 0.0f), false))
			{
				DrawInspector(InDocument);
			}
			ImGui::EndChild();

			ImGui::EndTable();
		}
	}

	void FStateMachineEditorPanel::DrawLeftPanel(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			return;
		}
		ImGui::Text("Name: %s", *Machine->Name);
		ImGui::Text("Skeleton: %s", *Machine->SkeletonName);
		ImGui::Text("Animation: %s", *Machine->AnimationAssetName);
		ImGui::TextDisabled("States %d  Parameters %d  Transitions %d"
			, Machine->States.Num(), Machine->Parameters.Num(), Machine->Transitions.Num());
		ImGui::Separator();

		if (ImGui::Button("Add Parameter", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
				{
					return InDocument.AddParameter(OutError);
				}, EString("Parameter added."));
			ImGui::TextDisabled("Parameter added.");
		}
		if (ImGui::BeginChild("##sm_parameters", ImVec2(0.0f, 190.0f), true))
		{
			DrawParameterTable(InDocument);
		}
		ImGui::EndChild();

		ImGui::Separator();
		if (ImGui::Button("Add State", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
				{
					return InDocument.AddState(OutError);
				}, EString("State added."));
		}
		if (ImGui::BeginChild("##sm_states", ImVec2(0.0f, 0.0f), true))
		{
			DrawStateList(InDocument);
		}
		ImGui::EndChild();
	}

	void FStateMachineEditorPanel::DrawParameterTable(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			return;
		}
		if (m_ParameterNameBufferCount != Machine->Parameters.Num())
		{
			m_ParameterNameBuffers.SetNum(Machine->Parameters.Num());
			for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
			{
				m_ParameterNameBuffers[ParameterIndex].SetText(Machine->Parameters[ParameterIndex].Name);
			}
			m_ParameterNameBufferCount = Machine->Parameters.Num();
		}

		const ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV
			| ImGuiTableFlags_SizingStretchProp;
		BOOL32 bCommitted = FALSE;
		if (ImGui::BeginTable("##sm_parameter_table", 4, TableFlags))
		{
			ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 74.0f);
			ImGui::TableSetupColumn("Default", ImGuiTableColumnFlags_WidthFixed, 76.0f);
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 26.0f);
			ImGui::TableHeadersRow();

			for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
			{
				const EAnimStateMachineParameter& Parameter = Machine->Parameters[ParameterIndex];
				ImGui::PushID(ParameterIndex);
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				ImGui::SetNextItemWidth(-FLT_MIN);
				m_ParameterNameBuffers[ParameterIndex].InputText("##name");
				if (ImGui::IsItemActivated())
				{
					InDocument.Selection.Type = EEditorSelectionType::PARAMETER;
					InDocument.Selection.ObjectId = Parameter.Name;
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					const EString NewName = m_ParameterNameBuffers[ParameterIndex].ToString();
					if (NewName != Parameter.Name)
					{
						RunMutation(InDocument, [&InDocument, ParameterIndex, NewName](EString& OutError) -> BOOL32
							{
								return InDocument.RenameParameter(ParameterIndex, NewName, OutError);
							}, EString("Parameter renamed."));
						m_ParameterNameBufferCount = -1;
						bCommitted = TRUE;
					}
				}
				if (bCommitted != FALSE)
				{
					ImGui::PopID();
					break;
				}

				ImGui::TableSetColumnIndex(1);
				const INT32 UsageCount = GetParameterUsageCount(InDocument, Parameter.Name);
				const BOOL32 bTypeLocked = ((UsageCount > 0) ? TRUE : FALSE);
				ImGui::BeginDisabled(bTypeLocked != FALSE);
				ImGui::SetNextItemWidth(-FLT_MIN);
				if (ImGui::BeginCombo("##type", GetParameterTypeLabel(Parameter.Type)))
				{
					const EAnimStateMachineParameterType Types[3] = {
						ANIM_PARAM_FLOAT, ANIM_PARAM_BOOL, ANIM_PARAM_TRIGGER
					};
					for (UINT32 TypeIndex = 0u; TypeIndex < 3u; TypeIndex++)
					{
						const BOOL8 bSelected = (Parameter.Type == Types[TypeIndex]);
						if (ImGui::Selectable(GetParameterTypeLabel(Types[TypeIndex]), bSelected))
						{
							RunMutation(InDocument, [&InDocument, ParameterIndex, &Types, TypeIndex](EString& OutError) -> BOOL32
								{
									return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(Types[TypeIndex])
										, 0.0f, FALSE, OutError);
								}, EString("Parameter type updated."));
							m_ParameterNameBufferCount = -1;
							bCommitted = TRUE;
							break;
						}
						if (bSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				ImGui::EndDisabled();
				if (bCommitted != FALSE)
				{
					ImGui::PopID();
					break;
				}
				if (bTypeLocked != FALSE)
				{
					ImGui::SetItemTooltip("Parameter is referenced by %d condition(s). Remove those references before changing its type."
						, UsageCount);
				}

				ImGui::TableSetColumnIndex(2);
				if (Parameter.Type == ANIM_PARAM_FLOAT)
				{
					FLOAT DefaultValue = Parameter.DefaultFloat;
					ImGui::SetNextItemWidth(-FLT_MIN);
					ImGui::InputFloat("##default", &DefaultValue, 0.0f, 0.0f, "%.3f");
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						RunMutation(InDocument, [&InDocument, ParameterIndex, DefaultValue](EString& OutError) -> BOOL32
							{
								return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(ANIM_PARAM_FLOAT)
									, DefaultValue, FALSE, OutError);
							}, EString("Parameter default updated."));
						ImGui::PopID();
						bCommitted = TRUE;
						break;
					}
				}
				else if (Parameter.Type == ANIM_PARAM_BOOL)
				{
					bool bDefaultValue = (Parameter.DefaultBool != FALSE);
					if (ImGui::Checkbox("##default", &bDefaultValue))
					{
						RunMutation(InDocument, [&InDocument, ParameterIndex, bDefaultValue](EString& OutError) -> BOOL32
							{
								return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(ANIM_PARAM_BOOL)
									, 0.0f, (bDefaultValue ? TRUE : FALSE), OutError);
							}, EString("Parameter default updated."));
						ImGui::PopID();
						bCommitted = TRUE;
						break;
					}
				}
				else
				{
					ImGui::TextDisabled("--");
				}

				ImGui::TableSetColumnIndex(3);
				if (ImGui::Button("x"))
				{
					RunMutation(InDocument, [&InDocument, ParameterIndex](EString& OutError) -> BOOL32
						{
							return InDocument.RemoveParameter(ParameterIndex, OutError);
						}, EString("Parameter removed."));
					m_ParameterNameBufferCount = -1;
					bCommitted = TRUE;
				}
				ImGui::PopID();
				if (bCommitted != FALSE)
				{
					break;
				}
			}
			ImGui::EndTable();
		}
	}

	void FStateMachineEditorPanel::DrawStateList(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			return;
		}
		for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
		{
			const EAnimStateMachineState& State = Machine->States[StateIndex];
			ImGui::PushID(StateIndex);
			const BOOL8 bSelected = ((InDocument.Selection.Is(EEditorSelectionType::STATE))
				&& (InDocument.Selection.ObjectId == State.Name)) ? TRUE : FALSE;
			CHAR Label[192] = "";
			::sprintf_s(Label, sizeof(Label), "%s%s", *State.Name
				, ((Machine->EntryStateIndex == StateIndex) ? "  [Entry]" : ""));
			if (ImGui::Selectable(Label, bSelected))
			{
				InDocument.Selection.Type = EEditorSelectionType::STATE;
				InDocument.Selection.ObjectId = State.Name;
				InDocument.Selection.ObjectIndex = StateIndex;
			}
			ImGui::PopID();
		}
	}

	void FStateMachineEditorPanel::DrawGraph(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if ((!Machine) || (Machine->States.Num() <= 0))
		{
			ImGui::TextDisabled("No states.");
			return;
		}
		if (m_LastGraphDocument != &InDocument)
		{
			m_LastGraphDocument = &InDocument;
			m_GraphPan = ImVec2(0.0f, 0.0f);
			m_GraphZoom = 1.0f;
			m_bGraphPanning = FALSE;
			m_GraphPanButton = -1;
			// First draw of a document shows the whole graph instead of only the first node.
			m_bGraphFitRequested = TRUE;
		}

		for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
		{
			const EString& StateName = Machine->States[StateIndex].Name;
			if (!InDocument.NodePositions.FindValueAsPtr(StateName))
			{
				InDocument.NodePositions.Add(StateName, ImVec2(40.0f + (StateIndex % 3) * 240.0f
					, 40.0f + (StateIndex / 3) * 112.0f));
			}
		}

		enum class EGraphActionType : UINT8
		{
			NONE = 0,
			ADD_STATE,
			DELETE_STATE,
			SET_ENTRY,
			ADD_TRANSITION,
			RESET_LAYOUT
		};
		EGraphActionType PendingAction = EGraphActionType::NONE;
		EString PendingStateName;
		EString PendingFromName;
		EString PendingToName;

		if (ImGui::BeginChild("##sm_graph", ImVec2(0.0f, 0.0f), true
			, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
		{
			const ImVec2 Origin = ImGui::GetCursorScreenPos();
			const ImVec2 Available = ImGui::GetContentRegionAvail();
			const ImVec2 CanvasSize(EMath::Max(Available.x, 120.0f), EMath::Max(Available.y, 120.0f));
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			constexpr FLOAT NodeWidth = 190.0f;
			constexpr FLOAT NodeHeight = 62.0f;
			const ImGuiIO& IO = ImGui::GetIO();

			BOOL32 bHasAnyTransition = FALSE;
			for (INT32 TransitionIndex = 0; TransitionIndex < Machine->Transitions.Num(); TransitionIndex++)
			{
				if (Machine->Transitions[TransitionIndex].FromStateIndex < 0)
				{
					bHasAnyTransition = TRUE;
					break;
				}
			}
			const ImVec2 AnyLocal(40.0f, 12.0f);
			const ImVec2 AnySize(100.0f, 32.0f);

			if (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
			{
				if (IO.MouseWheel != 0.0f)
				{
					const ImVec2 Mouse = IO.MousePos;
					const ImVec2 LocalBefore((Mouse.x - Origin.x - m_GraphPan.x) / m_GraphZoom
						, (Mouse.y - Origin.y - m_GraphPan.y) / m_GraphZoom);
					m_GraphZoom = EMath::Clamp(m_GraphZoom
						* ((IO.MouseWheel > 0.0f) ? 1.1f : 0.9f), 0.15f, 3.0f);
					m_GraphPan.x = Mouse.x - Origin.x - LocalBefore.x * m_GraphZoom;
					m_GraphPan.y = Mouse.y - Origin.y - LocalBefore.y * m_GraphZoom;
				}
				if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle))
				{
					m_bGraphPanning = TRUE;
					m_GraphPanButton = static_cast<INT32>(ImGuiMouseButton_Middle);
				}
				if ((IO.WantTextInput == false) && (ImGui::IsKeyPressed(ImGuiKey_Home, false) != false))
				{
					m_bGraphFitRequested = TRUE;
				}
			}
			if (m_bGraphPanning != FALSE)
			{
				const BOOL32 bPanDown = ((m_GraphPanButton == static_cast<INT32>(ImGuiMouseButton_Left))
					? ImGui::IsMouseDown(ImGuiMouseButton_Left)
					: ((m_GraphPanButton == static_cast<INT32>(ImGuiMouseButton_Middle))
						? ImGui::IsMouseDown(ImGuiMouseButton_Middle) : FALSE));
				if (bPanDown != FALSE)
				{
					m_GraphPan.x += IO.MouseDelta.x;
					m_GraphPan.y += IO.MouseDelta.y;
					ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				}
				else
				{
					m_bGraphPanning = FALSE;
					m_GraphPanButton = -1;
				}
			}

			FLOAT MinX = FLT_MAX;
			FLOAT MinY = FLT_MAX;
			FLOAT MaxX = -FLT_MAX;
			FLOAT MaxY = -FLT_MAX;
			for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
			{
				const ImVec2* Stored = InDocument.NodePositions.FindValueAsPtr(Machine->States[StateIndex].Name);
				const ImVec2 Local = (Stored ? *Stored : ImVec2(40.0f, 80.0f));
				MinX = EMath::Min(MinX, Local.x);
				MinY = EMath::Min(MinY, Local.y);
				MaxX = EMath::Max(MaxX, Local.x + NodeWidth);
				MaxY = EMath::Max(MaxY, Local.y + NodeHeight);
			}
			if (bHasAnyTransition != FALSE)
			{
				MinX = EMath::Min(MinX, AnyLocal.x);
				MinY = EMath::Min(MinY, AnyLocal.y);
				MaxX = EMath::Max(MaxX, AnyLocal.x + AnySize.x);
				MaxY = EMath::Max(MaxY, AnyLocal.y + AnySize.y);
			}
			if ((m_bGraphFitRequested != FALSE) && (MinX <= MaxX) && (MinY <= MaxY))
			{
				const FLOAT GraphWidth = EMath::Max(MaxX - MinX, 1.0f);
				const FLOAT GraphHeight = EMath::Max(MaxY - MinY, 1.0f);
				m_GraphZoom = EMath::Clamp(EMath::Min((CanvasSize.x - 40.0f) / GraphWidth
					, (CanvasSize.y - 40.0f) / GraphHeight), 0.15f, 1.5f);
				m_GraphPan.x = (CanvasSize.x - GraphWidth * m_GraphZoom) * 0.5f - MinX * m_GraphZoom;
				m_GraphPan.y = (CanvasSize.y - GraphHeight * m_GraphZoom) * 0.5f - MinY * m_GraphZoom;
				m_bGraphFitRequested = FALSE;
			}

			const FLOAT Zoom = m_GraphZoom;
			const ImVec2 NodeSize(NodeWidth * Zoom, NodeHeight * Zoom);
			// A full-canvas item keeps ImGui from treating a left-drag on empty graph space as
			// a window move. Later node buttons are allowed to overlap it.
			ImGui::SetCursorScreenPos(Origin);
			ImGui::SetNextItemAllowOverlap();
			ImGui::InvisibleButton("##sm_graph_canvas", CanvasSize);
			const BOOL32 bCanvasHovered = ImGui::IsItemHovered();
			const BOOL32 bCanvasActive = ImGui::IsItemActive();
			const BOOL32 bCanvasClicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
			if ((bCanvasHovered != FALSE) || (bCanvasActive != FALSE))
			{
				// Claim the wheel every frame while the canvas is hovered/active. Otherwise
				// ImGui forwards it to the scrollable parent window during NewFrame.
				ImGui::SetItemKeyOwner(ImGuiKey_MouseWheelY);
			}
			auto GraphToScreen = [this, &Origin](const ImVec2& InLocal) -> ImVec2
			{
				return ImVec2(Origin.x + m_GraphPan.x + InLocal.x * m_GraphZoom
					, Origin.y + m_GraphPan.y + InLocal.y * m_GraphZoom);
			};
			auto IsMouseOverGraphItem = [this, &InDocument, &Machine, &AnyLocal, &AnySize
				, &Origin, &CanvasSize, Zoom, NodeSize, &GraphToScreen, bHasAnyTransition]() -> BOOL32
			{
				const ImVec2 Mouse = ImGui::GetIO().MousePos;
				for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
				{
					const ImVec2* Stored = InDocument.NodePositions.FindValueAsPtr(Machine->States[StateIndex].Name);
					const ImVec2 Local = (Stored ? *Stored : ImVec2(40.0f, 80.0f));
					const ImVec2 Position = GraphToScreen(Local);
					if ((Mouse.x >= Position.x) && (Mouse.x <= Position.x + NodeSize.x)
						&& (Mouse.y >= Position.y) && (Mouse.y <= Position.y + NodeSize.y))
					{
						return TRUE;
					}
				}
				const ImVec2 AnyPosition = GraphToScreen(AnyLocal);
				const ImVec2 AnyScreenSize(AnySize.x * Zoom, AnySize.y * Zoom);
				if ((bHasAnyTransition != FALSE)
					&& (Mouse.x >= AnyPosition.x) && (Mouse.x <= AnyPosition.x + AnyScreenSize.x)
					&& (Mouse.y >= AnyPosition.y) && (Mouse.y <= AnyPosition.y + AnyScreenSize.y))
				{
					return TRUE;
				}
				const ImVec2 FitButtonMin(Origin.x + CanvasSize.x - 74.0f, Origin.y + 8.0f);
				if ((Mouse.x >= FitButtonMin.x) && (Mouse.x <= FitButtonMin.x + 66.0f)
					&& (Mouse.y >= FitButtonMin.y)
					&& (Mouse.y <= FitButtonMin.y + ImGui::GetFrameHeight()))
				{
					return TRUE;
				}
				return FALSE;
			};
			auto GetNodePosition = [&InDocument](const EString& InName) -> ImVec2
			{
				const ImVec2* Stored = InDocument.NodePositions.FindValueAsPtr(InName);
				return (Stored ? *Stored : ImVec2(40.0f, 80.0f));
			};

			struct FEdgeSegment
			{
				ImVec2 A;
				ImVec2 B;
				INT32 TransitionIndex;
			};
			TArray<FEdgeSegment> EdgeSegments;

			auto DrawArrow = [DrawList, Zoom](const ImVec2& InFrom, const ImVec2& InTo
				, const ImU32 InColor, const FLOAT InThickness)
			{
				DrawList->AddLine(InFrom, InTo, InColor, InThickness);
				const FLOAT DeltaX = InTo.x - InFrom.x;
				const FLOAT DeltaY = InTo.y - InFrom.y;
				const FLOAT Length = std::sqrt(DeltaX * DeltaX + DeltaY * DeltaY);
				if (Length > 0.001f)
				{
					const FLOAT DirectionX = DeltaX / Length;
					const FLOAT DirectionY = DeltaY / Length;
					const FLOAT ArrowLength = 10.0f * Zoom;
					const FLOAT ArrowHalfWidth = 5.0f * Zoom;
					const ImVec2 Base(InTo.x - DirectionX * ArrowLength, InTo.y - DirectionY * ArrowLength);
					const ImVec2 SideA(Base.x - DirectionY * ArrowHalfWidth, Base.y + DirectionX * ArrowHalfWidth);
					const ImVec2 SideB(Base.x + DirectionY * ArrowHalfWidth, Base.y - DirectionX * ArrowHalfWidth);
					DrawList->AddTriangleFilled(InTo, SideA, SideB, InColor);
				}
			};

			const ImU32 EdgeColor = IM_COL32(170, 180, 200, 190);
			for (INT32 TransitionIndex = 0; TransitionIndex < Machine->Transitions.Num(); TransitionIndex++)
			{
				const EAnimStateMachineTransition& Transition = Machine->Transitions[TransitionIndex];
				if ((Transition.ToStateIndex < 0) || (Transition.ToStateIndex >= Machine->States.Num()))
				{
					continue;
				}
				const BOOL32 bSelected = ((InDocument.Selection.Is(EEditorSelectionType::TRANSITION)
					|| InDocument.Selection.Is(EEditorSelectionType::CONDITION))
					&& (InDocument.Selection.ObjectIndex == TransitionIndex)) ? TRUE : FALSE;
				const ImU32 CurrentColor = (bSelected != FALSE) ? IM_COL32(255, 205, 90, 255) : EdgeColor;
				const FLOAT CurrentThickness = ((bSelected != FALSE) ? 3.0f : 2.0f)
					* EMath::Max(Zoom, 0.5f);

				const ImVec2 ToLocal = GetNodePosition(Machine->States[Transition.ToStateIndex].Name);
				const ImVec2 To = GraphToScreen(ImVec2(ToLocal.x, ToLocal.y + NodeHeight * 0.5f));

				ImVec2 From;
				if (Transition.FromStateIndex < 0)
				{
					if (!bHasAnyTransition)
					{
						continue;
					}
					From = GraphToScreen(ImVec2(AnyLocal.x + AnySize.x
						, AnyLocal.y + AnySize.y * 0.5f));
				}
				else
				{
					if (Transition.FromStateIndex >= Machine->States.Num())
					{
						continue;
					}
					const ImVec2 FromLocal = GetNodePosition(Machine->States[Transition.FromStateIndex].Name);
					if (Transition.FromStateIndex == Transition.ToStateIndex)
					{
						const ImVec2 TopLocal(FromLocal.x + NodeWidth * 0.5f, FromLocal.y);
						const ImVec2 LoopLeftLocal(TopLocal.x - 18.0f, TopLocal.y - 26.0f);
						const ImVec2 LoopRightLocal(TopLocal.x + 26.0f, TopLocal.y - 26.0f);
						const ImVec2 Top = GraphToScreen(TopLocal);
						const ImVec2 LoopLeft = GraphToScreen(LoopLeftLocal);
						const ImVec2 LoopRight = GraphToScreen(LoopRightLocal);
						DrawList->AddLine(Top, LoopLeft, CurrentColor, CurrentThickness);
						DrawList->AddLine(LoopLeft, LoopRight, CurrentColor, CurrentThickness);
						DrawArrow(LoopRight, Top, CurrentColor, CurrentThickness);
						FEdgeSegment SegmentA = { Top, LoopLeft, TransitionIndex };
						FEdgeSegment SegmentB = { LoopLeft, LoopRight, TransitionIndex };
						FEdgeSegment SegmentC = { LoopRight, Top, TransitionIndex };
						EdgeSegments.Add(SegmentA);
						EdgeSegments.Add(SegmentB);
						EdgeSegments.Add(SegmentC);
						continue;
					}
					From = GraphToScreen(ImVec2(FromLocal.x + NodeWidth
						, FromLocal.y + NodeHeight * 0.5f));
				}

				DrawArrow(From, To, CurrentColor, CurrentThickness);
				FEdgeSegment Segment = { From, To, TransitionIndex };
				EdgeSegments.Add(Segment);
				CHAR Label[64] = "";
				::sprintf_s(Label, sizeof(Label), "p%d c%d", Transition.Priority, Transition.Conditions.Num());
				DrawList->AddText(ImVec2((From.x + To.x) * 0.5f, (From.y + To.y) * 0.5f - 14.0f), CurrentColor, Label);
			}

			if (bHasAnyTransition)
			{
				const ImVec2 AnyPosition = GraphToScreen(AnyLocal);
				const ImVec2 AnyScreenSize(AnySize.x * Zoom, AnySize.y * Zoom);
				DrawList->AddRectFilled(AnyPosition
					, ImVec2(AnyPosition.x + AnyScreenSize.x, AnyPosition.y + AnyScreenSize.y)
					, IM_COL32(55, 55, 70, 255), 4.0f * Zoom);
				DrawList->AddRect(AnyPosition
					, ImVec2(AnyPosition.x + AnyScreenSize.x, AnyPosition.y + AnyScreenSize.y)
					, IM_COL32(120, 120, 150, 255), 4.0f * Zoom);
				DrawList->AddText(ImGui::GetFont(), ImGui::GetFontSize() * Zoom
					, ImVec2(AnyPosition.x + 8.0f * Zoom, AnyPosition.y + 7.0f * Zoom)
					, IM_COL32(210, 210, 230, 255), "Any State");
			}

			BOOL32 bStopGraph = FALSE;
			for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
			{
				const EAnimStateMachineState& State = Machine->States[StateIndex];
				ImVec2* StoredPosition = InDocument.NodePositions.FindValueAsPtr(State.Name);
				const ImVec2 Local = (StoredPosition ? *StoredPosition : ImVec2(40.0f, 80.0f));
				const ImVec2 Position = GraphToScreen(Local);

				BOOL32 bNodeMoved = FALSE;
				ImGui::PushID(*State.Name);
				ImGui::SetCursorScreenPos(Position);
				ImGui::InvisibleButton("##node", NodeSize);
				if (ImGui::IsItemClicked(ImGuiMouseButton_Left))
				{
					InDocument.Selection.Type = EEditorSelectionType::STATE;
					InDocument.Selection.ObjectId = State.Name;
					InDocument.Selection.ObjectIndex = StateIndex;
				}
				if (ImGui::BeginPopupContextItem("##node_menu"))
				{
					InDocument.Selection.Type = EEditorSelectionType::STATE;
					InDocument.Selection.ObjectId = State.Name;
					if (ImGui::MenuItem("Set as Entry", nullptr, false, (Machine->EntryStateIndex != StateIndex)))
					{
						PendingAction = EGraphActionType::SET_ENTRY;
						PendingStateName = State.Name;
					}
					if (ImGui::MenuItem("Delete State", nullptr, false, (Machine->States.Num() > 1)))
					{
						PendingAction = EGraphActionType::DELETE_STATE;
						PendingStateName = State.Name;
					}
					ImGui::Separator();
					if (ImGui::BeginMenu("Create Transition From This State"))
					{
						for (INT32 TargetIndex = 0; TargetIndex < Machine->States.Num(); TargetIndex++)
						{
							if (TargetIndex == StateIndex)
							{
								continue;
							}
							if (ImGui::MenuItem(*Machine->States[TargetIndex].Name))
							{
								PendingAction = EGraphActionType::ADD_TRANSITION;
								PendingFromName = State.Name;
								PendingToName = Machine->States[TargetIndex].Name;
							}
						}
						ImGui::EndMenu();
					}
					if (ImGui::BeginMenu("Create Transition To This State"))
					{
						if (ImGui::MenuItem("Any"))
						{
							PendingAction = EGraphActionType::ADD_TRANSITION;
							PendingFromName = EString();
							PendingToName = State.Name;
						}
						for (INT32 SourceIndex = 0; SourceIndex < Machine->States.Num(); SourceIndex++)
						{
							if (SourceIndex == StateIndex)
							{
								continue;
							}
							if (ImGui::MenuItem(*Machine->States[SourceIndex].Name))
							{
								PendingAction = EGraphActionType::ADD_TRANSITION;
								PendingFromName = Machine->States[SourceIndex].Name;
								PendingToName = State.Name;
							}
						}
						ImGui::EndMenu();
					}
					ImGui::EndPopup();
				}

				if (ImGui::IsItemActivated())
				{
					InDocument.Commands.BeginTransaction(InDocument.CaptureSnapshot());
				}
				if ((ImGui::IsItemActive()) && (StoredPosition))
				{
					const ImVec2 MouseDelta = ImGui::GetIO().MouseDelta;
					if ((MouseDelta.x != 0.0f) || (MouseDelta.y != 0.0f))
					{
						StoredPosition->x += MouseDelta.x / Zoom;
						StoredPosition->y += MouseDelta.y / Zoom;
						bNodeMoved = TRUE;
					}
				}
				if (ImGui::IsItemDeactivated())
				{
					if (bNodeMoved != FALSE)
					{
						EString Error;
						if (InDocument.CommitSourceText(InDocument.BuildSerializedJson(), Error))
						{
							InDocument.Commands.CommitTransaction(InDocument.CaptureSnapshot());
						}
						else
						{
							InDocument.Commands.CancelTransaction();
							InDocument.LastError = Error;
						}
						// The commit rebuilt the draft machine; this frame's State reference
						// is stale, so finish the child and let the next frame draw the graph.
						ImGui::PopID();
						bStopGraph = TRUE;
						break;
					}
					InDocument.Commands.CancelTransaction();
				}

				const BOOL32 bEntry = ((Machine->EntryStateIndex == StateIndex) ? TRUE : FALSE);
				const ImU32 BodyColor = IM_COL32(44, 46, 54, 255);
				const ImU32 HeaderColor = (bEntry != FALSE)
					? IM_COL32(50, 120, 80, 255) : IM_COL32(55, 80, 125, 255);
				const FLOAT NodeFontSize = ImGui::GetFontSize() * Zoom;
				DrawList->AddRectFilled(Position, ImVec2(Position.x + NodeSize.x, Position.y + NodeSize.y)
					, BodyColor, 4.0f * Zoom);
				DrawList->AddRectFilled(Position, ImVec2(Position.x + NodeSize.x
					, Position.y + 22.0f * Zoom), HeaderColor, 4.0f * Zoom);
				DrawList->AddRect(Position, ImVec2(Position.x + NodeSize.x, Position.y + NodeSize.y)
					, IM_COL32(100, 105, 120, 255), 4.0f * Zoom);
				DrawList->AddText(ImGui::GetFont(), NodeFontSize
					, ImVec2(Position.x + 8.0f * Zoom, Position.y + 4.0f * Zoom)
					, IM_COL32(235, 235, 240, 255), *State.Name);
				DrawList->AddText(ImGui::GetFont(), NodeFontSize
					, ImVec2(Position.x + 8.0f * Zoom, Position.y + 26.0f * Zoom)
					, IM_COL32(190, 200, 210, 255)
					, ((State.ClipName.Length() > 0u) ? *State.ClipName : "None"));
				CHAR Detail[96] = "";
				::sprintf_s(Detail, sizeof(Detail), "%s  rate %.2f%s"
					, GetLoopModeLabel(State.LoopMode), State.PlayRate, (bEntry != FALSE ? "  ENTRY" : ""));
				DrawList->AddText(ImGui::GetFont(), NodeFontSize
					, ImVec2(Position.x + 8.0f * Zoom, Position.y + 44.0f * Zoom)
					, IM_COL32(160, 170, 185, 255), Detail);
				ImGui::PopID();
			}

			if (bStopGraph != FALSE)
			{
				ImGui::EndChild();
				return;
			}

			const BOOL32 bCanvasPressed = ((bCanvasClicked != FALSE)
				|| ((ImGui::IsMouseClicked(ImGuiMouseButton_Left) != false)
					&& (ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) != false))) ? TRUE : FALSE;
			if (bCanvasPressed != FALSE)
			{
				const ImVec2 MousePosition = ImGui::GetIO().MousePos;
				const BOOL32 bInsideCanvas = ((MousePosition.x >= Origin.x)
					&& (MousePosition.x <= Origin.x + CanvasSize.x)
					&& (MousePosition.y >= Origin.y)
					&& (MousePosition.y <= Origin.y + CanvasSize.y)) ? TRUE : FALSE;

				if ((IsMouseOverGraphItem() == FALSE) && (bInsideCanvas != FALSE))
				{
					FLOAT BestDistanceSquared = 8.0f * 8.0f;
					INT32 BestTransitionIndex = -1;
					for (INT32 SegmentIndex = 0; SegmentIndex < EdgeSegments.Num(); SegmentIndex++)
					{
						const FEdgeSegment& Segment = EdgeSegments[SegmentIndex];
						const FLOAT DistanceSquared = DistanceToSegmentSquared(MousePosition, Segment.A, Segment.B);
						if (DistanceSquared < BestDistanceSquared)
						{
							BestDistanceSquared = DistanceSquared;
							BestTransitionIndex = Segment.TransitionIndex;
						}
					}
					if (BestTransitionIndex >= 0)
					{
						InDocument.Selection.Type = EEditorSelectionType::TRANSITION;
						InDocument.Selection.ObjectIndex = BestTransitionIndex;
					}
					else
					{
						// Left-drag on empty canvas pans the graph like middle-drag. Clicking
						// near an edge still selects the transition instead of panning.
						m_bGraphPanning = TRUE;
						m_GraphPanButton = static_cast<INT32>(ImGuiMouseButton_Left);
					}
				}
			}

			const ImVec2 BackgroundMouse = ImGui::GetIO().MousePos;
			const BOOL32 bBackgroundInsideCanvas = ((BackgroundMouse.x >= Origin.x)
				&& (BackgroundMouse.x <= Origin.x + CanvasSize.x)
				&& (BackgroundMouse.y >= Origin.y)
				&& (BackgroundMouse.y <= Origin.y + CanvasSize.y)) ? TRUE : FALSE;
			if ((ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem) != false)
				&& (ImGui::IsMouseClicked(ImGuiMouseButton_Right) != false)
				&& (bBackgroundInsideCanvas != FALSE) && (IsMouseOverGraphItem() == FALSE))
			{
				ImGui::OpenPopup("##sm_graph_menu");
			}
			if (ImGui::BeginPopup("##sm_graph_menu"))
			{
				if (ImGui::MenuItem("Add State"))
				{
					PendingAction = EGraphActionType::ADD_STATE;
				}
				if (ImGui::MenuItem("Reset Layout"))
				{
					PendingAction = EGraphActionType::RESET_LAYOUT;
				}
				if (ImGui::MenuItem("Fit Graph"))
				{
					m_bGraphFitRequested = TRUE;
				}
				if ((InDocument.Selection.Is(EEditorSelectionType::STATE))
					&& (InDocument.Selection.ObjectId.Length() > 0u))
				{
					ImGui::Separator();
					if (ImGui::MenuItem("Create Any -> Selected State"))
					{
						PendingAction = EGraphActionType::ADD_TRANSITION;
						PendingFromName = EString();
						PendingToName = InDocument.Selection.ObjectId;
					}
				}
				ImGui::EndPopup();
			}

			ImGui::SetCursorScreenPos(ImVec2(Origin.x + CanvasSize.x - 74.0f, Origin.y + 8.0f));
			if (ImGui::Button("Fit", ImVec2(66.0f, 0.0f)))
			{
				m_bGraphFitRequested = TRUE;
			}
			DrawList->AddText(ImVec2(Origin.x + 8.0f, Origin.y + CanvasSize.y - 20.0f)
				, IM_COL32(130, 135, 150, 220), "Wheel zoom | Middle/empty-drag pan | Home fit");
		}
		ImGui::EndChild();

		if (PendingAction != EGraphActionType::NONE)
		{
			switch (PendingAction)
			{
			case EGraphActionType::ADD_STATE:
				RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
					{
						return InDocument.AddState(OutError);
					}, EString("State added."));
				break;
			case EGraphActionType::DELETE_STATE:
				RunMutation(InDocument, [&InDocument, PendingStateName](EString& OutError) -> BOOL32
					{
						return InDocument.DeleteState(PendingStateName, OutError);
					}, EString("State deleted."));
				break;
			case EGraphActionType::SET_ENTRY:
				RunMutation(InDocument, [&InDocument, PendingStateName](EString& OutError) -> BOOL32
					{
						return InDocument.SetEntryState(PendingStateName, OutError);
					}, EString("Entry state updated."));
				break;
			case EGraphActionType::ADD_TRANSITION:
				RunMutation(InDocument, [&InDocument, PendingFromName, PendingToName](EString& OutError) -> BOOL32
					{
						return InDocument.AddTransition(PendingFromName, PendingToName, OutError);
					}, EString("Transition added."));
				break;
			case EGraphActionType::RESET_LAYOUT:
				InDocument.NodePositions.Empty();
				m_GraphPan = ImVec2(0.0f, 0.0f);
				m_GraphZoom = 1.0f;
				m_bGraphPanning = FALSE;
				m_GraphPanButton = -1;
				m_bGraphFitRequested = FALSE;
				for (INT32 StateIndex = 0; StateIndex < Machine->States.Num(); StateIndex++)
				{
					InDocument.NodePositions.Add(Machine->States[StateIndex].Name
						, ImVec2(40.0f + (StateIndex % 3) * 240.0f, 40.0f + (StateIndex / 3) * 112.0f));
				}
				RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
					{
						return InDocument.CommitSourceText(InDocument.BuildSerializedJson(), OutError);
					}, EString("Layout reset."));
				break;
			default:
				break;
			}
		}
	}

	void FStateMachineEditorPanel::DrawInspector(FStateMachineDocument& InDocument)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			return;
		}
		FImGuiIdScope InspectorScope("StateMachineInspector");
		ImGui::TextDisabled("Inspector");
		ImGui::Separator();
		if ((InDocument.Selection.Type == EEditorSelectionType::STATE)
			|| (InDocument.Selection.Type == EEditorSelectionType::NONE))
		{
			if (InDocument.Selection.Type == EEditorSelectionType::STATE)
			{
				DrawStateInspector(InDocument, InDocument.Selection.ObjectId);
			}
			else
			{
				ImGui::Text("Machine: %s", *Machine->Name);
				ImGui::Text("Skeleton: %s", *Machine->SkeletonName);
				ImGui::Text("Animation: %s", *Machine->AnimationAssetName);
				ImGui::Text("States: %d", Machine->States.Num());
				ImGui::Text("Parameters: %d", Machine->Parameters.Num());
				ImGui::Text("Transitions: %d", Machine->Transitions.Num());
				ImGui::Separator();
				ImGui::TextDisabled("Select a state, transition or parameter.");
			}
		}
		else if (InDocument.Selection.Type == EEditorSelectionType::PARAMETER)
		{
			DrawParameterInspector(InDocument, InDocument.Selection.ObjectId);
		}
		else if ((InDocument.Selection.Type == EEditorSelectionType::TRANSITION)
			|| (InDocument.Selection.Type == EEditorSelectionType::CONDITION))
		{
			DrawTransitionInspector(InDocument, InDocument.Selection.ObjectIndex);
		}
		else
		{
			ImGui::TextDisabled("Select a state, transition or parameter.");
		}
	}

	void FStateMachineEditorPanel::DrawStateInspector(FStateMachineDocument& InDocument, const EString& InStateName)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		const INT32 StateIndex = FindStateIndex(Machine, InStateName);
		if ((!Machine) || (StateIndex < 0))
		{
			InDocument.Selection.Clear();
			return;
		}
		const EAnimStateMachineState& State = Machine->States[StateIndex];

		if (m_StateNameBufferSource != State.Name)
		{
			m_StateNameBuffer.SetText(State.Name);
			m_StateNameBufferSource = State.Name;
		}
		ImGui::TextDisabled("State");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_StateNameBuffer.InputText("##state_name");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			const EString NewName = m_StateNameBuffer.ToString();
			if (NewName != State.Name)
			{
				RunMutation(InDocument, [&InDocument, InStateName, NewName](EString& OutError) -> BOOL32
					{
						return InDocument.RenameState(InStateName, NewName, OutError);
					}, EString("State renamed."));
				m_StateNameBufferSource = EString();
				return;
			}
		}

		const ESkeletonAnimation* Animation =
			(InDocument.GetAnimationAsset() ? InDocument.GetAnimationAsset()->GetStoragedResource() : nullptr);
		BOOL32 bStateCommitted = FALSE;
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##state_clip", ((State.ClipName.Length() > 0u) ? *State.ClipName : "None")))
		{
			if (Animation)
			{
				for (INT32 ClipIndex = 0; ClipIndex < Animation->GetAnimationClipNum(); ClipIndex++)
				{
					const ESkeletonAnimationClip* Clip = Animation->GetAnimationClip(ClipIndex);
					if (!Clip)
					{
						continue;
					}
					const BOOL8 bSelected = (State.ClipName == Clip->ClipName);
					if (ImGui::Selectable(*Clip->ClipName, bSelected))
					{
						RunMutation(InDocument, [&InDocument, StateIndex, Clip, &State](EString& OutError) -> BOOL32
							{
								return InDocument.UpdateState(StateIndex, Clip->ClipName
									, State.LoopMode, State.PlayRate, State.BlendSeconds, OutError);
							}, EString("State clip updated."));
						bStateCommitted = TRUE;
						break;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
			}
			else
			{
				ImGui::TextDisabled("Animation asset is not loaded.");
			}
			ImGui::EndCombo();
		}
		if (bStateCommitted != FALSE)
		{
			return;
		}

		const EAnimationLoopMode LoopModes[4] = {
			ANIMATION_LOOP_ONCE, ANIMATION_LOOP_REPEAT, ANIMATION_LOOP_PINGPONG, ANIMATION_LOOP_CLAMP
		};
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##state_loop", GetLoopModeLabel(State.LoopMode)))
		{
			for (UINT32 ModeIndex = 0u; ModeIndex < 4u; ModeIndex++)
			{
				const BOOL8 bSelected = (State.LoopMode == LoopModes[ModeIndex]);
				if (ImGui::Selectable(GetLoopModeLabel(LoopModes[ModeIndex]), bSelected))
				{
					RunMutation(InDocument, [&InDocument, StateIndex, &State, &LoopModes, ModeIndex](EString& OutError) -> BOOL32
						{
							return InDocument.UpdateState(StateIndex, State.ClipName
								, LoopModes[ModeIndex], State.PlayRate, State.BlendSeconds, OutError);
						}, EString("State loop mode updated."));
					bStateCommitted = TRUE;
					break;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		if (bStateCommitted != FALSE)
		{
			return;
		}

		FLOAT PlayRate = State.PlayRate;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##state_rate", &PlayRate, 0.01f, 4.0f, "Rate %.2f", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			RunMutation(InDocument, [&InDocument, StateIndex, &State, PlayRate](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateState(StateIndex, State.ClipName
						, State.LoopMode, PlayRate, State.BlendSeconds, OutError);
				}, EString("State play rate updated."));
			return;
		}
		FLOAT BlendSeconds = State.BlendSeconds;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##state_blend", &BlendSeconds, 0.0f, 2.0f, "Blend %.3f", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			RunMutation(InDocument, [&InDocument, StateIndex, &State, BlendSeconds](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateState(StateIndex, State.ClipName
						, State.LoopMode, State.PlayRate, BlendSeconds, OutError);
				}, EString("State blend updated."));
			return;
		}

		if (ImGui::Button("Set Entry", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument, InStateName](EString& OutError) -> BOOL32
				{
					return InDocument.SetEntryState(InStateName, OutError);
				}, EString("Entry state updated."));
			return;
		}
		ImGui::BeginDisabled(Machine->States.Num() <= 1);
		if (ImGui::Button("Delete State", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument, InStateName](EString& OutError) -> BOOL32
				{
					return InDocument.DeleteState(InStateName, OutError);
				}, EString("State deleted."));
			return;
		}
		ImGui::EndDisabled();
	}

	void FStateMachineEditorPanel::DrawParameterInspector(FStateMachineDocument& InDocument, const EString& InParameterName)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		const INT32 ParameterIndex = FindParameterIndex(Machine, InParameterName);
		if ((!Machine) || (ParameterIndex < 0))
		{
			InDocument.Selection.Clear();
			return;
		}
		const EAnimStateMachineParameter& Parameter = Machine->Parameters[ParameterIndex];
		if (m_ParameterNameBufferCount != Machine->Parameters.Num())
		{
			m_ParameterNameBuffers.SetNum(Machine->Parameters.Num());
			for (INT32 Index = 0; Index < Machine->Parameters.Num(); Index++)
			{
				m_ParameterNameBuffers[Index].SetText(Machine->Parameters[Index].Name);
			}
			m_ParameterNameBufferCount = Machine->Parameters.Num();
		}

		ImGui::TextDisabled("Parameter");
		ImGui::SetNextItemWidth(-FLT_MIN);
		m_ParameterNameBuffers[ParameterIndex].InputText("##parameter_name");
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			const EString NewName = m_ParameterNameBuffers[ParameterIndex].ToString();
			if (NewName != Parameter.Name)
			{
				RunMutation(InDocument, [&InDocument, ParameterIndex, NewName](EString& OutError) -> BOOL32
					{
						return InDocument.RenameParameter(ParameterIndex, NewName, OutError);
					}, EString("Parameter renamed."));
				m_ParameterNameBufferCount = -1;
				return;
			}
		}

		const INT32 UsageCount = GetParameterUsageCount(InDocument, Parameter.Name);
		ImGui::TextDisabled("Used by %d condition(s)", UsageCount);
		const BOOL32 bTypeLocked = ((UsageCount > 0) ? TRUE : FALSE);
		BOOL32 bTypeCommitted = FALSE;
		ImGui::BeginDisabled(bTypeLocked != FALSE);
		ImGui::SetNextItemWidth(-FLT_MIN);
		if (ImGui::BeginCombo("##parameter_type", GetParameterTypeLabel(Parameter.Type)))
		{
			const EAnimStateMachineParameterType Types[3] = {
				ANIM_PARAM_FLOAT, ANIM_PARAM_BOOL, ANIM_PARAM_TRIGGER
			};
			for (UINT32 TypeIndex = 0u; TypeIndex < 3u; TypeIndex++)
			{
				const BOOL8 bSelected = (Parameter.Type == Types[TypeIndex]);
				if (ImGui::Selectable(GetParameterTypeLabel(Types[TypeIndex]), bSelected))
				{
					RunMutation(InDocument, [&InDocument, ParameterIndex, &Types, TypeIndex](EString& OutError) -> BOOL32
						{
							return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(Types[TypeIndex])
								, 0.0f, FALSE, OutError);
						}, EString("Parameter type updated."));
					m_ParameterNameBufferCount = -1;
					bTypeCommitted = TRUE;
					break;
				}
				if (bSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::EndDisabled();
		if (bTypeCommitted != FALSE)
		{
			return;
		}
		if (bTypeLocked != FALSE)
		{
			ImGui::SetItemTooltip("Remove condition references before changing this parameter's type.");
		}

		if (Parameter.Type == ANIM_PARAM_FLOAT)
		{
			FLOAT DefaultValue = Parameter.DefaultFloat;
			ImGui::SetNextItemWidth(-FLT_MIN);
			ImGui::InputFloat("##parameter_default", &DefaultValue, 0.0f, 0.0f, "%.3f");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				RunMutation(InDocument, [&InDocument, ParameterIndex, DefaultValue](EString& OutError) -> BOOL32
					{
						return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(ANIM_PARAM_FLOAT)
							, DefaultValue, FALSE, OutError);
					}, EString("Parameter default updated."));
				return;
			}
		}
		else if (Parameter.Type == ANIM_PARAM_BOOL)
		{
			bool bDefaultValue = (Parameter.DefaultBool != FALSE);
			if (ImGui::Checkbox("Default", &bDefaultValue))
			{
				RunMutation(InDocument, [&InDocument, ParameterIndex, bDefaultValue](EString& OutError) -> BOOL32
					{
						return InDocument.UpdateParameter(ParameterIndex, static_cast<UINT8>(ANIM_PARAM_BOOL)
							, 0.0f, (bDefaultValue ? TRUE : FALSE), OutError);
					}, EString("Parameter default updated."));
				return;
			}
		}
		else
		{
			ImGui::TextDisabled("Trigger has no persistent default value.");
		}

		if (ImGui::Button("Remove Parameter", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument, ParameterIndex](EString& OutError) -> BOOL32
				{
					return InDocument.RemoveParameter(ParameterIndex, OutError);
				}, EString("Parameter removed."));
			m_ParameterNameBufferCount = -1;
			return;
		}
	}

	void FStateMachineEditorPanel::DrawTransitionInspector(FStateMachineDocument& InDocument, INT32 InTransitionIndex)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if ((!Machine) || (InTransitionIndex < 0) || (InTransitionIndex >= Machine->Transitions.Num()))
		{
			InDocument.Selection.Clear();
			return;
		}
		const EAnimStateMachineTransition& Transition = Machine->Transitions[InTransitionIndex];
		const EString FromName = ((Transition.FromStateIndex < 0)
			? EString("Any") : Machine->States[Transition.FromStateIndex].Name);
		const EString ToName = ((Transition.ToStateIndex >= 0) && (Transition.ToStateIndex < Machine->States.Num())
			? Machine->States[Transition.ToStateIndex].Name : EString("?"));
		ImGui::TextDisabled("Transition %s -> %s", *FromName, *ToName);

		INT32 Priority = Transition.Priority;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::InputInt("##transition_priority", &Priority);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			RunMutation(InDocument, [&InDocument, InTransitionIndex, Priority, &Transition](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateTransition(InTransitionIndex, Priority, Transition.BlendSeconds
						, Transition.MinTimeInState, Transition.bCanTransitionToSelf, OutError);
				}, EString("Transition priority updated."));
			return;
		}
		FLOAT BlendSeconds = Transition.BlendSeconds;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##transition_blend", &BlendSeconds, 0.0f, 2.0f, "Blend %.3f", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			RunMutation(InDocument, [&InDocument, InTransitionIndex, &Transition, BlendSeconds](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateTransition(InTransitionIndex, Transition.Priority, BlendSeconds
						, Transition.MinTimeInState, Transition.bCanTransitionToSelf, OutError);
				}, EString("Transition blend updated."));
			return;
		}
		FLOAT MinTime = Transition.MinTimeInState;
		ImGui::SetNextItemWidth(-FLT_MIN);
		ImGui::SliderFloat("##transition_min_time", &MinTime, 0.0f, 5.0f, "Min time %.3f", ImGuiSliderFlags_AlwaysClamp);
		if (ImGui::IsItemDeactivatedAfterEdit())
		{
			RunMutation(InDocument, [&InDocument, InTransitionIndex, &Transition, MinTime](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateTransition(InTransitionIndex, Transition.Priority, Transition.BlendSeconds
						, MinTime, Transition.bCanTransitionToSelf, OutError);
				}, EString("Transition min time updated."));
			return;
		}
		bool bCanSelf = (Transition.bCanTransitionToSelf != FALSE);
		if (ImGui::Checkbox("Can Transition To Self", &bCanSelf))
		{
			RunMutation(InDocument, [&InDocument, InTransitionIndex, &Transition, bCanSelf](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateTransition(InTransitionIndex, Transition.Priority, Transition.BlendSeconds
						, Transition.MinTimeInState, (bCanSelf ? TRUE : FALSE), OutError);
				}, EString("Transition self flag updated."));
			return;
		}
		if (ImGui::Button("Delete Transition", ImVec2(-FLT_MIN, 0.0f)))
		{
			RunMutation(InDocument, [&InDocument, InTransitionIndex](EString& OutError) -> BOOL32
				{
					return InDocument.DeleteTransition(InTransitionIndex, OutError);
				}, EString("Transition deleted."));
			return;
		}
		ImGui::Separator();
		DrawConditions(InDocument, InTransitionIndex);
	}

	void FStateMachineEditorPanel::DrawConditions(FStateMachineDocument& InDocument, INT32 InTransitionIndex)
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if ((!Machine) || (InTransitionIndex < 0) || (InTransitionIndex >= Machine->Transitions.Num()))
		{
			return;
		}
		const EAnimStateMachineTransition& Transition = Machine->Transitions[InTransitionIndex];
		FImGuiIdScope ConditionScope("TransitionConditions");
		ImGui::TextDisabled("Conditions");

		if (Machine->Parameters.Num() <= 0)
		{
			ImGui::TextDisabled("Add a parameter before creating conditions.");
		}
		else if (Transition.Conditions.Num() <= 0)
		{
			ImGui::TextDisabled("Unconditional: this transition fires as soon as it is allowed.");
		}

		enum class EConditionActionType : UINT8
		{
			NONE = 0,
			UPDATE,
			REMOVE,
			ADD
		};
		EConditionActionType PendingAction = EConditionActionType::NONE;
		INT32 PendingConditionIndex = -1;
		INT32 PendingParameterIndex = -1;
		EString PendingOp;
		FLOAT PendingValue = 0.0f;

		if (Machine->Parameters.Num() > 0)
		{
			const ImGuiTableFlags TableFlags = ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV
				| ImGuiTableFlags_SizingStretchProp | ImGuiTableFlags_NoSavedSettings;
			if (ImGui::BeginTable("##sm_conditions", 4, TableFlags))
			{
				ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Op", ImGuiTableColumnFlags_WidthFixed, 64.0f);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 84.0f);
				ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed, 24.0f);
				ImGui::TableHeadersRow();

				for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
				{
					const EAnimStateMachineCondition& Condition = Transition.Conditions[ConditionIndex];
					const EAnimStateMachineParameter* Parameter =
						(((Condition.ParameterIndex >= 0) && (Condition.ParameterIndex < Machine->Parameters.Num()))
							? &Machine->Parameters[Condition.ParameterIndex] : nullptr);
					const EString ParameterName = (Parameter ? Parameter->Name : EString("None"));
					const EAnimStateMachineParameterType ParameterType =
						(Parameter ? Parameter->Type : ANIM_PARAM_FLOAT);
					const CHAR* OpText = GetCompareOpText(Condition.Compare);
					ImGui::PushID(ConditionIndex);
					ImGui::TableNextRow();

					ImGui::TableSetColumnIndex(0);
					ImGui::SetNextItemWidth(-FLT_MIN);
					if (ImGui::BeginCombo("##param", *ParameterName))
					{
						for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
						{
							ImGui::PushID(ParameterIndex);
							const BOOL8 bSelected = (ParameterIndex == Condition.ParameterIndex);
							if (ImGui::Selectable(*Machine->Parameters[ParameterIndex].Name, bSelected)
								&& (PendingAction == EConditionActionType::NONE))
							{
								const EAnimStateMachineParameter& NewParameter = Machine->Parameters[ParameterIndex];
								PendingAction = EConditionActionType::UPDATE;
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
					if (ImGui::BeginCombo("##op", OpText))
					{
						const CHAR* FloatOps[6] = { ">", ">=", "<", "<=", "==", "!=" };
						const EAnimStateMachineCompareType FloatCompares[6] = {
							ANIM_COMPARE_GREATER, ANIM_COMPARE_GREATER_EQUAL, ANIM_COMPARE_LESS
							, ANIM_COMPARE_LESS_EQUAL, ANIM_COMPARE_EQUAL, ANIM_COMPARE_NOT_EQUAL
						};
						const CHAR* BoolOps[2] = { "==", "!=" };
						const EAnimStateMachineCompareType BoolCompares[2] = {
							ANIM_COMPARE_EQUAL, ANIM_COMPARE_NOT_EQUAL
						};
						const CHAR* TriggerOp = "trigger";
						const EAnimStateMachineCompareType TriggerCompare = ANIM_COMPARE_TRIGGER;
						INT32 OpCount = 6;
						const CHAR** Ops = FloatOps;
						const EAnimStateMachineCompareType* Compares = FloatCompares;
						if (ParameterType == ANIM_PARAM_BOOL)
						{
							OpCount = 2;
							Ops = BoolOps;
							Compares = BoolCompares;
						}
						else if (ParameterType == ANIM_PARAM_TRIGGER)
						{
							OpCount = 1;
							Ops = &TriggerOp;
							Compares = &TriggerCompare;
						}
						for (INT32 OpIndex = 0; OpIndex < OpCount; OpIndex++)
						{
							const BOOL8 bSelected = (Condition.Compare == Compares[OpIndex]);
							if (ImGui::Selectable(Ops[OpIndex], bSelected)
								&& (PendingAction == EConditionActionType::NONE))
							{
								PendingAction = EConditionActionType::UPDATE;
								PendingConditionIndex = ConditionIndex;
								PendingParameterIndex = Condition.ParameterIndex;
								PendingOp = EString(Ops[OpIndex]);
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
						ImGui::InputFloat("##value", &Value, 0.0f, 0.0f, "%.3f");
						if ((ImGui::IsItemDeactivatedAfterEdit() != false)
							&& (PendingAction == EConditionActionType::NONE))
						{
							PendingAction = EConditionActionType::UPDATE;
							PendingConditionIndex = ConditionIndex;
							PendingParameterIndex = Condition.ParameterIndex;
							PendingOp = EString(OpText);
							PendingValue = Value;
						}
					}
					else if (ParameterType == ANIM_PARAM_BOOL)
					{
						bool bValue = (Condition.Value != 0.0f);
						if ((ImGui::Checkbox("##value", &bValue) != false)
							&& (PendingAction == EConditionActionType::NONE))
						{
							PendingAction = EConditionActionType::UPDATE;
							PendingConditionIndex = ConditionIndex;
							PendingParameterIndex = Condition.ParameterIndex;
							PendingOp = EString(OpText);
							PendingValue = (bValue ? 1.0f : 0.0f);
						}
					}
					else
					{
						ImGui::TextDisabled("trigger");
					}

					ImGui::TableSetColumnIndex(3);
					if ((ImGui::Button("x") != false) && (PendingAction == EConditionActionType::NONE))
					{
						PendingAction = EConditionActionType::REMOVE;
						PendingConditionIndex = ConditionIndex;
					}
					ImGui::PopID();
				}
				ImGui::EndTable();
			}
		}

		if (Machine->Parameters.Num() > 0)
		{
			if (ImGui::Button("Add Condition"))
			{
				ImGui::OpenPopup("##sm_add_condition");
			}
			if (ImGui::BeginPopup("##sm_add_condition"))
			{
				ImGui::TextDisabled("Choose a parameter:");
				for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
				{
					ImGui::PushID(ParameterIndex);
					if (ImGui::MenuItem(*Machine->Parameters[ParameterIndex].Name)
						&& (PendingAction == EConditionActionType::NONE))
					{
						PendingAction = EConditionActionType::ADD;
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
		}

		switch (PendingAction)
		{
		case EConditionActionType::UPDATE:
			RunMutation(InDocument, [&InDocument, InTransitionIndex, PendingConditionIndex
				, PendingParameterIndex, PendingOp, PendingValue](EString& OutError) -> BOOL32
				{
					return InDocument.UpdateTransitionCondition(InTransitionIndex, PendingConditionIndex
						, PendingParameterIndex, PendingOp, PendingValue, OutError);
				}, EString("Condition updated."));
			break;
		case EConditionActionType::REMOVE:
			RunMutation(InDocument, [&InDocument, InTransitionIndex, PendingConditionIndex](EString& OutError) -> BOOL32
				{
					return InDocument.RemoveTransitionCondition(InTransitionIndex, PendingConditionIndex, OutError);
				}, EString("Condition removed."));
			break;
		case EConditionActionType::ADD:
			RunMutation(InDocument, [&InDocument, InTransitionIndex, PendingParameterIndex](EString& OutError) -> BOOL32
				{
					return InDocument.AddTransitionCondition(InTransitionIndex, PendingParameterIndex, OutError);
				}, EString("Condition added."));
			break;
		default:
			break;
		}
	}

	void FStateMachineEditorPanel::DrawBottomPanel(FStateMachineDocument& InDocument)
	{
		ImGui::Separator();
		if (ImGui::BeginChild("##sm_bottom", ImVec2(0.0f, 170.0f), true))
		{
			if (ImGui::BeginTabBar("##sm_bottom_tabs"))
			{
				if (ImGui::BeginTabItem("Diagnostics"))
				{
					DrawDiagnostics(InDocument);
					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Source"))
				{
					DrawSource(InDocument);
					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndChild();
	}

	void FStateMachineEditorPanel::DrawDiagnostics(FStateMachineDocument& InDocument)
	{
		const TArray<FEditorDiagnostic>& Items = InDocument.Diagnostics.GetItems();
		if (Items.Num() <= 0)
		{
			ImGui::TextDisabled("No problems detected.");
			return;
		}
		for (INT32 DiagnosticIndex = 0; DiagnosticIndex < Items.Num(); DiagnosticIndex++)
		{
			const FEditorDiagnostic& Diagnostic = Items[DiagnosticIndex];
			const ImVec4 Color = (Diagnostic.Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_ERROR)
				? ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
				: ((Diagnostic.Severity == EEditorDiagnosticSeverity::DIAGNOSTIC_WARNING)
					? ImVec4(1.0f, 0.8f, 0.35f, 1.0f) : ImVec4(0.7f, 0.8f, 0.9f, 1.0f));
			ImGui::PushID(DiagnosticIndex);
			ImGui::TextColored(Color, "%s", *Diagnostic.Message);
			ImGui::SameLine();
			if (ImGui::SmallButton("Locate"))
			{
				EEditorSelectionType SelectionType = Diagnostic.ObjectType;
				if (SelectionType == EEditorSelectionType::NONE)
				{
					if (Diagnostic.SubIndex >= 0)
					{
						SelectionType = EEditorSelectionType::CONDITION;
					}
					else if (Diagnostic.ObjectId == EString("MACHINE"))
					{
						SelectionType = EEditorSelectionType::MACHINE;
					}
					else
					{
						SelectionType = EEditorSelectionType::STATE;
					}
				}
				InDocument.Selection.Type = SelectionType;
				InDocument.Selection.ObjectId = Diagnostic.ObjectId;
				InDocument.Selection.ObjectIndex = Diagnostic.ObjectIndex;
				InDocument.Selection.SubIndex = Diagnostic.SubIndex;
			}
			ImGui::PopID();
		}
	}

	void FStateMachineEditorPanel::DrawSource(FStateMachineDocument& InDocument)
	{
		if (m_SourceDocument != &InDocument)
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
			m_SourceDocument = &InDocument;
		}
		if (ImGui::Button("Apply Text"))
		{
			const EString SourceText = m_SourceBuffer.ToString();
			RunMutation(InDocument, [&InDocument, SourceText](EString& OutError) -> BOOL32
				{
					return InDocument.CommitSourceText(SourceText, OutError);
				}, EString("Source text applied."));
			if (InDocument.LastError.Length() <= 0u)
			{
				m_SourceBuffer.SetText(InDocument.DraftJson);
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert Text"))
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
			InDocument.LastError = EString();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("Structured edits and this text share the same draft.");
		m_SourceBuffer.InputTextMultiline("##sm_source", ImVec2(-1.0f, -1.0f)
			, ImGuiInputTextFlags_AllowTabInput);
	}

	void FStateMachineEditorPanel::RunMutation(FStateMachineDocument& InDocument
		, const TFunction<BOOL32(EString&)>& InMutation, const EString& InSuccessMessage)
	{
		InDocument.Commands.BeginTransaction(InDocument.CaptureSnapshot());
		EString Error;
		if (InMutation(Error))
		{
			InDocument.Commands.CommitTransaction(InDocument.CaptureSnapshot());
			InDocument.LastError = EString();
			m_StatusMessage = InSuccessMessage;
			m_bStatusSucceeded = TRUE;
			m_SourceDocument = nullptr;
		}
		else
		{
			InDocument.Commands.CancelTransaction();
			InDocument.LastError = Error;
			m_StatusMessage = Error;
			m_bStatusSucceeded = FALSE;
		}
	}

	void FStateMachineEditorPanel::ResetEditorBuffers()
	{
		m_StateNameBufferSource = EString();
		m_ParameterNameBufferCount = -1;
		m_SourceDocument = nullptr;
	}

	INT32 FStateMachineEditorPanel::GetParameterUsageCount(const FStateMachineDocument& InDocument
		, const EString& InParameterName)const
	{
		const EAnimStateMachine* Machine = InDocument.GetDraftMachine();
		if (!Machine)
		{
			return 0;
		}
		const INT32 ParameterIndex = Machine->FindParameter(InParameterName);
		if (ParameterIndex < 0)
		{
			return 0;
		}
		INT32 UsageCount = 0;
		for (INT32 TransitionIndex = 0; TransitionIndex < Machine->Transitions.Num(); TransitionIndex++)
		{
			const EAnimStateMachineTransition& Transition = Machine->Transitions[TransitionIndex];
			for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
			{
				if (Transition.Conditions[ConditionIndex].ParameterIndex == ParameterIndex)
				{
					UsageCount++;
				}
			}
		}
		return UsageCount;
	}

}

#endif
