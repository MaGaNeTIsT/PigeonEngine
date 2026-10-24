#include "AnimationStateMachineEditor.h"

#if _EDITOR_ONLY

#include <imgui.h>

#include <AnimationStateMachineAsset/AnimationStateMachineAsset.h>
#include <Base/DataStructure/Text/Path.h>
#include <RapidJson/Headers/JsonObject.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <cmath>

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
	}

	void EAnimStateMachineEditor::EditorInit()
	{
		m_ShowEditor = FALSE;
		m_Asset = nullptr;
		m_OpenedAssetFullPath = EString();
		m_LastError = EString();
		m_JsonBuffer.Empty();
		m_NodePositions.Empty();
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
