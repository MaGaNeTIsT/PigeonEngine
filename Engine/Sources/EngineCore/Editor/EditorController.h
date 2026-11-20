#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

#include <PigeonBase/Object/Controller/Controller.h>

namespace PigeonEngine
{
	/*
	* The editor's fly camera: WASD/QE movement, right-drag mouse look, wheel speed scaling,
	* and the small setup window. The world creates it through the factory this module
	* registers before main, so the runtime never references the concrete type.
	*/
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
}

#endif
