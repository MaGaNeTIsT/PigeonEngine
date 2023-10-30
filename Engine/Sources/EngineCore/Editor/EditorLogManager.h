#pragma once



#include <Base/DataStructure/ObjectBase.h>

#include "Base/DataStructure/Container/Array.h"
#define PE_LOG(Log)  EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(Log);
namespace PigeonEngine
{
    class EEditorLogManager : public EManagerBase
    {
        CLASS_MANAGER_VIRTUAL_SINGLETON_BODY(EEditorLogManager)
    public:
        void Initialize() override;
        void ShutDown() override;

        void Init();
        void Uninit();
#if _EDITOR_ONLY
    public:
        void EditorInit();
        void EditorUpdate();

        void AddALog(const EString& NewLog);
    private:
        TArray<EString> Logs;

#endif
    };
}

