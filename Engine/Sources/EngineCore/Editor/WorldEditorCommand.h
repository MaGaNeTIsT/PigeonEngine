#pragma once

#include <CoreMinimal.h>

#if ENGINE_EDITOR_ONLY
#include <Base/DataStructure/ObjectBase.h>
#include <Reflection/Property.h>

namespace PigeonEngine
{
	class PActor;
	class PActorComponent;
	class PSceneComponent;
	class PWorld;

	/*
	* Shared state for one command stack: the world commands act on and the handle remap
	* table. Structural commands recreate objects with new unique ids, so a command that
	* stores an old handle records old->new here and every later resolve follows the chain.
	*/
	class FWorldEditorCommandContext
	{
	public:
		PWorld* World = nullptr;
		TMap<ObjectIdentityType, ObjectIdentityType> HandleRemap;

		PE_NODISCARD FObjectHandle Resolve(const FObjectHandle& InHandle) const;
		void AddRemap(const FObjectHandle& InOldHandle, const FObjectHandle& InNewHandle);
		void ClearRemap();
	};

	class IWorldEditorCommand
	{
	public:
		virtual ~IWorldEditorCommand() = default;
		virtual BOOL32 Undo(FWorldEditorCommandContext& InContext) = 0;
		virtual BOOL32 Redo(FWorldEditorCommandContext& InContext) = 0;
		PE_NODISCARD virtual EString GetLabel() const = 0;
	};

	class FWorldEditorCommandStack
	{
	public:
		FWorldEditorCommandStack() = default;
		~FWorldEditorCommandStack();

		void Clear();
		// Pushes a command whose effect is already applied (the UI performed it, then
		// recorded how to undo/redo it). Clears the redo stack.
		void PushExecuted(IWorldEditorCommand* InCommand);

		PE_NODISCARD BOOL32 CanUndo() const;
		PE_NODISCARD BOOL32 CanRedo() const;
		PE_NODISCARD EString GetUndoLabel() const;
		PE_NODISCARD EString GetRedoLabel() const;
		BOOL32 Undo();
		BOOL32 Redo();

		PE_NODISCARD FWorldEditorCommandContext& GetContext();
		// Increments whenever an edit enters or leaves the history; the editor uses it to
		// mark the level dirty without subscribing to every command type.
		PE_NODISCARD UINT64 GetHistoryRevision() const;
	private:
		TArray<IWorldEditorCommand*> UndoStack;
		TArray<IWorldEditorCommand*> RedoStack;
		FWorldEditorCommandContext Context;
		UINT32 MaxHistory = 100u;
		UINT64 HistoryRevision = 0u;
	};

	// UI-facing helpers. Each records the command and performs the mutation that the command
	// describes, so the editor menu and the outline stay on the same undo path.
	BOOL32 PushSpawnActorCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor);
	BOOL32 PushDeleteActorCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor);
	BOOL32 PushAddComponentCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor, PActorComponent* InComponent);
	BOOL32 PushDeleteComponentCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActorComponent* InComponent);
	BOOL32 PushSetPropertyCommand(FWorldEditorCommandStack& InStack, const FObjectHandle& InObjectHandle, const BOOL32& bInIsComponent,
		const size_t& InClassHash, const EString& InPropertyName, const FPropertyValue& InBefore, const FPropertyValue& InAfter);
	// Re-homes an actor under InNewParentActor (the world root detaches it back to the top
	// level) or a scene component under InNewParentComponent (same actor only; the level
	// format stores component parents per actor). Both apply the attach with KEEP_WORLD and
	// record the undo/redo through the command stack; refused drops return FALSE without
	// touching the history.
	BOOL32 PushReparentActorCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PActor* InActor, PActor* InNewParentActor);
	BOOL32 PushReparentComponentCommand(FWorldEditorCommandStack& InStack, PWorld& InWorld, PSceneComponent* InComponent, PSceneComponent* InNewParentComponent);
}
#endif
