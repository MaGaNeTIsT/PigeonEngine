#include <Editor/EditorController.h>

#if ENGINE_EDITOR_ONLY

#include <imgui.h>

#include <HID/Input/Input.h>
#include <PigeonBase/Object/Component/CameraAndLight/CameraComponent.h>
#include <PigeonBase/Object/World/World.h>

namespace PigeonEngine
{
	PEditorController::PEditorController()
	{
		// The base constructor has already run by the time this body executes. The removed
		// "PController();" statement constructed (and immediately destroyed) an unnamed
		// PController, allocating a PCameraComponent on that temporary and leaking it; it
		// never initialized this object.
	}
	PEditorController::~PEditorController()
	{
	}

	void PEditorController::Init()
	{
		PController::Init();
		MyCamera->SetIsMainCamera(TRUE);
	}

	void PEditorController::EditorTick(FLOAT deltaTime)
	{
		PController::EditorTick(deltaTime);

		DrawImGui();
		if(!bStart)
		{
			return;
		}

		// rotation
		{
			Vector2 MousePosition = Vector2::Zero();
			MousePosition.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
			MousePosition.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);

			const Vector2 DeltaMouse = MousePosition - MousePositionLastTick;

			Quaternion CurrentRotation = this->GetActorRotation();

			const Vector3 WorldUpDir    = this->GetWorld() ? this->GetWorld()->GetUpVector() : Vector3::YVector();
			const Vector3 ActorRightDir = this->GetActorRightVector();
			Quaternion A = MakeQuaternion(ActorRightDir, DeltaMouse.y * RotationSpeed * deltaTime);
			Quaternion B = MakeQuaternion(WorldUpDir,    DeltaMouse.x * RotationSpeed * deltaTime);
			Quaternion Final = Quaternion::MultiplyQuaternion(CurrentRotation, A);
			Final = Quaternion::MultiplyQuaternion(Final, B);
			this->SetActorRotation(Final);
			MousePositionLastTick = MousePosition;
		}


		// position
		{
			Vector3 MovingDirection = Vector3::Zero();
			const BOOL8 bWPressed = EInput::Controller.IsKeyPressed(0x57);
			const BOOL8 bAPressed = EInput::Controller.IsKeyPressed(0x41);
			const BOOL8 bSPressed = EInput::Controller.IsKeyPressed(0x53);
			const BOOL8 bDPressed = EInput::Controller.IsKeyPressed(0x44);
			const BOOL8 bQPressed = EInput::Controller.IsKeyPressed(0x51);
			const BOOL8 bEPressed = EInput::Controller.IsKeyPressed(0x45);

			MovingDirection.z = bWPressed ? (bSPressed ? 0.0f :  1.0f) : (bSPressed ? -1.0f : 0.0f);
			MovingDirection.x = bAPressed ? (bDPressed ? 0.0f : -1.0f) : (bDPressed ?  1.0f : 0.0f);
			MovingDirection.y = bQPressed ? (bEPressed ? 0.0f : -1.0f) : (bEPressed ?  1.0f : 0.0f);

			Vector3 CurrentLoc = this->GetActorLocation();
			Vector3 WorldUpDir = this->GetWorld() ? this->GetWorld()->GetUpVector() : Vector3::YVector();
			this->SetActorLocation(CurrentLoc + (MovingDirection.x * this->GetActorRightVector() + MovingDirection.y * WorldUpDir + MovingDirection.z * this->GetActorForwardVector()) * MovingSpeed * deltaTime);
		}
	}

	void PEditorController::OnMouse(IMouse::Event::EType Type)
	{
		PController::OnMouse(Type);
		if(Type == IMouse::Event::EType::RPress)
		{
			MousePositionLastTick = Vector2();
			MousePositionLastTick.x = static_cast<FLOAT>(EInput::Controller.GetMousePosition().first);
			MousePositionLastTick.y = static_cast<FLOAT>(EInput::Controller.GetMousePosition().second);
			bStart = TRUE;
		}

		if(Type == IMouse::Event::EType::RRelease)
		{
			bStart = FALSE;
		}
		if(!bStart)
		{
			return;
		}
		if(Type == IMouse::Event::EType::WheelUp)
		{
			this->MovingSpeed += 10.0f;
		}
		if(Type == IMouse::Event::EType::WheelDown)
		{
			this->MovingSpeed += -10.0f;
		}

	}

	void PEditorController::OnKey(IKeyboard::Event::EType Type, BYTE KeyCode)
	{
		//  EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();
		PController::OnKey(Type, KeyCode);
		EString TestLog;
		switch (Type)
		{
		case IKeyboard::Event::EType::Press:
			{
				TestLog += "Press ";
				break;
			}
		case IKeyboard::Event::EType::Release:
			{
				TestLog += "Release ";
				break;
			}
		}


	}

	void PEditorController::DrawImGui()
	{
		ImGui::Begin("EditorController", FALSE, ImGuiWindowFlags_::ImGuiWindowFlags_NoTitleBar);
		ImGui::Text("Editor Controller Setup");
		/*const BOOL8 bChangedX = */ImGui::DragScalar("##MovingSpeed", ImGuiDataType_Float, &this->MovingSpeed, 0.05f, NULL, NULL, "MovingSpeed : %.6f");
		/*const BOOL8 bChangedY = */ImGui::DragScalar("##RotationSpeed", ImGuiDataType_Float, &this->RotationSpeed, 0.001f, NULL, NULL, "RotationSpeed : %.6f");
		ImGui::End();
	}

	namespace
	{
		// Installed before main - earlier than any world's CreateDefaultController - so the
		// world can create the editor fly camera without naming the concrete type.
		const BOOL32 bEditorControllerFactoryRegistered = []() -> BOOL32
		{
			PWorld::SetEditorControllerFactory([]() -> PController* { return new PEditorController(); });
			return TRUE;
		}();
	}
}

#endif
