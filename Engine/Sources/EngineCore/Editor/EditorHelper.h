#pragma once

#if _EDITOR_ONLY

#include <CoreMinimal.h>
#include <EngineCommon.h>

namespace PigeonEngine
{

	extern class PActor* SelectObjectInViewport(const class PCameraComponent* InCamera, TArray<class PActor*>& InActors, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect);

	extern class PSceneComponent* SelectObjectInViewport(const class PCameraComponent* InCamera, TArray<class PSceneComponent*>& InActors, const FLOAT InMouseX, const FLOAT InMouseY, const ERect& InScreenRect);

	extern void DrawObjectBounds(TArray<class PActor*>& InObjects);

	// Gizmo axis enum
	enum class EGizmoAxis : UINT8
	{
		NONE = 0,
		AXIS_X,
		AXIS_Y,
		AXIS_Z
	};

	// Draw XYZ translation arrows for a selected component
	extern void DrawTranslationGizmo(const class PCameraComponent* InCamera, const class PSceneComponent* InComponent);

	// Hit-test the gizmo arrows; returns which axis was clicked (NONE if miss)
	// OutDragOriginWorld is the world-space point on the axis ray at click time
	extern EGizmoAxis HitTestTranslationGizmo(
		const class PCameraComponent* InCamera,
		const class PSceneComponent* InComponent,
		const FLOAT InMouseX, const FLOAT InMouseY,
		const ERect& InScreenRect,
		Vector3& OutDragOriginWorld
	);

	// Project current mouse onto the drag axis and return the world-space delta
	extern Vector3 ComputeGizmoDragDelta(
		const class PCameraComponent* InCamera,
		const class PSceneComponent* InComponent,
		EGizmoAxis InAxis,
		const Vector3& InDragOriginWorld,
		const FLOAT InMouseX, const FLOAT InMouseY,
		const ERect& InScreenRect
	);

};

#endif