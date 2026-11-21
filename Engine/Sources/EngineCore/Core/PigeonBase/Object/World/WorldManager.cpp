#include "WorldManager.h"

#include "World.h"
#include "../../Development/Message/Message.h"
#include <Development/EditorLogManager.h>
#include <Navigation/NavigationManager.h>
#if ENGINE_EDITOR_ONLY
#include <imgui.h>
#include <Editor/Panels/WorldOutlinePanel.h>
#include <Editor/Panels/WorldDetailsPanel.h>
#include <Editor/Manager/EditorWorldContext.h>
#include <Editor/Panels/LevelPanel.h>
#include "PigeonBase/Object/Component/Navigation/NavMeshComponent.h"
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

	// Level stem, derived the same way PWorld::SaveLevel derives the header name. The world
	// build uses it for generated navmesh asset names (<stem>_NavMesh, <stem>_<profile>_NavMesh).
	static EString MakeLevelStem(const EString& InLevelPath)
	{
		EString LevelStem = InLevelPath;
		const INT32 LastSlash = LevelStem.RightFind(EString(PE_TEXT("/")));
		if (LastSlash >= 0)
		{
			LevelStem = LevelStem.Right(LevelStem.Length() - static_cast<UINT32>(LastSlash) - 1u);
		}
		const INT32 LastDot = LevelStem.RightFind(EString(PE_TEXT(".")));
		if (LastDot > 0)
		{
			LevelStem = LevelStem.Left(static_cast<UINT32>(LastDot));
		}
		return LevelStem;
	}

	// A loaded level builds all of its navmesh profiles in one pass. The manager waits for the
	// asset queue once, then imports assigned assets and bakes the profiles without one.
	static void RequestNavigationBuildOnLevelLoad(PWorld* InWorld)
	{
		if (!InWorld)
		{
			return;
		}
		ENavigationManager* Navigation = ENavigationManager::GetManagerSingleton();
#if ENGINE_EDITOR_ONLY
		Navigation->RequestWorldBuild(*InWorld, ENavigationManager::ENavigationWorldBuildReason::LOAD);
#else
		Navigation->RequestWorldBuild(*InWorld, ENavigationManager::ENavigationWorldBuildReason::RUNTIME);
#endif
	}

#if ENGINE_EDITOR_ONLY
	// Save = build paths + save: every profile is baked from the world and its asset written, so
	// the level and the files it references describe the same world.
	static void BuildNavigationForSave(PWorld* InWorld, const EString& InLevelPath)
	{
		if (!InWorld)
		{
			return;
		}
		ENavigationManager::FNavigationWorldBuildReport Report;
		ENavigationManager::GetManagerSingleton()->BuildWorldSynchronously(*InWorld, MakeLevelStem(InLevelPath), Report);
	}
#endif

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
			ENavigationManager::GetManagerSingleton()->NotifyWorldDestroyed(this->World);
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
			RequestNavigationBuildOnLevelLoad(this->World);
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
			ENavigationManager::GetManagerSingleton()->NotifyWorldDestroyed(this->World);
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
			ELevelPanel::RequestUnsavedPrompt();
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
			ELevelPanel::RequestUnsavedPrompt();
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
#if ENGINE_EDITOR_ONLY
		// Before the serializer walks the properties: every profile is baked and its asset
		// written, so the saved file's references match the world being saved.
		BuildNavigationForSave(this->World, InPath);
#endif
		if (!this->World->SaveLevel(InPath))
		{
			return FALSE;
		}
		CurrentLevelPath = InPath;
		bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
		ELevelPanel::MarkLevelListDirty();
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
			// The previous level's build pass and input watch do not describe this empty world.
			ENavigationManager::GetManagerSingleton()->NotifyWorldDestroyed(this->World);
#if ENGINE_EDITOR_ONLY
			EEditorWorldContext::ClearSelection(this->World);
#endif
			this->World->SetUpVector(Vector3::YVector());
			this->World->SetGravitySettings(FWorldGravitySettings());
			CurrentLevelPath = EString();
			LevelRevision += 1u;
			bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
			ELevelPanel::MarkLevelListDirty();
#endif
			break;
		case ELevelRequestType::SAVE:
#if ENGINE_EDITOR_ONLY
			// Same contract as SaveLevelNow: every profile is rebuilt and its asset written.
			BuildNavigationForSave(this->World, Path);
#endif
			if (this->World->SaveLevel(Path))
			{
				CurrentLevelPath = Path;
				bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
				ELevelPanel::MarkLevelListDirty();
#endif
			}
			break;
		case ELevelRequestType::LOAD:
			if (this->World->LoadLevel(Path))
			{
				CurrentLevelPath = Path;
				LevelRevision += 1u;
				bLevelDirty = FALSE;
				// The new level's profiles replace whatever the previous one left behind; one
				// pass imports their assets and bakes the profiles without one.
				ENavigationManager::GetManagerSingleton()->NotifyWorldDestroyed(this->World);
				RequestNavigationBuildOnLevelLoad(this->World);
#if ENGINE_EDITOR_ONLY
				ELevelPanel::MarkLevelListDirty();
#endif
			}
			break;
		default:
			break;
		}
	}

#if ENGINE_EDITOR_ONLY
	void EWorldManager::RequestOpenLevelPopup()
	{
		ELevelPanel::RequestOpenPopup();
	}

	void EWorldManager::RequestSaveAsPopup()
	{
		ELevelPanel::RequestSaveAsPopup();
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

	BOOL32 EWorldManager::ConfirmUnsavedPromptWithSave(const EString& InSavePath)
	{
		if (!SaveLevelNow(InSavePath))
		{
			return FALSE;
		}
		ExecutePendingDiscardAction();
		return TRUE;
	}

	void EWorldManager::ConfirmUnsavedPromptWithDiscard()
	{
		ExecutePendingDiscardAction();
	}

	void EWorldManager::CancelPendingDiscardAction()
	{
		PendingDiscardRequest = ELevelRequestType::NONE;
		PendingDiscardPath = EString();
	}

	void EWorldManager::NotifyLevelFileRenamed(const EString& InOldPath, const EString& InNewPath)
	{
		if (CurrentLevelPath == InOldPath)
		{
			CurrentLevelPath = InNewPath;
		}
	}

	void EWorldManager::NotifyLevelFileDeleted(const EString& InPath)
	{
		if (CurrentLevelPath != InPath)
		{
			return;
		}
		CurrentLevelPath = EString();
		bLevelDirty = FALSE;
		RequestNewLevel();
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
		ELevelPanel::GenerateLevelPanel(this);
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
		EWorldDetailsPanel::GenerateDetails(this->GetWorld());
		ImGui::End();
	}
#endif
	PWorld* EWorldManager::GetWorld() const
	{
		return this->World;
	}
}

