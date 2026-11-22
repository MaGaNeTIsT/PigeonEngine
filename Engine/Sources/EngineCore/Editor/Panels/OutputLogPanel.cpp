#include <Editor/Panels/OutputLogPanel.h>

#if ENGINE_EDITOR_ONLY

#include <Editor/Manager/EditorWindowRegistry.h>

#include <Windows.h>
#include <shellapi.h>
#include <imgui.h>

namespace PigeonEngine
{
	namespace
	{
		ImVec4 GetLogColor(const ELogType InType)
		{
			if (InType == ELogType::ELT_ERROR)
			{
				return ImVec4(1.0f, 0.35f, 0.35f, 1.0f);
			}
			if (InType == ELogType::ELT_WARNING)
			{
				return ImVec4(1.0f, 0.78f, 0.35f, 1.0f);
			}
			return ImGui::GetStyleColorVec4(ImGuiCol_Text);
		}
	}

	FOutputLogPanel::FOutputLogPanel()
	{
		Categories.Add(EString("All Categories"));
	}

	void FOutputLogPanel::Draw()
	{
		// Size is a first-use default; position and docking come from the saved layout.
		// No SetNextWindowPos here - it would undock a reopened panel.
		ImGui::SetNextWindowSize(ImVec2(720.0f, 360.0f), ImGuiCond_FirstUseEver);
		FScopedEditorWindow Window(EEditorWindowType::OutputLog);
		if (Window.IsOpen() == FALSE)
		{
			return;
		}

		RefreshSnapshot();
		if (bFilterDirty != FALSE)
		{
			RebuildRows();
		}

		DrawToolbar();
		ImGui::Separator();
		DrawRows();
	}

	void FOutputLogPanel::RefreshSnapshot()
	{
		if (bPaused != FALSE)
		{
			return;
		}
		EEditorLogManager* LogManager = EEditorLogManager::GetManagerSingleton();
		const UINT32 Revision = LogManager->GetLogRevision();
		if (Revision == LastRevision)
		{
			return;
		}

		const UINT32 AddedNum = (Revision > LastRevision) ? (Revision - LastRevision) : 0u;
		LogManager->SnapshotVisibleLogs(Snapshot);
		if (Snapshot.IsEmpty())
		{
			MaxLineWidth = 0.0f;
		}
		else
		{
			// Only the lines that arrived since the previous frame need measuring: a clipped
			// row contributes no width, and a full re-measure on every batch would put the
			// CalcTextSize cost back on the game thread.
			UINT32 MeasureNum = (AddedNum > 0u) ? AddedNum : 1u;
			const UINT32 SnapshotNum = Snapshot.Num<UINT32>();
			if (MeasureNum > SnapshotNum)
			{
				MeasureNum = SnapshotNum;
			}
			for (UINT32 Index = SnapshotNum - MeasureNum; Index < SnapshotNum; Index++)
			{
				const FLOAT LineWidth = ImGui::CalcTextSize(*Snapshot[Index]->GetText()).x;
				if (LineWidth > MaxLineWidth)
				{
					MaxLineWidth = LineWidth;
				}
			}
			if (bFollowTail != FALSE)
			{
				bScrollToBottom = TRUE;
			}
		}

		LastRevision = Revision;
		bFilterDirty = TRUE;
		RebuildCategories();
	}

	void FOutputLogPanel::RebuildCategories()
	{
		EString Previous;
		if ((CategoryIndex > 0) && (CategoryIndex < Categories.Num<INT32>()))
		{
			Previous = Categories[CategoryIndex];
		}

		Categories.Empty();
		Categories.Add(EString("All Categories"));
		for (INT32 Index = 0, SnapshotNum = Snapshot.Num<INT32>(); Index < SnapshotNum; Index++)
		{
			const EString& Category = Snapshot[Index]->Category;
			BOOL8 bFound = FALSE;
			for (INT32 CategoryScan = 1, CategoryNum = Categories.Num<INT32>(); CategoryScan < CategoryNum; CategoryScan++)
			{
				if (Categories[CategoryScan] == Category)
				{
					bFound = TRUE;
					break;
				}
			}
			if (bFound == FALSE)
			{
				Categories.Add(Category);
			}
		}

		// Keep "All Categories" first; sort the collected names after it.
		for (INT32 Index = 2, CategoryNum = Categories.Num<INT32>(); Index < CategoryNum; Index++)
		{
			const EString Current = Categories[Index];
			INT32 InsertIndex = Index - 1;
			while ((InsertIndex >= 1) && (Categories[InsertIndex] > Current))
			{
				Categories[InsertIndex + 1] = Categories[InsertIndex];
				InsertIndex--;
			}
			Categories[InsertIndex + 1] = Current;
		}

		CategoryIndex = 0;
		if (Previous.Length() > 0u)
		{
			for (INT32 Index = 1, CategoryNum = Categories.Num<INT32>(); Index < CategoryNum; Index++)
			{
				if (Categories[Index] == Previous)
				{
					CategoryIndex = Index;
					break;
				}
			}
		}
	}

	void FOutputLogPanel::RebuildRows()
	{
		Rows.Empty();
		const EString SearchLower = EString(SearchBuffer).ToLower();
		INT32 Index = 0;
		const INT32 SnapshotNum = Snapshot.Num<INT32>();
		while (Index < SnapshotNum)
		{
			const ELog& Entry = *Snapshot[Index];
			if (MatchesFilters(Entry, SearchLower) == FALSE)
			{
				Index++;
				continue;
			}

			FLogRow Row;
			Row.FirstIndex = Index;
			Row.Count = 1;
			if (bCollapseRepeats != FALSE)
			{
				while (Index + Row.Count < SnapshotNum)
				{
					const ELog& Next = *Snapshot[Index + Row.Count];
					if ((Next.Type != Entry.Type) || (Next.Log != Entry.Log))
					{
						break;
					}
					Row.Count++;
				}
			}
			Rows.Add(Row);
			Index += Row.Count;
		}

		if ((SelectedRow < 0) || (SelectedRow >= Rows.Num<INT32>()))
		{
			SelectedRow = -1;
		}
		bFilterDirty = FALSE;
	}

	void FOutputLogPanel::DrawToolbar()
	{
		EEditorLogManager* LogManager = EEditorLogManager::GetManagerSingleton();

		if (ImGui::Button(bPaused != FALSE ? "Resume" : "Pause"))
		{
			bPaused = (bPaused != FALSE) ? FALSE : TRUE;
			if (bPaused == FALSE)
			{
				bScrollToBottom = TRUE;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			ClearLogView();
		}
		ImGui::SameLine();
		if (ImGui::Button("Copy"))
		{
			if (SelectedRow >= 0)
			{
				CopyRow(SelectedRow);
			}
			else
			{
				CopyAllFiltered();
			}
		}
		ImGui::SameLine();
		bool bFollow = (bFollowTail != FALSE) ? true : false;
		if (ImGui::Checkbox("Follow", &bFollow))
		{
			bFollowTail = bFollow ? TRUE : FALSE;
			if (bFollowTail != FALSE)
			{
				bScrollToBottom = TRUE;
			}
		}
		ImGui::SameLine();
		bool bCollapse = (bCollapseRepeats != FALSE) ? true : false;
		if (ImGui::Checkbox("Collapse", &bCollapse))
		{
			bCollapseRepeats = bCollapse ? TRUE : FALSE;
			bFilterDirty = TRUE;
		}
		ImGui::SameLine();
		if (ImGui::Button("Bottom"))
		{
			bFollowTail = TRUE;
			bScrollToBottom = TRUE;
		}
		ImGui::SameLine();
		if (ImGui::Button("Open Log File"))
		{
			const EString& LogPath = LogManager->GetLogFilePath();
			if (LogPath.Length() > 0u)
			{
				::ShellExecuteA(nullptr, "open", *LogPath, nullptr, nullptr, SW_SHOWNORMAL);
			}
		}

		// Level filters with the counts of the current session, not of the filtered view.
		bool bLog = (bShowLog != FALSE) ? true : false;
		if (ImGui::Checkbox("Log", &bLog))
		{
			bShowLog = bLog ? TRUE : FALSE;
			bFilterDirty = TRUE;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(%u)", LogManager->GetLogCount(ELogType::ELT_LOG));
		ImGui::SameLine();
		bool bWarning = (bShowWarning != FALSE) ? true : false;
		if (ImGui::Checkbox("Warning", &bWarning))
		{
			bShowWarning = bWarning ? TRUE : FALSE;
			bFilterDirty = TRUE;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(%u)", LogManager->GetLogCount(ELogType::ELT_WARNING));
		ImGui::SameLine();
		bool bError = (bShowError != FALSE) ? true : false;
		if (ImGui::Checkbox("Error", &bError))
		{
			bShowError = bError ? TRUE : FALSE;
			bFilterDirty = TRUE;
		}
		ImGui::SameLine();
		ImGui::TextDisabled("(%u)", LogManager->GetLogCount(ELogType::ELT_ERROR));

		ImGui::SetNextItemWidth(260.0f);
		if (ImGui::InputTextWithHint("##log_search", "Search text...", SearchBuffer, sizeof(SearchBuffer)))
		{
			bFilterDirty = TRUE;
		}
		ImGui::SameLine();
		ImGui::SetNextItemWidth(180.0f);
		if (Categories.IsEmpty())
		{
			RebuildCategories();
		}
		if ((CategoryIndex < 0) || (CategoryIndex >= Categories.Num<INT32>()))
		{
			CategoryIndex = 0;
		}
		if (ImGui::BeginCombo("##log_category", *Categories[CategoryIndex]))
		{
			for (INT32 Index = 0, CategoryNum = Categories.Num<INT32>(); Index < CategoryNum; Index++)
			{
				const BOOL8 bSelected = (Index == CategoryIndex) ? TRUE : FALSE;
				if (ImGui::Selectable(*Categories[Index], bSelected != FALSE))
				{
					CategoryIndex = Index;
					bFilterDirty = TRUE;
				}
				if (bSelected != FALSE)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::SameLine();
		ImGui::TextDisabled("%s", *LogManager->GetLogFilePath());

		if (bPaused != FALSE)
		{
			const UINT32 Revision = LogManager->GetLogRevision();
			const UINT32 PendingNum = (Revision > LastRevision) ? (Revision - LastRevision) : 0u;
			ImGui::SameLine();
			ImGui::TextDisabled("(paused, +%u new)", PendingNum);
		}
	}

	void FOutputLogPanel::DrawRows()
	{
		if (Snapshot.IsEmpty())
		{
			ImGui::TextDisabled("No log entries.");
			return;
		}
		if (Rows.IsEmpty())
		{
			ImGui::TextDisabled("No log entries match the current filter.");
			return;
		}

		const FLOAT RowHeight = ImGui::GetTextLineHeightWithSpacing();
		ImGui::BeginChild("##log_rows", ImVec2(0.0f, 0.0f), TRUE, ImGuiWindowFlags_HorizontalScrollbar);
		{
			ImGuiListClipper Clipper;
			Clipper.Begin(Rows.Num<INT32>(), RowHeight);
			while (Clipper.Step())
			{
				for (INT32 RowIndex = Clipper.DisplayStart; RowIndex < Clipper.DisplayEnd; RowIndex++)
				{
					DrawRow(RowIndex);
				}
			}
			Clipper.End();

			// Clipped rows contribute no width; the dummy keeps the horizontal scrollbar
			// describing the whole filtered log.
			ImGui::Dummy(ImVec2(MaxLineWidth + 8.0f, 0.0f));

			const FLOAT ScrollMaxY = ImGui::GetScrollMaxY();
			// Follow is a convenience, not a lock: an upward wheel or a drag of the
			// scrollbar turns it off so the user can read earlier lines while new ones
			// keep arriving.
			if ((ImGui::GetIO().MouseWheel > 0.0f)
				&& ImGui::IsWindowHovered(ImGuiHoveredFlags_ChildWindows))
			{
				bFollowTail = FALSE;
				bScrollToBottom = FALSE;
			}
			if ((bFollowTail != FALSE) && (bScrollToBottom == FALSE)
				&& (ScrollMaxY > 0.0f) && (ImGui::GetScrollY() < ScrollMaxY - 2.0f))
			{
				bFollowTail = FALSE;
			}
			if ((bScrollToBottom != FALSE) || (bFollowTail != FALSE))
			{
				ImGui::SetScrollY(ScrollMaxY);
				bScrollToBottom = FALSE;
			}
		}
		ImGui::EndChild();
	}

	void FOutputLogPanel::DrawRow(const INT32 InRowIndex)
	{
		const FLogRow& Row = Rows[InRowIndex];
		const ELog& Entry = *Snapshot[Row.FirstIndex];
		EString DisplayText = Entry.GetText();
		if (Row.Count > 1)
		{
			DisplayText += "  (x";
			DisplayText += EString::FromInt(static_cast<UINT32>(Row.Count));
			DisplayText += ")";
		}

		const FLOAT RowHeight = ImGui::GetTextLineHeightWithSpacing();
		const ImVec2 RowStart = ImGui::GetCursorScreenPos();
		const ImVec4 Color = GetLogColor(Entry.Type);

		ImGui::PushID(InRowIndex);
		// The selectable label is a fixed ID; the log text is drawn through the draw list
		// so a message containing '##' or a percent sign can never be parsed as an ID or
		// a printf format string.
		const FLOAT SelectWidth = (ImGui::GetContentRegionAvail().x > 1.0f)
			? ImGui::GetContentRegionAvail().x : ImGui::GetWindowWidth();
		if (ImGui::Selectable("##log_row", SelectedRow == InRowIndex
			, ImGuiSelectableFlags_AllowDoubleClick, ImVec2(SelectWidth, RowHeight)))
		{
			SelectedRow = InRowIndex;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::TextUnformatted(*Entry.GetText());
			ImGui::TextDisabled("Category: %s", *Entry.Category);
			ImGui::EndTooltip();
		}
		if (ImGui::BeginPopupContextItem("##log_row_menu"))
		{
			if (ImGui::MenuItem("Copy Line"))
			{
				CopyRow(InRowIndex);
			}
			if (ImGui::MenuItem("Copy All"))
			{
				CopyAllFiltered();
			}
			if (ImGui::MenuItem("Clear"))
			{
				ClearLogView();
			}
			ImGui::EndPopup();
		}

		const FLOAT TextY = RowStart.y + (RowHeight - ImGui::GetTextLineHeight()) * 0.5f;
		ImGui::GetWindowDrawList()->AddText(ImVec2(RowStart.x + 4.0f, TextY)
			, ImGui::GetColorU32(Color), *DisplayText);
		ImGui::PopID();
	}

	void FOutputLogPanel::ClearLogView()
	{
		EEditorLogManager* LogManager = EEditorLogManager::GetManagerSingleton();
		LogManager->ClearVisibleLogs();
		Snapshot.Empty();
		Rows.Empty();
		Categories.Empty();
		Categories.Add(EString("All Categories"));
		CategoryIndex = 0;
		SelectedRow = -1;
		MaxLineWidth = 0.0f;
		LastRevision = LogManager->GetLogRevision();
		bFilterDirty = FALSE;
		bScrollToBottom = TRUE;
	}

	void FOutputLogPanel::CopyRow(const INT32 InRowIndex) const
	{
		if ((InRowIndex < 0) || (InRowIndex >= Rows.Num<INT32>()))
		{
			return;
		}
		const FLogRow& Row = Rows[InRowIndex];
		if ((Row.FirstIndex >= 0) && (Row.FirstIndex < Snapshot.Num<INT32>()))
		{
			ImGui::SetClipboardText(*Snapshot[Row.FirstIndex]->GetText());
		}
	}

	void FOutputLogPanel::CopyAllFiltered() const
	{
		EString Text;
		for (INT32 Index = 0, RowNum = Rows.Num<INT32>(); Index < RowNum; Index++)
		{
			const FLogRow& Row = Rows[Index];
			if ((Row.FirstIndex < 0) || (Row.FirstIndex >= Snapshot.Num<INT32>()))
			{
				continue;
			}
			Text += Snapshot[Row.FirstIndex]->GetText();
			if (Row.Count > 1)
			{
				Text += "  (x";
				Text += EString::FromInt(static_cast<UINT32>(Row.Count));
				Text += ")";
			}
			Text += "\n";
		}
		if (Text.Length() > 0u)
		{
			ImGui::SetClipboardText(*Text);
		}
	}

	BOOL32 FOutputLogPanel::MatchesFilters(const ELog& InLog, const EString& InSearchLower) const
	{
		if ((InLog.Type == ELogType::ELT_LOG) && (bShowLog == FALSE))
		{
			return FALSE;
		}
		if ((InLog.Type == ELogType::ELT_WARNING) && (bShowWarning == FALSE))
		{
			return FALSE;
		}
		if ((InLog.Type == ELogType::ELT_ERROR) && (bShowError == FALSE))
		{
			return FALSE;
		}
		if (MatchesCategory(InLog) == FALSE)
		{
			return FALSE;
		}
		if (InSearchLower.Length() > 0u)
		{
			if (InLog.GetText().ToLower().Contains(InSearchLower) == FALSE)
			{
				return FALSE;
			}
		}
		return TRUE;
	}

	BOOL32 FOutputLogPanel::MatchesCategory(const ELog& InLog) const
	{
		if (CategoryIndex <= 0)
		{
			return TRUE;
		}
		if (CategoryIndex >= Categories.Num<INT32>())
		{
			return TRUE;
		}
		return (InLog.Category == Categories[CategoryIndex]) ? TRUE : FALSE;
	}
}

#endif
