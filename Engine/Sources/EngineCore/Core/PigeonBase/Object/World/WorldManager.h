#pragma once
#include "Base/DataStructure/ObjectBase.h"

namespace PigeonEngine
{
	class PWorld;
	class EWorldManager final : public EManagerBase
	{
		CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EWorldManager)
	public:
		void Initialize() override;
		void ShutDown() override;

		void Init();
		void Uninit();
#if ENGINE_EDITOR_ONLY
	public:
		void EditorInit();
		void EditorUpdate();

#endif
	public:
		PE_NODISCARD PWorld* GetWorld() const;

		// Level IO requests. The editor menu queues them; EMainManager::Update runs them after
		// FlushPendingDestroy, so a load never destroys actors in the middle of a frame's
		// callbacks or while the editor panels still hold this frame's selection.
		void RequestNewLevel();
		void RequestSaveLevel(const EString& InPath);
		void RequestLoadLevel(const EString& InPath);
		void RequestReloadLevel();
		void RequestOpenLevelPopup();
		void RequestSaveAsPopup();
		PE_NODISCARD const EString& GetCurrentLevelPath() const;
		// Bumped whenever a level is created or loaded. The editor uses it to drop undo
		// commands that point into the previous level's objects.
		PE_NODISCARD UINT64 GetLevelRevision() const;
		PE_NODISCARD BOOL32 IsLevelDirty() const;
		void MarkLevelDirty();
		// Synchronous save used by the unsaved-changes prompt before it continues a New/Open.
		BOOL32 SaveLevelNow(const EString& InPath);
		void ProcessPendingLevelRequest();
	private:
		enum class ELevelRequestType : UINT8
		{
			NONE,
			NEW,
			SAVE,
			LOAD,
		};
		ELevelRequestType	PendingLevelRequest = ELevelRequestType::NONE;
		EString				PendingLevelPath;
		// Last path that was loaded or saved; empty for a new, never-saved level.
		EString				CurrentLevelPath;
		UINT64				LevelRevision = 0u;
		BOOL32				bLevelDirty = FALSE;
#if ENGINE_EDITOR_ONLY
	private:
		void ExecutePendingDiscardAction();
	public:
		// Unsaved-changes prompt answers, driven by the Level panel: the pending New/Load
		// request that raised the prompt executes after the save, or directly, or drops.
		BOOL32 ConfirmUnsavedPromptWithSave(const EString& InSavePath);
		void ConfirmUnsavedPromptWithDiscard();
		void CancelPendingDiscardAction();
		// State fixups from the Level panel's file operations: the current path follows a
		// renamed level file, and deleting the current file queues a new empty level.
		void NotifyLevelFileRenamed(const EString& InOldPath, const EString& InNewPath);
		void NotifyLevelFileDeleted(const EString& InPath);
	private:
		// A New/Load request that is waiting for the unsaved-changes prompt.
		ELevelRequestType	PendingDiscardRequest = ELevelRequestType::NONE;
		EString				PendingDiscardPath;
#endif
	private:
		PWorld*  World;

	};
}

