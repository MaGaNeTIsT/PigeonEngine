#include "MainManager.h"
#include <Config/EngineConfig.h>
#include <RenderDevice/DeviceD3D11.h>

#ifdef _EDITOR_ONLY
#include "../../../EngineThirdParty/imGUI/Headers/imGUIManager.h"
#include "../../../EngineThirdParty/assimp/Headers/assimpManager.h"
#endif

namespace PigeonEngine
{
	EMainManager::EMainManager()
	{
		m_HWND			= nullptr;
		m_WindowSize	= Vector2Int(ESettings::ENGINE_SCREEN_WIDTH, ESettings::ENGINE_SCREEN_HEIGHT);
		m_GraphicDepth	= 24u;
		m_FrameRate		= ESettings::ENGINE_UPDATE_FRAME;
		m_Windowed		= ESettings::ENGINE_WINDOWED;

		m_RenderDeviceD3D11	= RDeviceD3D11::GetDeviceSingleton();
#if _EDITOR_ONLY
		m_ImGUIManager		= CImGUIManager::GetManagerSingleton();
		m_AssimpManager		= CAssimpManager::GetManagerSingleton();
#endif
	}
	EMainManager::~EMainManager()
	{
		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}
	}
	LRESULT EMainManager::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		return EInput::HandleMsg(hWnd, msg, wParam, lParam);
	}
	void EMainManager::Initialize()
	{
		EInput::Initialize(m_HWND);

		m_RenderDeviceD3D11->Init(m_HWND, m_WindowSize, m_GraphicDepth, m_FrameRate, m_Windowed);

#ifdef _EDITOR_ONLY
		m_ImGUIManager->Initialize();
		m_AssimpManager->Initialize();
#endif

		m_WindowTimer.Init();
	}
	void EMainManager::ShutDown()
	{
#ifdef _EDITOR_ONLY
		m_AssimpManager->ShutDown();
		m_ImGUIManager->ShutDown();
#endif

		m_RenderDeviceD3D11->Uninit();
		EInput::ShutDown();
	}
	void EMainManager::SetInitializer(HWND hWnd)
	{
		m_HWND = hWnd;
	}
	void EMainManager::UpdateManager()
	{
		m_WindowTimer.Update();
	}
	void EMainManager::Init()
	{
		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}
		m_GameTimer = new EGameTimer(&(this->m_WindowTimer));
#ifdef _EDITOR_ONLY
		m_EditorOpen = true;
#endif
		m_GameTimer->Reset();
	}
	void EMainManager::Uninit()
	{
		if (m_GameTimer)
		{
			delete m_GameTimer;
			m_GameTimer = nullptr;
		}
	}
	void EMainManager::Update()
	{
		m_GameTimer->Update();
#ifdef _EDITOR_ONLY
		m_ImGUIManager->Update();
		EMainManager::EditorUpdate();
#endif
	}
	void EMainManager::FixedUpdate()
	{

	}
	void EMainManager::Draw()
	{
#ifdef _EDITOR_ONLY
		m_ImGUIManager->Draw();
#endif
		m_RenderDeviceD3D11->Present();
	}
#ifdef _EDITOR_ONLY
	void EMainManager::EditorUpdate()
	{
		bool editorOpen = m_EditorOpen;
		ImGui::Begin("EditorRoot", &editorOpen, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Frame Rate Average %.3f ms/frame (%.1f FPS)", 1000.f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
		m_EditorOpen = editorOpen;
	}
#endif
	HWND EMainManager::GetWindowHandle()
	{
		return (m_HWND);
	}
	const EBaseTimer& EMainManager::GetWindowTimer()
	{
		return (m_WindowTimer);
	}
	const EGameTimer* EMainManager::GetGameTimer()
	{
		return (m_GameTimer);
	}
};