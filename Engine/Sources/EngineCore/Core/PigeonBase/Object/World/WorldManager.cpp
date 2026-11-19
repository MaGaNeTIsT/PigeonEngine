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
		PendingLevelRequest = ELevelRequestType::LOAD;
		PendingLevelPath = InPath;
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
			break;
		case ELevelRequestType::SAVE:
			this->World->SaveLevel(Path);
			break;
		case ELevelRequestType::LOAD:
			this->World->LoadLevel(Path);
			break;
		default:
			break;
		}
	}

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

