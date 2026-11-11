#include "NotifyEditorPanel.h"

#if _EDITOR_ONLY

#include <AnimationNotifyAsset/AnimationNotifyAsset.h>
#include <imgui.h>
#include <cfloat>
#include <cstdio>

namespace PigeonEngine
{

	void FNotifyEditorPanel::Draw(FNotifyDocument& InDocument)
	{
		RefreshBuffers(InDocument);
		DrawToolbar(InDocument);

		const FLOAT TotalHeight = EMath::Max(0.0f, ImGui::GetContentRegionAvail().y);
		const FLOAT SplitterCost = FEditorSplitter::GetCost();
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		if (m_bShowOutput != FALSE)
		{
			const FLOAT MaxBottom = EMath::Max(MinBottom, TotalHeight - SplitterCost);
			m_BottomHeight = EMath::Clamp(m_BottomHeight, MinBottom, MaxBottom);
			const FLOAT FormHeight = EMath::Max(0.0f, TotalHeight - m_BottomHeight - SplitterCost);
			if (FormHeight > 0.0f)
			{
				DrawForm(InDocument, FormHeight);
			}
			DrawBottomPanel(InDocument, TotalHeight);
		}
		else
		{
			DrawForm(InDocument, TotalHeight);
		}
	}

	void FNotifyEditorPanel::RefreshBuffers(FNotifyDocument& InDocument)
	{
		if (m_BufferSource == InDocument.AssetPath)
		{
			return;
		}
		const EAnimNotifyDefinition* Definition = InDocument.GetDraftDefinition();
		if (Definition)
		{
			m_NameBuffer.SetText(Definition->Name);
			m_DescriptionBuffer.SetText(Definition->Description);
			m_PayloadBuffer.SetText(Definition->PayloadJson);
		}
		else
		{
			m_NameBuffer.SetText(EString());
			m_DescriptionBuffer.SetText(EString());
			m_PayloadBuffer.SetText(EString());
		}
		m_BufferSource = InDocument.AssetPath;
	}

	void FNotifyEditorPanel::DrawToolbar(FNotifyDocument& InDocument)
	{
		FImGuiIdScope ToolbarScope("NotifyToolbar");
		ImGui::Text("%s", *InDocument.DisplayName);
		ImGui::SameLine();
		ImGui::TextDisabled("Notify Definition");
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
				m_StatusMessage = EString("Notify definition applied.");
				m_bStatusSucceeded = TRUE;
				m_BufferSource = EString();
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
				m_StatusMessage = EString("Notify definition saved.");
				m_bStatusSucceeded = TRUE;
				m_BufferSource = EString();
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
				ImGui::OpenPopup("##notify_reload_confirm");
			}
			else
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Notify definition reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_BufferSource = EString();
					m_SourceDocument = nullptr;
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

		if (ImGui::BeginPopupModal("##notify_reload_confirm", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::Text("Discard unsaved notify changes and reload from disk?");
			if (ImGui::Button("Reload"))
			{
				EString Error;
				if (InDocument.Reload(Error))
				{
					m_StatusMessage = EString("Notify definition reloaded from disk.");
					m_bStatusSucceeded = TRUE;
					m_BufferSource = EString();
					m_SourceDocument = nullptr;
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

	void FNotifyEditorPanel::DrawForm(FNotifyDocument& InDocument, FLOAT InHeight)
	{
		if (InHeight <= 0.0f)
		{
			return;
		}
		if (ImGui::BeginChild("##notify_form", ImVec2(0.0f, InHeight), true))
		{
			const EAnimNotifyDefinition* Definition = InDocument.GetDraftDefinition();
			if (!Definition)
			{
				ImGui::TextDisabled("Notify definition is invalid. Fix it in the Source tab.");
				ImGui::EndChild();
				return;
			}
			const BOOL32 bIsState = Definition->bIsState;
			// Copy fields before any commit: a successful commit rebuilds the draft and frees
			// the definition this pointer refers to.
			const EString CurrentClassName = Definition->NotifyClassName;
			const FLOAT CurrentDuration = Definition->DefaultDuration;
			ImGui::Text("Notify: %s", (Definition->Name.Length() > 0u)
				? *Definition->Name : "(unnamed)");
			ImGui::TextDisabled("Kind: %s  |  Class: %s"
				, (bIsState != FALSE) ? "State" : "Instant", *Definition->NotifyClassName);
			ImGui::Separator();

			ImGui::TextDisabled("Name");
			ImGui::SetNextItemWidth(-FLT_MIN);
			m_NameBuffer.InputText("##notify_name");
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				RunMutation(InDocument, [this, &InDocument, CurrentClassName, CurrentDuration](EString& OutError) -> BOOL32
					{
						return CommitDefinition(InDocument, CurrentClassName, CurrentDuration, OutError);
					}, EString("Notify name updated."));
				ImGui::EndChild();
				return;
			}

			ImGui::TextDisabled("Description");
			ImGui::SetNextItemWidth(-FLT_MIN);
			m_DescriptionBuffer.InputTextMultiline("##notify_description", ImVec2(-FLT_MIN, 72.0f));
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				RunMutation(InDocument, [this, &InDocument, CurrentClassName, CurrentDuration](EString& OutError) -> BOOL32
					{
						return CommitDefinition(InDocument, CurrentClassName, CurrentDuration, OutError);
					}, EString("Notify description updated."));
				ImGui::EndChild();
				return;
			}

			const TArray<EString> ClassNames = EAnimNotifyRegistry::GetRegisteredClassNames();
			ImGui::TextDisabled("Class");
			ImGui::SetNextItemWidth(-FLT_MIN);
			if (ImGui::BeginCombo("##notify_class", ((Definition->NotifyClassName.Length() > 0u)
				? *Definition->NotifyClassName : "None")))
			{
				BOOL32 bCommitted = FALSE;
				for (INT32 ClassIndex = 0; ClassIndex < ClassNames.Num(); ClassIndex++)
				{
					BOOL32 bClassIsState = FALSE;
					const BOOL32 bKnownClass = EAnimNotifyRegistry::IsStateClass(ClassNames[ClassIndex], bClassIsState);
					EString ClassLabel = ClassNames[ClassIndex];
					if (bKnownClass != FALSE)
					{
						ClassLabel += ((bClassIsState != FALSE) ? EString(" (State)") : EString(" (Instant)"));
					}
					const BOOL8 bSelected = (Definition->NotifyClassName == ClassNames[ClassIndex]);
					if (ImGui::Selectable(*ClassLabel, bSelected))
					{
						const FLOAT NewDuration = ((bClassIsState != FALSE)
							? EMath::Max(CurrentDuration, 0.1f) : 0.0f);
						RunMutation(InDocument, [this, &InDocument, ClassIndex, NewDuration](EString& OutError) -> BOOL32
							{
								const TArray<EString> Names = EAnimNotifyRegistry::GetRegisteredClassNames();
								if ((ClassIndex < 0) || (ClassIndex >= Names.Num()))
								{
									OutError = EString("Notify class is no longer registered.");
									return FALSE;
								}
								return CommitDefinition(InDocument, Names[ClassIndex], NewDuration, OutError);
							}, EString("Notify class updated."));
						bCommitted = TRUE;
						break;
					}
					if (bSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
				if (bCommitted != FALSE)
				{
					ImGui::EndChild();
					return;
				}
			}

			if (bIsState != FALSE)
			{
				FLOAT Duration = CurrentDuration;
				ImGui::TextDisabled("Default Duration (s)");
				ImGui::SetNextItemWidth(-FLT_MIN);
				ImGui::InputFloat("##notify_duration", &Duration, 0.0f, 0.0f, "%.3f");
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					RunMutation(InDocument, [this, &InDocument, CurrentClassName, Duration](EString& OutError) -> BOOL32
						{
							return CommitDefinition(InDocument, CurrentClassName
								, EMath::Max(Duration, 0.001f), OutError);
						}, EString("Notify default duration updated."));
					ImGui::EndChild();
					return;
				}
			}

			ImGui::TextDisabled("Payload JSON");
			ImGui::SetNextItemWidth(-FLT_MIN);
			m_PayloadBuffer.InputTextMultiline("##notify_payload", ImVec2(-FLT_MIN, 120.0f));
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				RunMutation(InDocument, [this, &InDocument, CurrentClassName, CurrentDuration](EString& OutError) -> BOOL32
					{
						return CommitDefinition(InDocument, CurrentClassName, CurrentDuration, OutError);
					}, EString("Notify payload updated."));
				ImGui::EndChild();
				return;
			}

			ImGui::Separator();
			ImGui::TextDisabled("Clips and montages reference this definition by its NotifyAsset name.");
		}
		ImGui::EndChild();
	}

	void FNotifyEditorPanel::DrawBottomPanel(FNotifyDocument& InDocument, FLOAT InTotalHeight)
	{
		const FLOAT MinBottom = ImGui::GetFrameHeight() + ImGui::GetStyle().WindowPadding.y * 2.0f;
		FEditorSplitter::HorizontalInverted("##notify_output_split", m_BottomHeight
			, MinBottom, 0.0f, InTotalHeight);
		if (ImGui::BeginChild("##notify_output", ImVec2(0.0f, m_BottomHeight), true))
		{
			if (ImGui::BeginTabBar("##notify_output_tabs"))
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

	void FNotifyEditorPanel::DrawDiagnostics(FNotifyDocument& InDocument)
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
			ImGui::TextWrapped("%s", *Diagnostic.Message);
			ImGui::PopStyleColor();
		}
	}

	void FNotifyEditorPanel::DrawSource(FNotifyDocument& InDocument)
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
				}, EString("Notify source applied."));
			if (InDocument.LastError.Length() <= 0u)
			{
				m_SourceBuffer.SetText(InDocument.DraftJson);
				m_BufferSource = EString();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Revert Text"))
		{
			m_SourceBuffer.SetText(InDocument.DraftJson);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75f, 0.75f, 0.75f, 1.0f));
		m_SourceBuffer.InputTextMultiline("##notify_source", ImVec2(-FLT_MIN, -FLT_MIN)
			, ImGuiInputTextFlags_AllowTabInput);
		ImGui::PopStyleColor();
	}

	BOOL32 FNotifyEditorPanel::CommitDefinition(FNotifyDocument& InDocument
		, const EString& InClassName, FLOAT InDefaultDuration, EString& OutError)
	{
		return InDocument.UpdateDefinition(m_NameBuffer.ToString(), m_DescriptionBuffer.ToString()
			, InClassName, m_PayloadBuffer.ToString(), InDefaultDuration, OutError);
	}

	void FNotifyEditorPanel::RunMutation(FNotifyDocument& InDocument
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

	void FNotifyEditorPanel::RestoreSnapshot(FNotifyDocument& InDocument, const FEditorSnapshot& InSnapshot)
	{
		InDocument.RestoreSnapshot(InSnapshot);
		m_BufferSource = EString();
		m_SourceDocument = nullptr;
	}

}

#endif
