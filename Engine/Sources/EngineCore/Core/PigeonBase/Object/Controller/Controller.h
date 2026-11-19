#pragma once
#include "Input/InputType.h"
#include "PigeonBase/Object/Object.h"
#include "PigeonBase/Object/Actor.h"
namespace PigeonEngine
{
	MAKE_DELEGATE_MULTI_TWO_PARAM(OnMouseEvent, IMouse::Event::EType,  const Vector2&);
	MAKE_DELEGATE_MULTI_TWO_PARAM(OnKeyEvent,   IKeyboard::Event::EType, const EKey&);
	class PCameraComponent;
	class PController : public PActor
	{
		friend class PWorld;
	protected:
		// for frame, 
		void BeginAddedToScene(PWorld* World) override;
		void RemovedFromScene() override;

		void Init() override;
		void Uninit() override;

		// for users
		void UserBeginPlay() override;
		void UserTick(FLOAT deltaTime) override;
		void UserEndPlay() override;
	protected:
		virtual void OnMouse(IMouse::Event::EType Type) ;
		virtual void OnKey(IKeyboard::Event::EType Type, BYTE KeyCode) ;
	public:
		OnMouseEvent OnMouseEvent;
		OnKeyEvent   OnKeyEvent;
		// Controllers are created by the world (editor or runtime), never by a level file.
		PE_NODISCARD virtual BOOL32 ShouldSerializeToLevel() const override { return FALSE; }
	private:
		// Handles of the global input subscriptions taken in Init and released in Uninit.
		// Input events are dispatched straight from the Win32 message pump, which can
		// fire at any point between frames, so these must come off before the object is
		// deleted; the handle form works without keeping the TFunction copies alive.
		FDelegateHandle OnMouseDownHandle = 0u;
		FDelegateHandle OnKeyDownHandle = 0u;
	public:

		PE_NODISCARD PCameraComponent* GetCamera() const;
		void SetCamera(PCameraComponent* NewCam, BOOL8 InIsMainCamera);
		void ResetToDefaultCamera();

	protected:
		PCameraComponent* MyCamera = nullptr;

	public:
		void SetActorLocation(const Vector3& Location) override;
		void SetActorRotation(const Quaternion& Rotation) override;
		void SetActorScale(const Vector3& Scale) override;
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PController)
	};

#if ENGINE_EDITOR_ONLY


	class PEditorController : public PController
	{
	public:
		CLASS_VIRTUAL_NOCOPY_BODY(PEditorController)
		void Init() override;
		void EditorTick(FLOAT deltaTime) override;
		void DrawImGui();
	protected:
		void OnMouse(IMouse::Event::EType Type) override;
		void OnKey(IKeyboard::Event::EType Type, BYTE KeyCode) override;

	private:

		BOOL8 bStart = FALSE;
		FLOAT MovingSpeed = 400.0;
		FLOAT RotationSpeed = 0.25f;
		Vector2 MousePositionLastTick = Vector2::Zero();
	};
#endif

}

