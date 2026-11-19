#include "WorldManager.h"

#include "World.h"
#include "../../Development/Message/Message.h"
#include <Editor/EditorLogManager.h>
#if ENGINE_EDITOR_ONLY
#include <imgui.h>
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
	}

	void EWorldManager::RequestOpenLevelPopup()
	{
		bOpenLevelPopupRequested = TRUE;
	}

	void EWorldManager::RequestSaveAsPopup()
	{
		bSaveAsPopupRequested = TRUE;
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
				ImGui::PushID(Index);
				const BOOL32 bCurrent = (CurrentLevelPath.Length() > 0u) &&
					(GetLevelFileName(CurrentLevelPath) == FileName) ? TRUE : FALSE;
				if (ImGui::Selectable(*FileName, bCurrent))
				{
					RequestLoadLevel(EString(EBaseSettings::ENGINE_LEVEL_PATH) + FileName);
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
		this->GetWorld()->GenerateWorldOutline();
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

