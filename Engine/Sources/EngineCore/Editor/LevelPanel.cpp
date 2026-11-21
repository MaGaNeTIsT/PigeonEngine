#include <Editor/LevelPanel.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <IO/FileHelper.h>
#include <Config/EngineConfig.h>
#include <PigeonBase/Object/World/WorldManager.h>

namespace PigeonEngine
{
	namespace
	{
		enum class ELevelFileOperationType : UINT8
		{
			NONE,
			RENAME,
			DUPLICATE,
			REMOVE,
		};

		// The level directory listing, refreshed on demand: after every file operation,
		// after a save/new/load, and from the Refresh button.
		TArray<EString>	g_LevelFiles;
		BOOL32			g_bLevelListDirty = TRUE;

		// Popup intents, set by the world manager's request API and the editor File menu.
		BOOL32			g_bOpenPopupRequested = FALSE;
		BOOL32			g_bSaveAsPopupRequested = FALSE;
		BOOL32			g_bUnsavedPromptRequested = FALSE;

		// One rename/duplicate/delete flow at a time.
		ELevelFileOperationType	g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
		EString					g_PendingLevelFilePath;
		EString					g_LevelFileOperationError;
		BOOL32					g_bLevelFileOperationPopupRequested = FALSE;

		CHAR			g_SaveAsNameBuffer[256] = {};
		CHAR			g_LevelFileNameBuffer[256] = {};

		EString GetLevelFileName(const EString& InPath)
		{
			const INT32 Separator = InPath.RightFind(EString(PE_TEXT("/")));
			return (Separator >= 0) ? InPath.Right(InPath.Length() - static_cast<UINT32>(Separator) - 1u) : InPath;
		}

		BOOL32 HasLevelExtension(const EString& InName)
		{
			return ((InName.Length() >= 7u) && (InName.Right(7u) == EString(PE_TEXT(".PLevel")))) ? TRUE : FALSE;
		}

		EString GetLevelStem(const EString& InPath)
		{
			const EString FileName = GetLevelFileName(InPath);
			return HasLevelExtension(FileName) ? FileName.Left(FileName.Length() - 7u) : FileName;
		}

		void RefreshLevelList()
		{
			g_LevelFiles.Empty();
			TArray<EString> Folders;
			TArray<EString> Files;
			if (EFileHelper::ScanDirectory(EString(EBaseSettings::ENGINE_LEVEL_PATH), Folders, Files, FALSE))
			{
				for (UINT32 Index = 0u, Num = Files.Num<UINT32>(); Index < Num; Index++)
				{
					if (HasLevelExtension(Files[Index]))
					{
						g_LevelFiles.Add(Files[Index]);
					}
				}
			}
			// ScanDirectory's order is filesystem order; sort so the list is stable.
			for (INT32 Index = 1, Num = g_LevelFiles.Num<INT32>(); Index < Num; Index++)
			{
				for (INT32 Insert = Index; (Insert > 0) && (g_LevelFiles[Insert] < g_LevelFiles[Insert - 1]); Insert--)
				{
					const EString Temp = g_LevelFiles[Insert];
					g_LevelFiles[Insert] = g_LevelFiles[Insert - 1];
					g_LevelFiles[Insert - 1] = Temp;
				}
			}
			g_bLevelListDirty = FALSE;
		}

		void RequestRenameLevel(const EString& InPath)
		{
			g_PendingLevelFileOperation = ELevelFileOperationType::RENAME;
			g_PendingLevelFilePath = InPath;
			g_LevelFileOperationError = EString();
			const EString Stem = GetLevelStem(InPath);
			const UINT32 NameLength = EMath::Min(Stem.Length(), static_cast<UINT32>(sizeof(g_LevelFileNameBuffer) - 1u));
			memset(g_LevelFileNameBuffer, 0, sizeof(g_LevelFileNameBuffer));
			if (NameLength > 0u)
			{
				memcpy(g_LevelFileNameBuffer, *Stem, NameLength);
			}
			g_bLevelFileOperationPopupRequested = TRUE;
		}

		void RequestDuplicateLevel(const EString& InPath)
		{
			g_PendingLevelFileOperation = ELevelFileOperationType::DUPLICATE;
			g_PendingLevelFilePath = InPath;
			g_LevelFileOperationError = EString();
			const EString Stem = GetLevelStem(InPath) + EString(PE_TEXT("_Copy"));
			const UINT32 NameLength = EMath::Min(Stem.Length(), static_cast<UINT32>(sizeof(g_LevelFileNameBuffer) - 1u));
			memset(g_LevelFileNameBuffer, 0, sizeof(g_LevelFileNameBuffer));
			if (NameLength > 0u)
			{
				memcpy(g_LevelFileNameBuffer, *Stem, NameLength);
			}
			g_bLevelFileOperationPopupRequested = TRUE;
		}

		void RequestDeleteLevel(const EString& InPath)
		{
			g_PendingLevelFileOperation = ELevelFileOperationType::REMOVE;
			g_PendingLevelFilePath = InPath;
			g_LevelFileOperationError = EString();
			g_bLevelFileOperationPopupRequested = TRUE;
		}

		void DrawLevelFileOperationPopup(EWorldManager* InManager)
		{
			if (g_PendingLevelFileOperation == ELevelFileOperationType::NONE)
			{
				return;
			}
			if (g_bLevelFileOperationPopupRequested)
			{
				g_bLevelFileOperationPopupRequested = FALSE;
				switch (g_PendingLevelFileOperation)
				{
				case ELevelFileOperationType::RENAME:
					ImGui::OpenPopup("Rename Level");
					break;
				case ELevelFileOperationType::DUPLICATE:
					ImGui::OpenPopup("Duplicate Level");
					break;
				case ELevelFileOperationType::REMOVE:
					ImGui::OpenPopup("Delete Level");
					break;
				default:
					break;
				}
			}

			const EString SourceFileName = GetLevelFileName(g_PendingLevelFilePath);
			const EString CurrentLevelPath = InManager->GetCurrentLevelPath();
			const BOOL32 bIsCurrentLevel = (CurrentLevelPath == g_PendingLevelFilePath) ? TRUE : FALSE;

			if ((g_PendingLevelFileOperation == ELevelFileOperationType::RENAME) &&
				ImGui::BeginPopupModal("Rename Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("Rename:");
				ImGui::TextDisabled("%s", *SourceFileName);
				ImGui::SetNextItemWidth(320.0f);
				BOOL32 bConfirm = ImGui::InputText("##RenameLevelName", g_LevelFileNameBuffer, sizeof(g_LevelFileNameBuffer),
					ImGuiInputTextFlags_EnterReturnsTrue);
				if (ImGui::Button("Rename"))
				{
					bConfirm = TRUE;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
					g_PendingLevelFilePath = EString();
					ImGui::CloseCurrentPopup();
				}

				if (bConfirm)
				{
					g_LevelFileOperationError = EString();
					EString TargetFileName(g_LevelFileNameBuffer);
					if (TargetFileName.Length() == 0u)
					{
						g_LevelFileOperationError = EString(PE_TEXT("The file name cannot be empty."));
					}
					else
					{
						if (!HasLevelExtension(TargetFileName))
						{
							TargetFileName += EString(PE_TEXT(".PLevel"));
						}
						if (TargetFileName == SourceFileName)
						{
							g_LevelFileOperationError = EString(PE_TEXT("The new name is the same as the current name."));
						}
						else
						{
							const EString TargetPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + TargetFileName;
							if (EFileHelper::IsFileExists(TargetPath))
							{
								g_LevelFileOperationError = EString(PE_TEXT("A level with that name already exists."));
							}
							else if (!EFileHelper::MoveFile(g_PendingLevelFilePath, TargetPath))
							{
								g_LevelFileOperationError = EString(PE_TEXT("The level file could not be renamed."));
							}
							else
							{
								// The manager fixes up the current path when its own file moved.
								InManager->NotifyLevelFileRenamed(g_PendingLevelFilePath, TargetPath);
								g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
								g_PendingLevelFilePath = EString();
								g_bLevelListDirty = TRUE;
								ImGui::CloseCurrentPopup();
							}
						}
					}
				}
				if (g_LevelFileOperationError.Length() > 0u)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *g_LevelFileOperationError);
				}
				ImGui::EndPopup();
			}

			if ((g_PendingLevelFileOperation == ELevelFileOperationType::DUPLICATE) &&
				ImGui::BeginPopupModal("Duplicate Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("Duplicate:");
				ImGui::TextDisabled("%s", *SourceFileName);
				ImGui::SetNextItemWidth(320.0f);
				BOOL32 bConfirm = ImGui::InputText("##DuplicateLevelName", g_LevelFileNameBuffer, sizeof(g_LevelFileNameBuffer),
					ImGuiInputTextFlags_EnterReturnsTrue);
				if (ImGui::Button("Duplicate"))
				{
					bConfirm = TRUE;
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
					g_PendingLevelFilePath = EString();
					ImGui::CloseCurrentPopup();
				}

				if (bConfirm)
				{
					g_LevelFileOperationError = EString();
					EString TargetFileName(g_LevelFileNameBuffer);
					if (TargetFileName.Length() == 0u)
					{
						g_LevelFileOperationError = EString(PE_TEXT("The file name cannot be empty."));
					}
					else
					{
						if (!HasLevelExtension(TargetFileName))
						{
							TargetFileName += EString(PE_TEXT(".PLevel"));
						}
						const EString TargetPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + TargetFileName;
						if (EFileHelper::IsFileExists(TargetPath))
						{
							g_LevelFileOperationError = EString(PE_TEXT("A level with that name already exists."));
						}
						else if (!EFileHelper::CopyFileToNewPath(g_PendingLevelFilePath, TargetPath))
						{
							g_LevelFileOperationError = EString(PE_TEXT("The level file could not be copied."));
						}
						else
						{
							g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
							g_PendingLevelFilePath = EString();
							g_bLevelListDirty = TRUE;
							ImGui::CloseCurrentPopup();
						}
					}
				}
				if (g_LevelFileOperationError.Length() > 0u)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *g_LevelFileOperationError);
				}
				ImGui::EndPopup();
			}

			if ((g_PendingLevelFileOperation == ELevelFileOperationType::REMOVE) &&
				ImGui::BeginPopupModal("Delete Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("Delete this level file? This cannot be undone.");
				ImGui::TextDisabled("%s", *SourceFileName);
				const BOOL32 bDeleteBlocked = ((bIsCurrentLevel) && (InManager->IsLevelDirty())) ? TRUE : FALSE;
				if (bDeleteBlocked)
				{
					ImGui::TextColored(ImVec4(0.9f, 0.8f, 0.2f, 1.0f),
						"Save or discard the open level's changes before deleting it.");
				}
				if (bDeleteBlocked)
				{
					ImGui::BeginDisabled();
				}
				if (ImGui::Button("Delete"))
				{
					g_LevelFileOperationError = EString();
					if (!EFileHelper::DeleteFile(g_PendingLevelFilePath))
					{
						g_LevelFileOperationError = EString(PE_TEXT("The level file could not be deleted."));
					}
					else
					{
						// When the deleted file was the current level, the manager clears the
						// path, drops the dirty flag and queues a new empty level.
						InManager->NotifyLevelFileDeleted(g_PendingLevelFilePath);
						g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
						g_PendingLevelFilePath = EString();
						g_bLevelListDirty = TRUE;
						ImGui::CloseCurrentPopup();
					}
				}
				if (bDeleteBlocked)
				{
					ImGui::EndDisabled();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					g_PendingLevelFileOperation = ELevelFileOperationType::NONE;
					g_PendingLevelFilePath = EString();
					ImGui::CloseCurrentPopup();
				}
				if (g_LevelFileOperationError.Length() > 0u)
				{
					ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *g_LevelFileOperationError);
				}
				ImGui::EndPopup();
			}
		}
	}

	namespace ELevelPanel
	{
		void RequestOpenPopup()
		{
			g_bOpenPopupRequested = TRUE;
		}

		void RequestSaveAsPopup()
		{
			g_bSaveAsPopupRequested = TRUE;
		}

		void RequestUnsavedPrompt()
		{
			g_bUnsavedPromptRequested = TRUE;
		}

		void MarkLevelListDirty()
		{
			g_bLevelListDirty = TRUE;
		}

		void GenerateLevelPanel(EWorldManager* InManager)
		{
			if (!InManager)
			{
				return;
			}
			if (g_bLevelListDirty)
			{
				RefreshLevelList();
			}

			const EString& CurrentLevelPath = InManager->GetCurrentLevelPath();
			const BOOL32 bLevelDirty = InManager->IsLevelDirty();

			ImGui::TextUnformatted("Current level:");
			ImGui::SameLine();
			if (CurrentLevelPath.Length() > 0u)
			{
				EString DisplayName = GetLevelFileName(CurrentLevelPath);
				if (bLevelDirty)
				{
					DisplayName += EString(PE_TEXT(" *"));
				}
				ImGui::TextUnformatted(*DisplayName);
			}
			else
			{
				ImGui::TextUnformatted(bLevelDirty ? "<unsaved> *" : "<unsaved>");
			}
			if (CurrentLevelPath.Length() > 0u)
			{
				ImGui::TextDisabled("%s", *CurrentLevelPath);
			}

			if (ImGui::Button("New"))
			{
				InManager->RequestNewLevel();
			}
			ImGui::SameLine();
			if (ImGui::Button("Save"))
			{
				if (CurrentLevelPath.Length() > 0u)
				{
					InManager->RequestSaveLevel(CurrentLevelPath);
				}
				else
				{
					RequestSaveAsPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Save As..."))
			{
				RequestSaveAsPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Reload"))
			{
				InManager->RequestReloadLevel();
			}
			ImGui::SameLine();
			if (ImGui::Button("Refresh"))
			{
				g_bLevelListDirty = TRUE;
			}

			if (g_bSaveAsPopupRequested)
			{
				g_bSaveAsPopupRequested = FALSE;
				EString InitialName = EString(PE_TEXT("NewLevel.PLevel"));
				if (CurrentLevelPath.Length() > 0u)
				{
					InitialName = GetLevelFileName(CurrentLevelPath);
				}
				const UINT32 NameLength = EMath::Min(InitialName.Length(), static_cast<UINT32>(sizeof(g_SaveAsNameBuffer) - 1u));
				memset(g_SaveAsNameBuffer, 0, sizeof(g_SaveAsNameBuffer));
				if (NameLength > 0u)
				{
					memcpy(g_SaveAsNameBuffer, *InitialName, NameLength);
				}
				ImGui::OpenPopup("Save Level As");
			}
			if (g_bUnsavedPromptRequested)
			{
				g_bUnsavedPromptRequested = FALSE;
				EString InitialName = EString(PE_TEXT("NewLevel.PLevel"));
				if (CurrentLevelPath.Length() > 0u)
				{
					InitialName = GetLevelFileName(CurrentLevelPath);
				}
				const UINT32 NameLength = EMath::Min(InitialName.Length(), static_cast<UINT32>(sizeof(g_SaveAsNameBuffer) - 1u));
				memset(g_SaveAsNameBuffer, 0, sizeof(g_SaveAsNameBuffer));
				if (NameLength > 0u)
				{
					memcpy(g_SaveAsNameBuffer, *InitialName, NameLength);
				}
				ImGui::OpenPopup("Unsaved Level Changes");
			}
			if (ImGui::BeginPopupModal("Unsaved Level Changes", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("The current level has unsaved changes.");
				if (CurrentLevelPath.Length() > 0u)
				{
					ImGui::TextDisabled("%s", *CurrentLevelPath);
				}
				else
				{
					ImGui::TextUnformatted("Save as:");
					ImGui::SetNextItemWidth(320.0f);
					ImGui::InputText("##UnsavedName", g_SaveAsNameBuffer, sizeof(g_SaveAsNameBuffer));
				}

				if (ImGui::Button("Save"))
				{
					EString SavePath = CurrentLevelPath;
					if (SavePath.Length() == 0u)
					{
						EString FileName(g_SaveAsNameBuffer);
						if ((FileName.Length() > 0u) && (!HasLevelExtension(FileName)))
						{
							FileName += EString(PE_TEXT(".PLevel"));
						}
						if (FileName.Length() > 0u)
						{
							SavePath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName;
						}
					}
					if (InManager->ConfirmUnsavedPromptWithSave(SavePath))
					{
						ImGui::CloseCurrentPopup();
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("Discard"))
				{
					InManager->ConfirmUnsavedPromptWithDiscard();
					ImGui::CloseCurrentPopup();
				}
				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					InManager->CancelPendingDiscardAction();
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}
			if (ImGui::BeginPopupModal("Save Level As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				ImGui::TextUnformatted("File name (saved under Assets/Levels):");
				ImGui::SetNextItemWidth(320.0f);
				ImGui::InputText("##SaveAsName", g_SaveAsNameBuffer, sizeof(g_SaveAsNameBuffer));
				if (ImGui::Button("Save"))
				{
					EString FileName(g_SaveAsNameBuffer);
					if ((FileName.Length() > 0u) && (!HasLevelExtension(FileName)))
					{
						FileName += EString(PE_TEXT(".PLevel"));
					}
					if (FileName.Length() > 0u)
					{
						InManager->RequestSaveLevel(EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
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
			DrawLevelFileOperationPopup(InManager);

			if (g_bOpenPopupRequested)
			{
				g_bOpenPopupRequested = FALSE;
				g_bLevelListDirty = TRUE;
				ImGui::OpenPopup("Open Level");
			}
			if (ImGui::BeginPopupModal("Open Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				if (g_bLevelListDirty)
				{
					RefreshLevelList();
				}
				if (g_LevelFiles.Num<UINT32>() == 0u)
				{
					ImGui::TextUnformatted("No .PLevel files under Assets/Levels.");
				}
				for (UINT32 Index = 0u, Num = g_LevelFiles.Num<UINT32>(); Index < Num; Index++)
				{
					const EString& FileName = g_LevelFiles[Index];
					ImGui::PushID(Index);
					if (ImGui::Selectable(*FileName))
					{
						InManager->RequestLoadLevel(EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
						ImGui::CloseCurrentPopup();
					}
					ImGui::PopID();
				}
				ImGui::Separator();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}
				ImGui::EndPopup();
			}

			ImGui::Separator();
			ImGui::TextUnformatted("Levels");
			if (ImGui::BeginChild("LevelList", ImVec2(0.0f, 180.0f), TRUE))
			{
				for (UINT32 Index = 0u, Num = g_LevelFiles.Num<UINT32>(); Index < Num; Index++)
				{
					const EString& FileName = g_LevelFiles[Index];
					const EString LevelPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName;
					ImGui::PushID(Index);
					const BOOL32 bCurrent = (CurrentLevelPath.Length() > 0u) &&
						(GetLevelFileName(CurrentLevelPath) == FileName) ? TRUE : FALSE;
					if (ImGui::Selectable(*FileName, bCurrent))
					{
						InManager->RequestLoadLevel(LevelPath);
					}
					if (ImGui::BeginPopupContextItem("##LevelContext"))
					{
						if (ImGui::MenuItem("Rename..."))
						{
							RequestRenameLevel(LevelPath);
						}
						if (ImGui::MenuItem("Duplicate..."))
						{
							RequestDuplicateLevel(LevelPath);
						}
						if (ImGui::MenuItem("Delete..."))
						{
							RequestDeleteLevel(LevelPath);
						}
						ImGui::EndPopup();
					}
					if (bCurrent)
					{
						ImGui::SameLine();
						ImGui::TextDisabled("(current)");
					}
					ImGui::PopID();
				}
			}
			ImGui::EndChild();
			ImGui::TextDisabled("Right-click a level to rename, duplicate or delete it.");
		}
	}
}

#endif
