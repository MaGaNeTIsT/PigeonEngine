#include "WorldManager.h"

#include "World.h"
#include "../../Development/Message/Message.h"
#include <Editor/EditorLogManager.h>
#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#include <Editor/WorldOutlinePanel.h>
#endif

namespace PigeonEngine
{
	static void RegisterClassTypes()
	{
		RegisterClassType<EWorldManager, EManagerBase>();
	}

	static void LogTestMessage(const EString& InMessage)
	{
		PE_LOG_LOG(EString("[TestMessage] ") + InMessage);
	}

	PE_REGISTER_CLASS_TYPE(&RegisterClassTypes);

	EWorldManager::EWorldManager()
	{
		this->World = nullptr;
	}

	EWorldManager::~EWorldManager()
	{

	}
	void EWorldManager::Initialize()
	{
		EManagerBase::Initialize();

		this->World = new PWorld();
	}

	void EWorldManager::ShutDown()
	{
		if(this->World)
		{
			this->World->Destroy();
			this->World = nullptr;
		}
		EManagerBase::ShutDown();
	}

	void EWorldManager::Init()
	{
		if(!this->World)
		{
			this->World = new PWorld();
		}
		this->World->Init();

		// The default demo is a level asset now. A missing file is a warning, not a fallback:
		// the world stays empty and the editor can create content or load another level.
		const EString DefaultLevelPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + EString(PE_TEXT("Demo.PLevel"));
		if (this->World->LoadLevel(DefaultLevelPath))
		{
			CurrentLevelPath = DefaultLevelPath;
			LevelRevision += 1u;
		}
		else
		{
			CurrentLevelPath = EString();
			PE_LOG_WARN(EString(PE_TEXT("World: default level missing or failed to load: ")) + DefaultLevelPath);
		}
	}

	void EWorldManager::Uninit()
	{
		if(this->World)
		{
			this->World->Destroy();
			this->World = nullptr;
		}
	}

	void EWorldManager::RequestNewLevel()
	{
#if ENGINE_EDITOR_ONLY
		if (bLevelDirty)
		{
			PendingDiscardRequest = ELevelRequestType::NEW;
			PendingDiscardPath = EString();
			bUnsavedPromptRequested = TRUE;
			return;
		}
#endif
		PendingLevelRequest = ELevelRequestType::NEW;
		PendingLevelPath = EString();
	}

	void EWorldManager::RequestSaveLevel(const EString& InPath)
	{
		PendingLevelRequest = ELevelRequestType::SAVE;
		PendingLevelPath = InPath;
	}

	void EWorldManager::RequestLoadLevel(const EString& InPath)
	{
#if ENGINE_EDITOR_ONLY
		if (bLevelDirty)
		{
			PendingDiscardRequest = ELevelRequestType::LOAD;
			PendingDiscardPath = InPath;
			bUnsavedPromptRequested = TRUE;
			return;
		}
#endif
		PendingLevelRequest = ELevelRequestType::LOAD;
		PendingLevelPath = InPath;
	}

	void EWorldManager::RequestReloadLevel()
	{
		if (CurrentLevelPath.Length() > 0u)
		{
			RequestLoadLevel(CurrentLevelPath);
		}
	}

	const EString& EWorldManager::GetCurrentLevelPath() const
	{
		return CurrentLevelPath;
	}

	UINT64 EWorldManager::GetLevelRevision() const
	{
		return LevelRevision;
	}

	BOOL32 EWorldManager::IsLevelDirty() const
	{
		return bLevelDirty;
	}

	void EWorldManager::MarkLevelDirty()
	{
		bLevelDirty = TRUE;
	}

	BOOL32 EWorldManager::SaveLevelNow(const EString& InPath)
	{
		if ((!this->World) || (InPath.Length() == 0u))
		{
			return FALSE;
		}
		if (!this->World->SaveLevel(InPath))
		{
			return FALSE;
		}
		CurrentLevelPath = InPath;
		bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
		bLevelListDirty = TRUE;
#endif
		return TRUE;
	}

	void EWorldManager::ProcessPendingLevelRequest()
	{
		if ((!this->World) || (PendingLevelRequest == ELevelRequestType::NONE))
		{
			return;
		}
		const ELevelRequestType Request = PendingLevelRequest;
		const EString Path = PendingLevelPath;
		PendingLevelRequest = ELevelRequestType::NONE;
		PendingLevelPath = EString();

		switch (Request)
		{
		case ELevelRequestType::NEW:
			this->World->ClearLevelActors();
#if ENGINE_EDITOR_ONLY
			this->World->SetSelectedActor(nullptr);
			this->World->SetSelectedComponent(nullptr);
#endif
			this->World->SetUpVector(Vector3::YVector());
			this->World->SetGravitySettings(FWorldGravitySettings());
			CurrentLevelPath = EString();
			LevelRevision += 1u;
			bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
			bLevelListDirty = TRUE;
#endif
			break;
		case ELevelRequestType::SAVE:
			if (this->World->SaveLevel(Path))
			{
				CurrentLevelPath = Path;
				bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
				bLevelListDirty = TRUE;
#endif
			}
			break;
		case ELevelRequestType::LOAD:
			if (this->World->LoadLevel(Path))
			{
				CurrentLevelPath = Path;
				LevelRevision += 1u;
				bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
				bLevelListDirty = TRUE;
#endif
			}
			break;
		default:
			break;
		}
	}

#if ENGINE_EDITOR_ONLY
	namespace
	{
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
	}


	void EWorldManager::RequestOpenLevelPopup()
	{
		bOpenLevelPopupRequested = TRUE;
	}

	void EWorldManager::RequestSaveAsPopup()
	{
		bSaveAsPopupRequested = TRUE;
	}

	void EWorldManager::RequestRenameLevel(const EString& InPath)
	{
		PendingLevelFileOperation = ELevelFileOperationType::RENAME;
		PendingLevelFilePath = InPath;
		LevelFileOperationError = EString();
		const EString Stem = GetLevelStem(InPath);
		const UINT32 NameLength = EMath::Min(Stem.Length(), static_cast<UINT32>(sizeof(LevelFileNameBuffer) - 1u));
		memset(LevelFileNameBuffer, 0, sizeof(LevelFileNameBuffer));
		if (NameLength > 0u)
		{
			memcpy(LevelFileNameBuffer, *Stem, NameLength);
		}
		bLevelFileOperationPopupRequested = TRUE;
	}

	void EWorldManager::RequestDuplicateLevel(const EString& InPath)
	{
		PendingLevelFileOperation = ELevelFileOperationType::DUPLICATE;
		PendingLevelFilePath = InPath;
		LevelFileOperationError = EString();
		const EString Stem = GetLevelStem(InPath) + EString(PE_TEXT("_Copy"));
		const UINT32 NameLength = EMath::Min(Stem.Length(), static_cast<UINT32>(sizeof(LevelFileNameBuffer) - 1u));
		memset(LevelFileNameBuffer, 0, sizeof(LevelFileNameBuffer));
		if (NameLength > 0u)
		{
			memcpy(LevelFileNameBuffer, *Stem, NameLength);
		}
		bLevelFileOperationPopupRequested = TRUE;
	}

	void EWorldManager::RequestDeleteLevel(const EString& InPath)
	{
		PendingLevelFileOperation = ELevelFileOperationType::REMOVE;
		PendingLevelFilePath = InPath;
		LevelFileOperationError = EString();
		bLevelFileOperationPopupRequested = TRUE;
	}

	void EWorldManager::ExecutePendingDiscardAction()
	{
		const ELevelRequestType Request = PendingDiscardRequest;
		const EString Path = PendingDiscardPath;
		PendingDiscardRequest = ELevelRequestType::NONE;
		PendingDiscardPath = EString();
		bLevelDirty = FALSE;
		if (Request == ELevelRequestType::NEW)
		{
			RequestNewLevel();
		}
		else if (Request == ELevelRequestType::LOAD)
		{
			RequestLoadLevel(Path);
		}
	}

	void EWorldManager::RefreshLevelList()
	{
		LevelFiles.Empty();
		TArray<EString> Folders;
		TArray<EString> Files;
		if (EFileHelper::ScanDirectory(EString(EBaseSettings::ENGINE_LEVEL_PATH), Folders, Files, FALSE))
		{
			for (UINT32 Index = 0u, Num = Files.Num<UINT32>(); Index < Num; Index++)
			{
				if (HasLevelExtension(Files[Index]))
				{
					LevelFiles.Add(Files[Index]);
				}
			}
		}
		// ScanDirectory's order is filesystem order; sort so the list is stable.
		for (INT32 Index = 1, Num = LevelFiles.Num<INT32>(); Index < Num; Index++)
		{
			for (INT32 Insert = Index; (Insert > 0) && (LevelFiles[Insert] < LevelFiles[Insert - 1]); Insert--)
			{
				const EString Temp = LevelFiles[Insert];
				LevelFiles[Insert] = LevelFiles[Insert - 1];
				LevelFiles[Insert - 1] = Temp;
			}
		}
		bLevelListDirty = FALSE;
	}

	void EWorldManager::DrawLevelFileOperationPopup()
	{
		if (PendingLevelFileOperation == ELevelFileOperationType::NONE)
		{
			return;
		}
		if (bLevelFileOperationPopupRequested)
		{
			bLevelFileOperationPopupRequested = FALSE;
			switch (PendingLevelFileOperation)
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

		const EString SourceFileName = GetLevelFileName(PendingLevelFilePath);
		const BOOL32 bIsCurrentLevel = (CurrentLevelPath == PendingLevelFilePath) ? TRUE : FALSE;

		if ((PendingLevelFileOperation == ELevelFileOperationType::RENAME) &&
			ImGui::BeginPopupModal("Rename Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Rename:");
			ImGui::TextDisabled("%s", *SourceFileName);
			ImGui::SetNextItemWidth(320.0f);
			BOOL32 bConfirm = ImGui::InputText("##RenameLevelName", LevelFileNameBuffer, sizeof(LevelFileNameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue);
			if (ImGui::Button("Rename"))
			{
				bConfirm = TRUE;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				PendingLevelFileOperation = ELevelFileOperationType::NONE;
				PendingLevelFilePath = EString();
				ImGui::CloseCurrentPopup();
			}

			if (bConfirm)
			{
				LevelFileOperationError = EString();
				EString TargetFileName(LevelFileNameBuffer);
				if (TargetFileName.Length() == 0u)
				{
					LevelFileOperationError = EString(PE_TEXT("The file name cannot be empty."));
				}
				else
				{
					if (!HasLevelExtension(TargetFileName))
					{
						TargetFileName += EString(PE_TEXT(".PLevel"));
					}
					if (TargetFileName == SourceFileName)
					{
						LevelFileOperationError = EString(PE_TEXT("The new name is the same as the current name."));
					}
					else
					{
						const EString TargetPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + TargetFileName;
						if (EFileHelper::IsFileExists(TargetPath))
						{
							LevelFileOperationError = EString(PE_TEXT("A level with that name already exists."));
						}
						else if (!EFileHelper::MoveFile(PendingLevelFilePath, TargetPath))
						{
							LevelFileOperationError = EString(PE_TEXT("The level file could not be renamed."));
						}
						else
						{
							if (bIsCurrentLevel)
							{
								CurrentLevelPath = TargetPath;
							}
							PendingLevelFileOperation = ELevelFileOperationType::NONE;
							PendingLevelFilePath = EString();
							bLevelListDirty = TRUE;
							ImGui::CloseCurrentPopup();
						}
					}
				}
			}
			if (LevelFileOperationError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *LevelFileOperationError);
			}
			ImGui::EndPopup();
		}

		if ((PendingLevelFileOperation == ELevelFileOperationType::DUPLICATE) &&
			ImGui::BeginPopupModal("Duplicate Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Duplicate:");
			ImGui::TextDisabled("%s", *SourceFileName);
			ImGui::SetNextItemWidth(320.0f);
			BOOL32 bConfirm = ImGui::InputText("##DuplicateLevelName", LevelFileNameBuffer, sizeof(LevelFileNameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue);
			if (ImGui::Button("Duplicate"))
			{
				bConfirm = TRUE;
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				PendingLevelFileOperation = ELevelFileOperationType::NONE;
				PendingLevelFilePath = EString();
				ImGui::CloseCurrentPopup();
			}

			if (bConfirm)
			{
				LevelFileOperationError = EString();
				EString TargetFileName(LevelFileNameBuffer);
				if (TargetFileName.Length() == 0u)
				{
					LevelFileOperationError = EString(PE_TEXT("The file name cannot be empty."));
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
						LevelFileOperationError = EString(PE_TEXT("A level with that name already exists."));
					}
					else if (!EFileHelper::CopyFileToNewPath(PendingLevelFilePath, TargetPath))
					{
						LevelFileOperationError = EString(PE_TEXT("The level file could not be copied."));
					}
					else
					{
						PendingLevelFileOperation = ELevelFileOperationType::NONE;
						PendingLevelFilePath = EString();
						bLevelListDirty = TRUE;
						ImGui::CloseCurrentPopup();
					}
				}
			}
			if (LevelFileOperationError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *LevelFileOperationError);
			}
			ImGui::EndPopup();
		}

		if ((PendingLevelFileOperation == ELevelFileOperationType::REMOVE) &&
			ImGui::BeginPopupModal("Delete Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("Delete this level file? This cannot be undone.");
			ImGui::TextDisabled("%s", *SourceFileName);
			const BOOL32 bDeleteBlocked = ((bIsCurrentLevel) && (bLevelDirty)) ? TRUE : FALSE;
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
				LevelFileOperationError = EString();
				if (!EFileHelper::DeleteFile(PendingLevelFilePath))
				{
					LevelFileOperationError = EString(PE_TEXT("The level file could not be deleted."));
				}
				else
				{
					if (bIsCurrentLevel)
					{
						CurrentLevelPath = EString();
						bLevelDirty = FALSE;
						RequestNewLevel();
					}
					PendingLevelFileOperation = ELevelFileOperationType::NONE;
					PendingLevelFilePath = EString();
					bLevelListDirty = TRUE;
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
				PendingLevelFileOperation = ELevelFileOperationType::NONE;
				PendingLevelFilePath = EString();
				ImGui::CloseCurrentPopup();
			}
			if (LevelFileOperationError.Length() > 0u)
			{
				ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "%s", *LevelFileOperationError);
			}
			ImGui::EndPopup();
		}
	}

	void EWorldManager::DrawLevelPanelContents()
	{
		if (bLevelListDirty)
		{
			RefreshLevelList();
		}

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
			RequestNewLevel();
		}
		ImGui::SameLine();
		if (ImGui::Button("Save"))
		{
			if (CurrentLevelPath.Length() > 0u)
			{
				RequestSaveLevel(CurrentLevelPath);
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
			RequestReloadLevel();
		}
		ImGui::SameLine();
		if (ImGui::Button("Refresh"))
		{
			bLevelListDirty = TRUE;
		}

		if (bSaveAsPopupRequested)
		{
			bSaveAsPopupRequested = FALSE;
			EString InitialName = EString(PE_TEXT("NewLevel.PLevel"));
			if (CurrentLevelPath.Length() > 0u)
			{
				InitialName = GetLevelFileName(CurrentLevelPath);
			}
			const UINT32 NameLength = EMath::Min(InitialName.Length(), static_cast<UINT32>(sizeof(SaveAsNameBuffer) - 1u));
			memset(SaveAsNameBuffer, 0, sizeof(SaveAsNameBuffer));
			if (NameLength > 0u)
			{
				memcpy(SaveAsNameBuffer, *InitialName, NameLength);
			}
			ImGui::OpenPopup("Save Level As");
		}
		if (bUnsavedPromptRequested)
		{
			bUnsavedPromptRequested = FALSE;
			EString InitialName = EString(PE_TEXT("NewLevel.PLevel"));
			if (CurrentLevelPath.Length() > 0u)
			{
				InitialName = GetLevelFileName(CurrentLevelPath);
			}
			const UINT32 NameLength = EMath::Min(InitialName.Length(), static_cast<UINT32>(sizeof(SaveAsNameBuffer) - 1u));
			memset(SaveAsNameBuffer, 0, sizeof(SaveAsNameBuffer));
			if (NameLength > 0u)
			{
				memcpy(SaveAsNameBuffer, *InitialName, NameLength);
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
				ImGui::InputText("##UnsavedName", SaveAsNameBuffer, sizeof(SaveAsNameBuffer));
			}

			if (ImGui::Button("Save"))
			{
				EString SavePath = CurrentLevelPath;
				if (SavePath.Length() == 0u)
				{
					EString FileName(SaveAsNameBuffer);
					if ((FileName.Length() > 0u) && (!HasLevelExtension(FileName)))
					{
						FileName += EString(PE_TEXT(".PLevel"));
					}
					if (FileName.Length() > 0u)
					{
						SavePath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName;
					}
				}
				if (SaveLevelNow(SavePath))
				{
					ExecutePendingDiscardAction();
					ImGui::CloseCurrentPopup();
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Discard"))
			{
				bLevelDirty = FALSE;
				ExecutePendingDiscardAction();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Cancel"))
			{
				PendingDiscardRequest = ELevelRequestType::NONE;
				PendingDiscardPath = EString();
				ImGui::CloseCurrentPopup();
			}
			ImGui::EndPopup();
		}
		if (ImGui::BeginPopupModal("Save Level As", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			ImGui::TextUnformatted("File name (saved under Assets/Levels):");
			ImGui::SetNextItemWidth(320.0f);
			ImGui::InputText("##SaveAsName", SaveAsNameBuffer, sizeof(SaveAsNameBuffer));
			if (ImGui::Button("Save"))
			{
				EString FileName(SaveAsNameBuffer);
				if ((FileName.Length() > 0u) && (!HasLevelExtension(FileName)))
				{
					FileName += EString(PE_TEXT(".PLevel"));
				}
				if (FileName.Length() > 0u)
				{
					RequestSaveLevel(EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
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
		this->DrawLevelFileOperationPopup();

		if (bOpenLevelPopupRequested)
		{
			bOpenLevelPopupRequested = FALSE;
			bLevelListDirty = TRUE;
			ImGui::OpenPopup("Open Level");
		}
		if (ImGui::BeginPopupModal("Open Level", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
		{
			if (bLevelListDirty)
			{
				RefreshLevelList();
			}
			if (LevelFiles.Num<UINT32>() == 0u)
			{
				ImGui::TextUnformatted("No .PLevel files under Assets/Levels.");
			}
			for (UINT32 Index = 0u, Num = LevelFiles.Num<UINT32>(); Index < Num; Index++)
			{
				const EString& FileName = LevelFiles[Index];
				ImGui::PushID(Index);
				if (ImGui::Selectable(*FileName))
				{
					RequestLoadLevel(EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
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
			for (UINT32 Index = 0u, Num = LevelFiles.Num<UINT32>(); Index < Num; Index++)
			{
				const EString& FileName = LevelFiles[Index];
				const EString LevelPath = EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName;
				ImGui::PushID(Index);
				const BOOL32 bCurrent = (CurrentLevelPath.Length() > 0u) &&
					(GetLevelFileName(CurrentLevelPath) == FileName) ? TRUE : FALSE;
				if (ImGui::Selectable(*FileName, bCurrent))
				{
					RequestLoadLevel(LevelPath);
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
#endif

#if ENGINE_EDITOR_ONLY
	void EWorldManager::EditorInit()
	{
	}

	void EWorldManager::EditorUpdate()
	{
		if (!this->GetWorld())
		{
			return;
		}

		// ---- Level panel: current file plus New/Open/Save/Save As/Reload ----
		ImGui::SetNextWindowSize(ImVec2(360, 330), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(300, 30), ImGuiCond_FirstUseEver);
		ImGui::Begin("Level");
		this->DrawLevelPanelContents();
		ImGui::End();

		// ---- World Outline panel ----
		ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
		ImGui::Begin("World Outline");
		EWorldOutlinePanel::GenerateOutline(this->GetWorld());
		ImGui::End();

		// ---- Details panel ----
		ImGui::SetNextWindowSize(ImVec2(280, 400), ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowPos(ImVec2(10, 440), ImGuiCond_FirstUseEver);
		ImGui::Begin("Details");
		this->GetWorld()->GenerateDetail();
		ImGui::End();
	}
#endif
	PWorld* EWorldManager::GetWorld() const
	{
		return this->World;
	}
}

