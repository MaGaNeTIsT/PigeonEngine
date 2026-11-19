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
#if ENGINE_EDITOR_ONLY
	private:
		void RefreshLevelList();
		void DrawLevelPanelContents();
	private:
		TArray<EString>		LevelFiles;
		BOOL32				bLevelListDirty = TRUE;
		BOOL32				bOpenLevelPopupRequested = FALSE;
		BOOL32				bSaveAsPopupRequested = FALSE;
		CHAR				SaveAsNameBuffer[256] = {};
#endif
	private:
		PWorld*  World;

	};
}

