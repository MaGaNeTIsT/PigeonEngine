#include "Controller.h"

#include <Development/EditorLogManager.h>
#include "../../../../HID/Input/InputType.h"
#include "PigeonBase/Object/Component/CameraAndLight/CameraComponent.h"
#include "PigeonBase/Object/World/World.h"

namespace PigeonEngine
{
	void PController::BeginAddedToScene(PWorld* World)
	{
		PActor::BeginAddedToScene(World);
	}

	void PController::RemovedFromScene()
	{
		PActor::RemovedFromScene();
	}

	void PController::Init()
	{
		PActor::Init();
		this->SetIsTickable(TRUE);
		SetCamera(MyCamera, FALSE);
		OnMouseDownHandle = EInput::MouseEvent.Add(TFunction<void(IMouse::Event::EType)>([this](IMouse::Event::EType Type)
		{
			this->OnMouse(Type);
		}));
		OnKeyDownHandle = EInput::KeyEvent.Add(TFunction<void(IKeyboard::Event::EType, BYTE)>([this](IKeyboard::Event::EType Type, BYTE KeyCode)
		{
			this->OnKey(Type, KeyCode);
		}));
	}

	void PController::Uninit()
	{
		// Detach from the global input delegates while the object is still alive. An
		// unreleased subscription outlives this object as a dangling callback that the
		// message pump can invoke between any two frames.
		if (OnMouseDownHandle != 0u)
		{
			EInput::MouseEvent.Remove(OnMouseDownHandle);
			OnMouseDownHandle = 0u;
		}
		if (OnKeyDownHandle != 0u)
		{
			EInput::KeyEvent.Remove(OnKeyDownHandle);
			OnKeyDownHandle = 0u;
		}
		PActor::Uninit();
	}

	void PController::UserBeginPlay()
	{
		PActor::UserBeginPlay();
	}

	void PController::UserTick(FLOAT deltaTime)
	{
		PActor::UserTick(deltaTime);
	}

	void PController::UserEndPlay()
	{
		PActor::UserEndPlay();
	}

	void PController::OnMouse(IMouse::Event::EType Type)
	{
		Vector2 MousePosition = Vector2();
		MousePosition.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
		MousePosition.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
		this->OnMouseEvent.Broadcast(Type, MousePosition);
	}

	void PController::OnKey(IKeyboard::Event::EType Type, BYTE KeyCode)
	{
		this->OnKeyEvent.Broadcast(Type, EKeysBuiltIn::GetKeyByKeyCode(KeyCode));
	}

	PCameraComponent* PController::GetCamera() const
	{
		return this->MyCamera;
	}

	void PController::SetCamera(PCameraComponent* NewCam, BOOL8 InIsMainCamera)
	{
		if (this->MyCamera)
		{
			this->MyCamera->SetIsMainCamera(FALSE);
		}
		this->MyCamera = NewCam;
		if (InIsMainCamera)
		{
			this->MyCamera->SetIsMainCamera(TRUE);
		}
		this->MyCamera->SetIsTickable(TRUE);
	}

	void PController::ResetToDefaultCamera()
	{
		PCameraComponent* MyCam = GetRootComponent()->AsType<PCameraComponent>();
		if(MyCam)
		{
			SetCamera(MyCam, FALSE);
		}
	}

	void PController::SetActorLocation(const Vector3& Location)
	{
		PActor::SetActorLocation(Location);
	}

	void PController::SetActorRotation(const Quaternion& Rotation)
	{
		PActor::SetActorRotation(Rotation);
	}

	void PController::SetActorScale(const Vector3& Scale)
	{
		PActor::SetActorScale(Scale);
	}

	PController::PController()
	{
		MyCamera = new PCameraComponent(0.f, 0.f, EEngineSettings::ENGINE_SCREEN_WIDTH, EEngineSettings::ENGINE_SCREEN_HEIGHT);
		this->SetRootComponent(MyCamera);
	}
	PController::~PController()
	{
	}

}
