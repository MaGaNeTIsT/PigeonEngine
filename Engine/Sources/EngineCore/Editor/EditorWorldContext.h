#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY

namespace PigeonEngine
{
	class PActor;
	class PActorComponent;
	class PWorld;
	class FWorldEditorCommandStack;

	/*
	* Per-world editor state that used to live on PWorld: the selection and the command
	* stack the panels record their mutations into. The selection is stored as object
	* handles and resolved through the world registry on every read, so a destroyed or
	* removed object reads as null immediately - no runtime lifecycle hooks are needed to
	* keep it coherent, and disposable worlds (sanity scenarios, the profiler benchmark)
	* work without installation. Contexts are erased when their world is deleted.
	*/
	namespace EEditorWorldContext
	{
		PE_NODISCARD PActor* GetSelectedActor(PWorld* InWorld);
		PE_NODISCARD PActorComponent* GetSelectedComponent(PWorld* InWorld);
		// Selecting an actor also picks the default component selection (the root, or the
		// root's single child, so properties like materials are visible without a click).
		void SetSelectedActor(PWorld* InWorld, PActor* InActor);
		void SetSelectedComponent(PWorld* InWorld, PActorComponent* InComponent);
		void ClearSelection(PWorld* InWorld);

		// The editor installs its undo stack per world; null in a world that is not edited.
		void SetEditorCommandStack(PWorld* InWorld, FWorldEditorCommandStack* InCommands);
		PE_NODISCARD FWorldEditorCommandStack* GetEditorCommandStack(PWorld* InWorld);
	}
}

#endif
