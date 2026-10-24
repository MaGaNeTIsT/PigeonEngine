#include "AnimationStateMachineEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <Editor/EditorLogManager.h>
#include <RapidJson/Headers/JsonObject.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>
#include <cstring>

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

		const CHAR* GetDefaultConditionOp(const EAnimStateMachineParameterType InType)
		{
			switch (InType)
			{
			case ANIM_PARAM_BOOL:
				return "==";
			case ANIM_PARAM_TRIGGER:
				return "trigger";
			case ANIM_PARAM_FLOAT:
			default:
				return ">";
			}
		}

		EString GetJsonStringField(const rapidjson::Value& InObject, const CHAR* InFieldName)
		{
			if (!InObject.IsObject())
			{
				return EString();
			}
			auto It = InObject.FindMember(InFieldName);
			if ((It != InObject.MemberEnd()) && (It->value.IsString()))
			{
				return EString(It->value.GetString());
			}
			return EString();
		}

		BOOL32 JsonStateNameExists(const rapidjson::Value& InStates, const EString& InName)
		{
			if (!InStates.IsArray())
			{
				return FALSE;
			}
			for (auto& State : InStates.GetArray())
			{
				if (GetJsonStringField(State, "name") == InName)
				{
					return TRUE;
				}
			}
			return FALSE;
		}

		BOOL32 SerializeJsonDocument(const rapidjson::Document& InDocument, EString& OutText)
		{
			rapidjson::StringBuffer Buffer;
			rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
			InDocument.Accept(Writer);
			OutText = EString(Buffer.GetString());
			return TRUE;
		}
	}

	void EAnimStateMachineEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
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
		m_Asset = nullptr;
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
		}
	}

	void EAnimStateMachineEditor::EditorUpdate()
	{
		if (!m_ShowEditor)
		{
			return;
		}

		ImGui::SetNextWindowSize(ImVec2(900.0f, 720.0f), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(420.0f, 40.0f), ImGuiCond_FirstUseEver);
		ImGui::Begin("State Machine Editor", &m_ShowEditor, ImGuiWindowFlags_None);
		{
			if (ImGui::Button("Reload") && (m_OpenedAssetFullPath.Length() > 0u))
			{
				OpenStateMachineAsset(m_OpenedAssetFullPath);
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
					}
					else
					{
						m_LastError = Error;
					}
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (m_Asset)
				{
					WriteNodePositionsToJson();
					EString Error;
					const EString JsonText(m_JsonBuffer.GetData());
					if (EAnimStateMachineManager::GetManagerSingleton()->SaveStateMachineAsset(m_Asset, JsonText, Error))
					{
						m_LastError = EString();
					}
					else
					{
						m_LastError = Error;
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
			}
			ImGui::SameLine();
			ImGui::TextWrapped("%s", (m_OpenedAssetFullPath.Length() > 0u ? *m_OpenedAssetFullPath : "No state machine selected."));

			if (m_LastError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.35f, 0.35f, 1.0f), "%s", *m_LastError);
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
				ImGui::InputTextMultiline("JSON", m_JsonBuffer.GetData(), static_cast<int>(m_JsonBuffer.Num())
					, ImVec2(-1.0f, -1.0f), ImGuiInputTextFlags_AllowTabInput);
			}
		}
		ImGui::End();
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
		auto DrawArrow = [DrawList](const ImVec2& InFrom, const ImVec2& InTo, const ImU32 InColor)
		{
			DrawList->AddLine(InFrom, InTo, InColor, 2.0f);
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
					DrawList->AddLine(Top, LoopLeft, EdgeColor, 2.0f);
					DrawList->AddLine(LoopLeft, LoopRight, EdgeColor, 2.0f);
					DrawArrow(LoopRight, Top, EdgeColor);
					CHAR SelfLabel[48] = "";
					::sprintf_s(SelfLabel, sizeof(SelfLabel), "p%d c%d", Transition.Priority, Transition.Conditions.Num());
					DrawList->AddText(ImVec2(LoopLeft.x, LoopLeft.y - 16.0f), EdgeColor, SelfLabel);
					continue;
				}
				From = ImVec2(Origin.x + FromLocal.x + NodeWidth, Origin.y + FromLocal.y + NodeHeight * 0.5f);
			}

			DrawArrow(From, To, EdgeColor);
			CHAR Label[64] = "";
			::sprintf_s(Label, sizeof(Label), "p%d c%d", Transition.Priority, Transition.Conditions.Num());
			DrawList->AddText(ImVec2((From.x + To.x) * 0.5f, (From.y + To.y) * 0.5f - 14.0f), EdgeColor, Label);
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
			if ((ImGui::IsItemActive()) && (StoredPosition))
			{
				const ImVec2 MouseDelta = ImGui::GetIO().MouseDelta;
				StoredPosition->x += MouseDelta.x;
				StoredPosition->y += MouseDelta.y;
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			return;
		}
		auto StatesIt = Doc.FindMember("states");
		if ((StatesIt == Doc.MemberEnd()) || (!StatesIt->value.IsArray()))
		{
			return;
		}
		for (auto& StateValue : StatesIt->value.GetArray())
		{
			if (!StateValue.IsObject())
			{
				continue;
			}
			auto NameIt = StateValue.FindMember("name");
			auto XIt = StateValue.FindMember("nodeX");
			auto YIt = StateValue.FindMember("nodeY");
			if ((NameIt == StateValue.MemberEnd()) || (!NameIt->value.IsString())
				|| (XIt == StateValue.MemberEnd()) || (!XIt->value.IsNumber())
				|| (YIt == StateValue.MemberEnd()) || (!YIt->value.IsNumber()))
			{
				continue;
			}
			const EString StateName(NameIt->value.GetString());
			if (StateName.Length() > 0u)
			{
				m_NodePositions.Add(StateName, ImVec2(XIt->value.GetFloat(), YIt->value.GetFloat()));
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			return;
		}
		auto StatesIt = Doc.FindMember("states");
		if ((StatesIt == Doc.MemberEnd()) || (!StatesIt->value.IsArray()))
		{
			return;
		}
		for (auto& StateValue : StatesIt->value.GetArray())
		{
			if (!StateValue.IsObject())
			{
				continue;
			}
			auto NameIt = StateValue.FindMember("name");
			if ((NameIt == StateValue.MemberEnd()) || (!NameIt->value.IsString()))
			{
				continue;
			}
			const EString StateName(NameIt->value.GetString());
			const ImVec2* Position = m_NodePositions.FindValueAsPtr(StateName);
			if (!Position)
			{
				continue;
			}

			auto XIt = StateValue.FindMember("nodeX");
			if (XIt == StateValue.MemberEnd())
			{
				rapidjson::Value XValue(static_cast<double>(Position->x));
				StateValue.AddMember("nodeX", XValue, Doc.GetAllocator());
			}
			else
			{
				XIt->value.SetDouble(static_cast<double>(Position->x));
			}
			auto YIt = StateValue.FindMember("nodeY");
			if (YIt == StateValue.MemberEnd())
			{
				rapidjson::Value YValue(static_cast<double>(Position->y));
				StateValue.AddMember("nodeY", YValue, Doc.GetAllocator());
			}
			else
			{
				YIt->value.SetDouble(static_cast<double>(Position->y));
			}
		}

		rapidjson::StringBuffer Buffer;
		rapidjson::Writer<rapidjson::StringBuffer> Writer(Buffer);
		Doc.Accept(Writer);
		const EString Updated(Buffer.GetString());
		m_JsonBuffer.Empty();
		const INT32 Length = static_cast<INT32>(Updated.Length()) + StateMachineEditorBufferHeadroom + 1;
		m_JsonBuffer.SetNum(Length);
		::memset(m_JsonBuffer.GetData(), 0, static_cast<size_t>(Length));
		if (Updated.Length() > 0u)
		{
			::memcpy_s(m_JsonBuffer.GetData(), static_cast<rsize_t>(Length), *Updated, Updated.Length());
		}
	}

	void EAnimStateMachineEditor::DrawStructureControls(const EAnimStateMachine& InMachine)
	{
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

		m_TransitionFromIndex = EMath::Clamp(m_TransitionFromIndex, -1, InMachine.States.Num() - 1);
		m_TransitionToIndex = EMath::Clamp(m_TransitionToIndex, 0, InMachine.States.Num() - 1);
		const EString FromLabel = ((m_TransitionFromIndex < 0) ? EString("Any") : InMachine.States[m_TransitionFromIndex].Name);
		if (ImGui::BeginCombo("From", *FromLabel))
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
		ImGui::SameLine();
		const EString& ToLabel = InMachine.States[m_TransitionToIndex].Name;
		if (ImGui::BeginCombo("To", *ToLabel))
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
		ImGui::SameLine();
		if (ImGui::Button("Add Transition"))
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
			ImGui::TextDisabled("Select a transition above to edit its conditions.");
			return;
		}

		const EAnimStateMachineTransition& Transition = InMachine.Transitions[m_SelectedTransitionIndex];
		const EString FromName = ((Transition.FromStateIndex < 0)
			? EString("Any") : InMachine.States[Transition.FromStateIndex].Name);
		const EString ToName = ((Transition.ToStateIndex >= 0) && (Transition.ToStateIndex < InMachine.States.Num())
			? InMachine.States[Transition.ToStateIndex].Name : EString("?"));
		ImGui::Separator();
		ImGui::Text("Conditions: %s -> %s", *FromName, *ToName);
		if (InMachine.Parameters.Num() <= 0)
		{
			ImGui::TextDisabled("This machine has no parameters to test.");
			return;
		}
		if (Transition.Conditions.Num() <= 0)
		{
			ImGui::TextDisabled("No conditions: this transition is unconditional.");
		}

		for (INT32 ConditionIndex = 0; ConditionIndex < Transition.Conditions.Num(); ConditionIndex++)
		{
			const EAnimStateMachineCondition& Condition = Transition.Conditions[ConditionIndex];
			const EString ParameterName = ((Condition.ParameterIndex >= 0) && (Condition.ParameterIndex < InMachine.Parameters.Num())
				? InMachine.Parameters[Condition.ParameterIndex].Name : EString("None"));
			const CHAR* OpText = GetCompareOpText(Condition.Compare);
			ImGui::PushID(ConditionIndex);

			if (ImGui::BeginCombo("Param", *ParameterName))
			{
				INT32 NewParameterIndex = -1;
				for (INT32 ParameterIndex = 0; ParameterIndex < InMachine.Parameters.Num(); ParameterIndex++)
				{
					const BOOL8 bSelected = (ParameterIndex == Condition.ParameterIndex);
					if (ImGui::Selectable(*InMachine.Parameters[ParameterIndex].Name, bSelected))
					{
						NewParameterIndex = ParameterIndex;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
				if (NewParameterIndex >= 0)
				{
					UpdateTransitionCondition(m_SelectedTransitionIndex, ConditionIndex, NewParameterIndex
						, EString(GetDefaultConditionOp(InMachine.Parameters[NewParameterIndex].Type)), 0.0f);
					ImGui::PopID();
					return;
				}
			}
			ImGui::SameLine();
			if (ImGui::BeginCombo("Op", OpText))
			{
				const CHAR* CompareOps[] = { ">", ">=", "<", "<=", "==", "!=", "trigger" };
				EString NewOp;
				for (const CHAR* CompareOp : CompareOps)
				{
					const BOOL8 bSelected = (EString(CompareOp) == EString(OpText));
					if (ImGui::Selectable(CompareOp, bSelected))
					{
						NewOp = EString(CompareOp);
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
				if (NewOp.Length() > 0u)
				{
					UpdateTransitionCondition(m_SelectedTransitionIndex, ConditionIndex
						, Condition.ParameterIndex, NewOp, Condition.Value);
					ImGui::PopID();
					return;
				}
			}
			ImGui::SameLine();
			FLOAT Value = Condition.Value;
			ImGui::SetNextItemWidth(120.0f);
			ImGui::InputFloat("Value", &Value, 0.0f, 0.0f, "%.3f");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				UpdateTransitionCondition(m_SelectedTransitionIndex, ConditionIndex
					, Condition.ParameterIndex, EString(OpText), Value);
				ImGui::PopID();
				return;
			}
			ImGui::SameLine();
			if (ImGui::Button("Remove"))
			{
				RemoveTransitionCondition(m_SelectedTransitionIndex, ConditionIndex);
				ImGui::PopID();
				return;
			}
			ImGui::PopID();
		}

		if (ImGui::Button("Add Condition"))
		{
			AddTransitionCondition(m_SelectedTransitionIndex, 0);
			return;
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt == Doc.MemberEnd()) || (!TransitionsIt->value.IsArray())
			|| (InTransitionIndex >= static_cast<INT32>(TransitionsIt->value.Size())))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}
		rapidjson::Value& Transition = TransitionsIt->value[InTransitionIndex];
		if (!Transition.IsObject())
		{
			m_LastError = EString("JSON transition is not an object.");
			return FALSE;
		}

		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		auto ConditionsIt = Transition.FindMember("conditions");
		if ((ConditionsIt == Transition.MemberEnd()) || (!ConditionsIt->value.IsArray()))
		{
			rapidjson::Value EmptyConditions(rapidjson::kArrayType);
			if (ConditionsIt == Transition.MemberEnd())
			{
				Transition.AddMember("conditions", EmptyConditions, Allocator);
			}
			else
			{
				ConditionsIt->value.Swap(EmptyConditions);
			}
			ConditionsIt = Transition.FindMember("conditions");
		}

		const EAnimStateMachineParameter& Parameter = Machine->Parameters[InParameterIndex];
		rapidjson::Value Condition(rapidjson::kObjectType);
		Condition.AddMember("param", rapidjson::Value(*Parameter.Name, static_cast<rapidjson::SizeType>(Parameter.Name.Length()), Allocator), Allocator);
		const CHAR* DefaultOp = GetDefaultConditionOp(Parameter.Type);
		Condition.AddMember("op", rapidjson::Value(DefaultOp, static_cast<rapidjson::SizeType>(strlen(DefaultOp)), Allocator), Allocator);
		Condition.AddMember("value", rapidjson::Value(0.0), Allocator);
		ConditionsIt->value.PushBack(Condition, Allocator);

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = EString();
		return TRUE;
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt == Doc.MemberEnd()) || (!TransitionsIt->value.IsArray())
			|| (InTransitionIndex >= static_cast<INT32>(TransitionsIt->value.Size())))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}
		rapidjson::Value& Transition = TransitionsIt->value[InTransitionIndex];
		auto ConditionsIt = Transition.FindMember("conditions");
		if ((ConditionsIt == Transition.MemberEnd()) || (!ConditionsIt->value.IsArray())
			|| (InConditionIndex < 0) || (InConditionIndex >= static_cast<INT32>(ConditionsIt->value.Size())))
		{
			m_LastError = EString("JSON condition index is invalid.");
			return FALSE;
		}
		rapidjson::Value& Condition = ConditionsIt->value[InConditionIndex];
		if (!Condition.IsObject())
		{
			m_LastError = EString("JSON condition is not an object.");
			return FALSE;
		}

		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		const EString& ParameterName = Machine->Parameters[InParameterIndex].Name;
		auto ParameterIt = Condition.FindMember("param");
		if (ParameterIt == Condition.MemberEnd())
		{
			Condition.AddMember("param", rapidjson::Value(*ParameterName, static_cast<rapidjson::SizeType>(ParameterName.Length()), Allocator), Allocator);
		}
		else
		{
			ParameterIt->value.SetString(*ParameterName, static_cast<rapidjson::SizeType>(ParameterName.Length()), Allocator);
		}
		auto OpIt = Condition.FindMember("op");
		if (OpIt == Condition.MemberEnd())
		{
			Condition.AddMember("op", rapidjson::Value(*InOp, static_cast<rapidjson::SizeType>(InOp.Length()), Allocator), Allocator);
		}
		else
		{
			OpIt->value.SetString(*InOp, static_cast<rapidjson::SizeType>(InOp.Length()), Allocator);
		}
		auto ValueIt = Condition.FindMember("value");
		if (ValueIt == Condition.MemberEnd())
		{
			Condition.AddMember("value", rapidjson::Value(static_cast<double>(InValue)), Allocator);
		}
		else
		{
			ValueIt->value.SetDouble(static_cast<double>(InValue));
		}

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = EString();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::RemoveTransitionCondition(INT32 InTransitionIndex, INT32 InConditionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt == Doc.MemberEnd()) || (!TransitionsIt->value.IsArray())
			|| (InTransitionIndex < 0) || (InTransitionIndex >= static_cast<INT32>(TransitionsIt->value.Size())))
		{
			m_LastError = EString("JSON transition index is invalid.");
			return FALSE;
		}
		rapidjson::Value& Transition = TransitionsIt->value[InTransitionIndex];
		auto ConditionsIt = Transition.FindMember("conditions");
		if ((ConditionsIt == Transition.MemberEnd()) || (!ConditionsIt->value.IsArray())
			|| (InConditionIndex < 0) || (InConditionIndex >= static_cast<INT32>(ConditionsIt->value.Size())))
		{
			m_LastError = EString("JSON condition index is invalid.");
			return FALSE;
		}

		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		rapidjson::Value NewConditions(rapidjson::kArrayType);
		rapidjson::Value& Conditions = ConditionsIt->value;
		for (rapidjson::SizeType Index = 0; Index < Conditions.Size(); Index++)
		{
			if (static_cast<INT32>(Index) == InConditionIndex)
			{
				continue;
			}
			NewConditions.PushBack(Conditions[Index], Allocator);
		}
		ConditionsIt->value.Swap(NewConditions);

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = EString();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::AddState(const EAnimStateMachine& InMachine)
	{
		if ((!m_Asset) || (InMachine.States.Num() <= 0))
		{
			m_LastError = EString("Cannot add a state to an empty machine.");
			return FALSE;
		}

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto StatesIt = Doc.FindMember("states");
		if ((StatesIt == Doc.MemberEnd()) || (!StatesIt->value.IsArray()))
		{
			m_LastError = EString("JSON has no states array.");
			return FALSE;
		}
		rapidjson::Value& States = StatesIt->value;
		INT32 Suffix = static_cast<INT32>(States.Size());
		EString NewName;
		do
		{
			NewName = EString("State") + ToString(Suffix++);
		} while (JsonStateNameExists(States, NewName));

		EString ClipName;
		if (States.Size() > 0)
		{
			ClipName = GetJsonStringField(States[0], "clip");
		}

		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		rapidjson::Value State(rapidjson::kObjectType);
		State.AddMember("name", rapidjson::Value(*NewName, static_cast<rapidjson::SizeType>(NewName.Length()), Allocator), Allocator);
		State.AddMember("clip", rapidjson::Value(*ClipName, static_cast<rapidjson::SizeType>(ClipName.Length()), Allocator), Allocator);
		State.AddMember("loop", rapidjson::Value("Repeat", 6, Allocator), Allocator);
		State.AddMember("rate", rapidjson::Value(1.0), Allocator);
		State.AddMember("blend", rapidjson::Value(0.2), Allocator);
		States.PushBack(State, Allocator);

		auto EntryIt = Doc.FindMember("entry");
		if ((States.Size() == 1) && ((EntryIt == Doc.MemberEnd()) || (GetJsonStringField(Doc, "entry").Length() == 0u)))
		{
			if (EntryIt == Doc.MemberEnd())
			{
				Doc.AddMember("entry", rapidjson::Value(*NewName, static_cast<rapidjson::SizeType>(NewName.Length()), Allocator), Allocator);
			}
			else
			{
				EntryIt->value.SetString(*NewName, static_cast<rapidjson::SizeType>(NewName.Length()), Allocator);
			}
		}

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto StatesIt = Doc.FindMember("states");
		if ((StatesIt == Doc.MemberEnd()) || (!StatesIt->value.IsArray()))
		{
			m_LastError = EString("JSON has no states array.");
			return FALSE;
		}
		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		rapidjson::Value NewStates(rapidjson::kArrayType);
		for (auto& State : StatesIt->value.GetArray())
		{
			if (GetJsonStringField(State, "name") != DeletedStateName)
			{
				NewStates.PushBack(State, Allocator);
			}
		}
		StatesIt->value.Swap(NewStates);

		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt != Doc.MemberEnd()) && (TransitionsIt->value.IsArray()))
		{
			rapidjson::Value NewTransitions(rapidjson::kArrayType);
			for (auto& Transition : TransitionsIt->value.GetArray())
			{
				if ((GetJsonStringField(Transition, "from") == DeletedStateName)
					|| (GetJsonStringField(Transition, "to") == DeletedStateName))
				{
					continue;
				}
				NewTransitions.PushBack(Transition, Allocator);
			}
			TransitionsIt->value.Swap(NewTransitions);
		}

		const EString EntryName = GetJsonStringField(Doc, "entry");
		if ((EntryName == DeletedStateName) || (EntryName.Length() == 0u))
		{
			EString FirstStateName;
			if ((StatesIt->value.IsArray()) && (StatesIt->value.Size() > 0))
			{
				FirstStateName = GetJsonStringField(StatesIt->value[0], "name");
			}
			auto EntryIt = Doc.FindMember("entry");
			if (FirstStateName.Length() > 0u)
			{
				if (EntryIt == Doc.MemberEnd())
				{
					Doc.AddMember("entry", rapidjson::Value(*FirstStateName, static_cast<rapidjson::SizeType>(FirstStateName.Length()), Allocator), Allocator);
				}
				else
				{
					EntryIt->value.SetString(*FirstStateName, static_cast<rapidjson::SizeType>(FirstStateName.Length()), Allocator);
				}
			}
		}

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
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

		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt == Doc.MemberEnd()) || (!TransitionsIt->value.IsArray()))
		{
			rapidjson::Value EmptyTransitions(rapidjson::kArrayType);
			if (TransitionsIt == Doc.MemberEnd())
			{
				Doc.AddMember("transitions", EmptyTransitions, Allocator);
			}
			else
			{
				TransitionsIt->value.Swap(EmptyTransitions);
			}
			TransitionsIt = Doc.FindMember("transitions");
		}

		rapidjson::Value Conditions(rapidjson::kArrayType);
		const EAnimStateMachine* Machine = m_Asset->GetStoragedResource();
		if (Machine)
		{
			for (INT32 ParameterIndex = 0; ParameterIndex < Machine->Parameters.Num(); ParameterIndex++)
			{
				if (Machine->Parameters[ParameterIndex].Type != ANIM_PARAM_FLOAT)
				{
					continue;
				}
				const EString& ParameterName = Machine->Parameters[ParameterIndex].Name;
				rapidjson::Value Condition(rapidjson::kObjectType);
				Condition.AddMember("param", rapidjson::Value(*ParameterName, static_cast<rapidjson::SizeType>(ParameterName.Length()), Allocator), Allocator);
				Condition.AddMember("op", rapidjson::Value(">", 1, Allocator), Allocator);
				Condition.AddMember("value", rapidjson::Value(1000000000.0), Allocator);
				Conditions.PushBack(Condition, Allocator);
				break;
			}
		}

		const BOOL32 bHasDefaultCondition = ((Conditions.Size() > 0) ? TRUE : FALSE);
		rapidjson::Value Transition(rapidjson::kObjectType);
		Transition.AddMember("from", rapidjson::Value(*InFromStateName, static_cast<rapidjson::SizeType>(InFromStateName.Length()), Allocator), Allocator);
		Transition.AddMember("to", rapidjson::Value(*InToStateName, static_cast<rapidjson::SizeType>(InToStateName.Length()), Allocator), Allocator);
		Transition.AddMember("priority", rapidjson::Value(0), Allocator);
		Transition.AddMember("blend", rapidjson::Value(0.2), Allocator);
		Transition.AddMember("conditions", Conditions, Allocator);
		TransitionsIt->value.PushBack(Transition, Allocator);

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = ((bHasDefaultCondition) ? EString() : EString("Transition added without conditions; edit the JSON before it runs."));
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::DeleteTransition(INT32 InTransitionIndex)
	{
		if (!m_Asset)
		{
			return FALSE;
		}
		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		auto TransitionsIt = Doc.FindMember("transitions");
		if ((TransitionsIt == Doc.MemberEnd()) || (!TransitionsIt->value.IsArray())
			|| (InTransitionIndex < 0) || (InTransitionIndex >= static_cast<INT32>(TransitionsIt->value.Size())))
		{
			m_LastError = EString("Transition index is invalid.");
			return FALSE;
		}

		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		rapidjson::Value NewTransitions(rapidjson::kArrayType);
		rapidjson::Value& Transitions = TransitionsIt->value;
		for (rapidjson::SizeType Index = 0; Index < Transitions.Size(); Index++)
		{
			if (static_cast<INT32>(Index) == InTransitionIndex)
			{
				continue;
			}
			NewTransitions.PushBack(Transitions[Index], Allocator);
		}
		TransitionsIt->value.Swap(NewTransitions);

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = EString();
		return TRUE;
	}

	BOOL32 EAnimStateMachineEditor::SetEntryState(const EString& InStateName)
	{
		if ((!m_Asset) || (InStateName.Length() <= 0u))
		{
			m_LastError = EString("No entry state selected.");
			return FALSE;
		}
		rapidjson::Document Doc;
		Doc.Parse(m_JsonBuffer.GetData());
		if ((Doc.HasParseError()) || (!Doc.IsObject()))
		{
			m_LastError = EString("JSON cannot be parsed.");
			return FALSE;
		}
		rapidjson::Document::AllocatorType& Allocator = Doc.GetAllocator();
		auto EntryIt = Doc.FindMember("entry");
		if (EntryIt == Doc.MemberEnd())
		{
			Doc.AddMember("entry", rapidjson::Value(*InStateName, static_cast<rapidjson::SizeType>(InStateName.Length()), Allocator), Allocator);
		}
		else
		{
			EntryIt->value.SetString(*InStateName, static_cast<rapidjson::SizeType>(InStateName.Length()), Allocator);
		}

		EString NewText;
		EString Error;
		SerializeJsonDocument(Doc, NewText);
		if (!EAnimStateMachineManager::GetManagerSingleton()->RebuildStateMachineAsset(m_Asset, NewText, Error))
		{
			m_LastError = Error;
			return FALSE;
		}
		ReplaceJsonBuffer(NewText);
		m_LastError = EString();
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
