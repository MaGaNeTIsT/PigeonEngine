#include "SlotGroupEditorPanel.h"

#if ENGINE_EDITOR_ONLY

#include <SkeletonAsset/SkeletonAsset.h>
#include <imgui.h>
#include <cfloat>
#include <cstdio>

namespace PigeonEngine
{

	void FSlotGroupEditorPanel::Draw(FSlotGroupDocument& InDocument)
	{
		RefreshBuffers(InDocument);
		EnsureSelection(InDocument);
		DrawToolbar(InDocument);

		const FLOAT TotalHeight = EMath::Max(0.0f, ImGui::GetContentRegionAvail().y);
		const FLOAT SplitterCost = FEditorSplitter::GetCost();
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		if (m_bShowOutput != FALSE)
		{
			const FLOAT MaxBottom = EMath::Max(MinBottom, TotalHeight - SplitterCost);
			m_BottomHeight = EMath::Clamp(m_BottomHeight, MinBottom, MaxBottom);
			const FLOAT EditorHeight = EMath::Max(0.0f, TotalHeight - m_BottomHeight - SplitterCost);
			if (EditorHeight > 0.0f)
			{
				DrawEditor(InDocument, EditorHeight);
			}
			DrawBottomPanel(InDocument, TotalHeight);
		}
		else
		{
			DrawEditor(InDocument, TotalHeight);
		}
	}

	void FSlotGroupEditorPanel::EnsureSelection(FSlotGroupDocument& InDocument)
	{
		const TArray<ESkeletonSlotGroup>& Groups = InDocument.GetDraftGroups();
		if (Groups.Num() <= 0)
		{
			m_SelectedGroupIndex = -1;
			m_SelectedSlotIndex = -1;
			InDocument.Selection.Clear();
			return;
		}
		if ((InDocument.Selection.Is(EEditorSelectionType::SLOT_GROUP))
			&& (InDocument.Selection.ObjectIndex >= 0)
			&& (InDocument.Selection.ObjectIndex < Groups.Num()))
		{
			m_SelectedGroupIndex = InDocument.Selection.ObjectIndex;
		}
		else
		{
			m_SelectedGroupIndex = EMath::Clamp(m_SelectedGroupIndex, 0, Groups.Num() - 1);
			InDocument.Selection.Type = EEditorSelectionType::SLOT_GROUP;
			InDocument.Selection.ObjectIndex = m_SelectedGroupIndex;
		}
		const INT32 SlotCount = Groups[m_SelectedGroupIndex].SlotNames.Num();
		if ((m_SelectedSlotIndex < 0) || (m_SelectedSlotIndex >= SlotCount))
		{
			m_SelectedSlotIndex = -1;
		}
		InDocument.Selection.SubIndex = m_SelectedSlotIndex;
	}

	void FSlotGroupEditorPanel::RefreshBuffers(FSlotGroupDocument& InDocument)
	{
		if (m_BufferSource != InDocument.AssetPath)
		{
			m_BufferSource = InDocument.AssetPath;
			m_bBuffersDirty = TRUE;
		}
		if (m_bBuffersDirty == FALSE)
		{
			return;
		}
		const TArray<ESkeletonSlotGroup>& Groups = InDocument.GetDraftGroups();
		m_GroupNameBuffers.SetNum(Groups.Num());
		m_SlotNameBuffers.SetNum(Groups.Num());
		for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
		{
			m_GroupNameBuffers[GroupIndex].SetText(Groups[GroupIndex].Name);
			m_SlotNameBuffers[GroupIndex].SetNum(Groups[GroupIndex].SlotNames.Num());
			for (INT32 SlotIndex = 0; SlotIndex < Groups[GroupIndex].SlotNames.Num(); SlotIndex++)
			{
				m_SlotNameBuffers[GroupIndex][SlotIndex].SetText(Groups[GroupIndex].SlotNames[SlotIndex]);
			}
		}
		m_bBuffersDirty = FALSE;
	}

	void FSlotGroupEditorPanel::InvalidateBuffers()
	{
		m_bBuffersDirty = TRUE;
	}

	void FSlotGroupEditorPanel::DrawToolbar(FSlotGroupDocument& InDocument)
	{
		FImGuiIdScope ToolbarScope("SlotGroupToolbar");
		ImGui::Text("%s", *InDocument.DisplayName);
		ImGui::SameLine();
		ImGui::TextDisabled("Skeleton Slot Groups");
		if (InDocument.IsDirty() != FALSE)
		{
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(1.0f, 0.75f, 0.35f, 1.0f), "unsaved");
		}

		auto DoUndo = [this, &InDocument]()
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Undo(InDocument.CaptureSnapshot(), Snapshot))
			{
				RestoreSnapshot(InDocument, Snapshot);
				m_StatusMessage = EString("Undo.");
				m_bStatusSucceeded = TRUE;
			}
		};
		auto DoRedo = [this, &InDocument]()
		{
			FEditorSnapshot Snapshot;
			if (InDocument.Commands.Redo(InDocument.CaptureSnapshot(), Snapshot))
			{
				RestoreSnapshot(InDocument, Snapshot);
				m_StatusMessage = EString("Redo.");
				m_bStatusSucceeded = TRUE;
			}
		};

		ImGui::SameLine();
		if (ImGui::Button("Apply"))
		{
			EString Error;
			if (InDocument.Apply(Error))
			{
				m_StatusMessage = EString("Slot groups applied to the live skeleton.");
				m_bStatusSucceeded = TRUE;
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
				m_StatusMessage = EString("Skeleton slot groups saved.");
				m_bStatusSucceeded = TRUE;
			}
			else
			{
				m_StatusMessage = Error;
				m_bStatusSucceeded = FALSE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Undo") && (InDocument.Commands.CanUndo() != FALSE))
		{
			DoUndo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Redo") && (InDocument.Commands.CanRedo() != FALSE))
		{
			DoRedo();
		}
		ImGui::SameLine();
		if (ImGui::Button("Reload"))
		{
			if (InDocument.IsDirty() != FALSE)
			{
				ImGui::OpenPopup("##slot_group_reload_confirm");
			}
			else
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Slot groups reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_BufferSource = EString();
					m_SourceDocument = nullptr;
					InvalidateBuffers();
				}
				else
				{
					m_StatusMessage = Error;
					m_bStatusSucceeded = FALSE;
				}
			}
		}
		ImGui::SameLine();
		ImGui::Checkbox("Output", &m_bShowOutput);

		const ImGuiIO& IO = ImGui::GetIO();
		if ((IO.WantTextInput == false) && (IO.KeyCtrl != false)
			&& (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) != false))
		{
			if ((ImGui::IsKeyPressed(ImGuiKey_Z, false) != false)
				&& (InDocument.Commands.CanUndo() != FALSE))
			{
				DoUndo();
			}
			else if ((ImGui::IsKeyPressed(ImGuiKey_Y, false) != false)
				&& (InDocument.Commands.CanRedo() != FALSE))
			{
				DoRedo();
			}
		}

		if (m_StatusMessage.Length() > 0u)
		{
			ImGui::TextColored((m_bStatusSucceeded != FALSE)
				? ImVec4(0.45f, 0.85f, 0.45f, 1.0f) : ImVec4(1.0f, 0.35f, 0.35f, 1.0f)
				, "%s", *m_StatusMessage);
		}

		if (ImGui::BeginPopupModal("##slot_group_reload_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Discard unsaved slot group changes and reload from disk?");
			if (ImGui::Button("Reload"))
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Slot groups reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_BufferSource = EString();
					m_SourceDocument = nullptr;
					InvalidateBuffers();
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
	}

	void FSlotGroupEditorPanel::DrawEditor(FSlotGroupDocument& InDocument, FLOAT InHeight)
	{
		if (InHeight <= 0.0f)
		{
			return;
		}
		BOOL32 bMutated = FALSE;
		if (ImGui::BeginChild("##slot_group_main", ImVec2(0.0f, InHeight), false))
		{
			const TArray<ESkeletonSlotGroup>& Groups = InDocument.GetDraftGroups();

			if (ImGui::BeginChild("##slot_group_list", ImVec2(260.0f, 0.0f), true))
			{
				ImGui::Text("Slot Groups: %d", Groups.Num());
				ImGui::TextDisabled("Top = base layer, bottom = highest overlay.");
				if (ImGui::Button("Add Group", ImVec2(-FLT_MIN, 0.0f)))
				{
					RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
						{
							return InDocument.AddGroup(OutError);
						}, EString("Slot group added."));
					if (InDocument.LastError.Length() <= 0u)
					{
						m_SelectedGroupIndex = InDocument.GetDraftGroups().Num() - 1;
						m_SelectedSlotIndex = 0;
						InDocument.Selection.Type = EEditorSelectionType::SLOT_GROUP;
						InDocument.Selection.ObjectIndex = m_SelectedGroupIndex;
						InDocument.Selection.SubIndex = m_SelectedSlotIndex;
					}
					bMutated = TRUE;
				}
				if (ImGui::Button("Reset Defaults", ImVec2(-FLT_MIN, 0.0f)))
				{
					RunMutation(InDocument, [&InDocument](EString& OutError) -> BOOL32
						{
							return InDocument.ResetDefaults(OutError);
						}, EString("Default slot groups restored."));
					m_SelectedGroupIndex = 0;
					m_SelectedSlotIndex = 0;
					InDocument.Selection.Type = EEditorSelectionType::SLOT_GROUP;
					InDocument.Selection.ObjectIndex = 0;
					InDocument.Selection.SubIndex = 0;
					bMutated = TRUE;
				}
				ImGui::Separator();
				if (bMutated == FALSE)
				{
					for (INT32 GroupIndex = 0; GroupIndex < Groups.Num(); GroupIndex++)
					{
						const ESkeletonSlotGroup& Group = Groups[GroupIndex];
						ImGui::PushID(GroupIndex);
						CHAR Label[256] = "";
						::snprintf(Label, sizeof(Label), "%d  %s  (%d slots)"
							, GroupIndex, *Group.Name, Group.SlotNames.Num());
						if (ImGui::Selectable(Label, (GroupIndex == m_SelectedGroupIndex)))
						{
							m_SelectedGroupIndex = GroupIndex;
							m_SelectedSlotIndex = -1;
							InDocument.Selection.Type = EEditorSelectionType::SLOT_GROUP;
							InDocument.Selection.ObjectIndex = GroupIndex;
							InDocument.Selection.SubIndex = -1;
						}
						ImGui::PopID();
					}
				}
				ImGui::Separator();
				const BOOL32 bHasSelection = ((m_SelectedGroupIndex >= 0)
					&& (m_SelectedGroupIndex < Groups.Num())) ? TRUE : FALSE;
				ImGui::BeginDisabled((bHasSelection == FALSE) || (bMutated != FALSE));
				if (ImGui::Button("Up") && (m_SelectedGroupIndex > 0))
				{
					RunMutation(InDocument, [&InDocument, this](EString& OutError) -> BOOL32
						{
							return InDocument.MoveGroup(m_SelectedGroupIndex, -1, OutError);
						}, EString("Slot group moved up."));
					if (InDocument.LastError.Length() <= 0u)
					{
						m_SelectedGroupIndex--;
						m_SelectedSlotIndex = -1;
						InDocument.Selection.ObjectIndex = m_SelectedGroupIndex;
						InDocument.Selection.SubIndex = -1;
					}
					bMutated = TRUE;
				}
				ImGui::SameLine();
				if (ImGui::Button("Down") && bHasSelection && (m_SelectedGroupIndex + 1 < Groups.Num()))
				{
					RunMutation(InDocument, [&InDocument, this](EString& OutError) -> BOOL32
						{
							return InDocument.MoveGroup(m_SelectedGroupIndex, 1, OutError);
						}, EString("Slot group moved down."));
					if (InDocument.LastError.Length() <= 0u)
					{
						m_SelectedGroupIndex++;
						m_SelectedSlotIndex = -1;
						InDocument.Selection.ObjectIndex = m_SelectedGroupIndex;
						InDocument.Selection.SubIndex = -1;
					}
					bMutated = TRUE;
				}
				ImGui::SameLine();
				if (ImGui::Button("Remove Group") && bHasSelection)
				{
					RunMutation(InDocument, [&InDocument, this](EString& OutError) -> BOOL32
						{
							return InDocument.DeleteGroup(m_SelectedGroupIndex, OutError);
						}, EString("Slot group removed."));
					m_SelectedSlotIndex = -1;
					m_SelectedGroupIndex = EMath::Clamp(m_SelectedGroupIndex, 0
						, EMath::Max(InDocument.GetDraftGroups().Num() - 1, 0));
					InDocument.Selection.ObjectIndex = m_SelectedGroupIndex;
					InDocument.Selection.SubIndex = -1;
					bMutated = TRUE;
				}
				ImGui::EndDisabled();
			}
			ImGui::EndChild();

			ImGui::SameLine();
			if (ImGui::BeginChild("##slot_group_slots", ImVec2(0.0f, 0.0f), true))
			{
				if ((bMutated == FALSE) && (m_SelectedGroupIndex >= 0)
					&& (m_SelectedGroupIndex < Groups.Num()))
				{
					const ESkeletonSlotGroup& Group = Groups[m_SelectedGroupIndex];
					ImGui::Text("Group: %s  |  Priority: %d", *Group.Name, m_SelectedGroupIndex);
					ImGui::TextDisabled("Name");
					ImGui::SetNextItemWidth(-FLT_MIN);
					m_GroupNameBuffers[m_SelectedGroupIndex].InputText("##slot_group_name");
					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						RunMutation(InDocument, [&InDocument, this](EString& OutError) -> BOOL32
							{
								return InDocument.RenameGroup(m_SelectedGroupIndex
									, m_GroupNameBuffers[m_SelectedGroupIndex].ToString(), OutError);
							}, EString("Slot group renamed."));
						bMutated = TRUE;
					}
					if (bMutated == FALSE)
					{
						if (ImGui::Button("Add Slot"))
						{
							RunMutation(InDocument, [&InDocument, this](EString& OutError) -> BOOL32
								{
									return InDocument.AddSlot(m_SelectedGroupIndex, OutError);
								}, EString("Slot added."));
							if (InDocument.LastError.Length() <= 0u)
							{
								m_SelectedSlotIndex = InDocument.GetDraftGroups()[m_SelectedGroupIndex]
									.SlotNames.Num() - 1;
								InDocument.Selection.SubIndex = m_SelectedSlotIndex;
							}
							bMutated = TRUE;
						}
						ImGui::Separator();
						if (bMutated == FALSE)
						{
							if (ImGui::BeginChild("##slot_name_list", ImVec2(0.0f, 0.0f), false))
							{
								for (INT32 SlotIndex = 0; SlotIndex < Group.SlotNames.Num(); SlotIndex++)
								{
									ImGui::PushID(SlotIndex);
									ImGui::SetNextItemWidth(EMath::Max(ImGui::GetContentRegionAvail().x - 78.0f, 80.0f));
									m_SlotNameBuffers[m_SelectedGroupIndex][SlotIndex].InputText("##slot_name");
									if (ImGui::IsItemActivated())
									{
										m_SelectedSlotIndex = SlotIndex;
										InDocument.Selection.SubIndex = SlotIndex;
									}
									if (ImGui::IsItemDeactivatedAfterEdit())
									{
										RunMutation(InDocument, [&InDocument, this, SlotIndex](EString& OutError) -> BOOL32
											{
												return InDocument.RenameSlot(m_SelectedGroupIndex, SlotIndex
													, m_SlotNameBuffers[m_SelectedGroupIndex][SlotIndex].ToString()
													, OutError);
											}, EString("Slot renamed."));
										bMutated = TRUE;
									}
									ImGui::SameLine();
									ImGui::BeginDisabled(bMutated != FALSE);
									if (ImGui::Button("Remove", ImVec2(72.0f, 0.0f)))
									{
										RunMutation(InDocument, [&InDocument, this, SlotIndex](EString& OutError) -> BOOL32
											{
												return InDocument.DeleteSlot(m_SelectedGroupIndex, SlotIndex, OutError);
											}, EString("Slot removed."));
										m_SelectedSlotIndex = -1;
										InDocument.Selection.SubIndex = -1;
										bMutated = TRUE;
									}
									ImGui::EndDisabled();
									ImGui::PopID();
									if (bMutated != FALSE)
									{
										break;
									}
								}
							}
							ImGui::EndChild();
						}
					}
				}
				else if (bMutated == FALSE)
				{
					ImGui::TextDisabled("Select a slot group.");
				}
			}
			ImGui::EndChild();
		}
		ImGui::EndChild();
		if (bMutated != FALSE)
		{
			InvalidateBuffers();
		}
	}

	void FSlotGroupEditorPanel::DrawBottomPanel(FSlotGroupDocument& InDocument, FLOAT InTotalHeight)
	{
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		FEditorSplitter::HorizontalInverted("##slot_group_output_split", m_BottomHeight
			, MinBottom, 0.0f, InTotalHeight);
		if (ImGui::BeginChild("##slot_group_output", ImVec2(0.0f, m_BottomHeight), true))
		{
			if (ImGui::BeginTabBar("##slot_group_output_tabs"))
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

	void FSlotGroupEditorPanel::DrawDiagnostics(FSlotGroupDocument& InDocument)
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
					? ImVec4(1.0f, 0.75f, 0.35f, 1.0f) : ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Text, Color);
			if (ImGui::Selectable(*Diagnostic.Message))
			{
				if ((Diagnostic.ObjectType == EEditorSelectionType::SLOT_GROUP)
					&& (Diagnostic.ObjectIndex >= 0))
				{
					m_SelectedGroupIndex = Diagnostic.ObjectIndex;
					m_SelectedSlotIndex = Diagnostic.SubIndex;
					InDocument.Selection.Type = EEditorSelectionType::SLOT_GROUP;
					InDocument.Selection.ObjectIndex = Diagnostic.ObjectIndex;
					InDocument.Selection.SubIndex = Diagnostic.SubIndex;
				}
			}
			ImGui::PopStyleColor();
		}
	}

	void FSlotGroupEditorPanel::DrawSource(FSlotGroupDocument& InDocument)
	{
		if (m_SourceDocument != &InDocument)
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
			m_SourceDocument = &InDocument;
		}
		if (ImGui::Button("Apply Text"))
		{
			const EString Text = m_SourceBuffer.ToString();
			RunMutation(InDocument, [&InDocument, Text](EString& OutError) -> BOOL32
				{
					return InDocument.CommitSourceText(Text, OutError);
				}, EString("Slot group source applied."));
			if (InDocument.LastError.Length() <= 0u)
			{
				m_SourceBuffer.SetText(InDocument.DraftJson);
				m_BufferSource = EString();
				InvalidateBuffers();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert Text"))
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
		m_SourceBuffer.InputTextMultiline("##slot_group_source", ImVec2(-FLT_MIN, -FLT_MIN)
			, ImGuiInputTextFlags_AllowTabInput);
		ImGui::PopStyleColor();
	}

	void FSlotGroupEditorPanel::RunMutation(FSlotGroupDocument& InDocument
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
			InvalidateBuffers();
		}
		else
		{
			InDocument.Commands.CancelTransaction();
			InDocument.LastError = Error;
			m_StatusMessage = Error;
			m_bStatusSucceeded = FALSE;
		}
	}

	void FSlotGroupEditorPanel::RestoreSnapshot(FSlotGroupDocument& InDocument
		, const FEditorSnapshot& InSnapshot)
	{
		InDocument.RestoreSnapshot(InSnapshot);
		m_BufferSource = EString();
		m_SourceDocument = nullptr;
		InvalidateBuffers();
	}

}

#endif
