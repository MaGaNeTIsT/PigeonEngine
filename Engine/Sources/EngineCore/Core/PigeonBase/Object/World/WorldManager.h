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
	private:
		PWorld*  World;

	};
}

