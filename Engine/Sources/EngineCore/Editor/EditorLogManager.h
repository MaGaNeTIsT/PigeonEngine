#pragma once



#include <Base/DataStructure/ObjectBase.h>
#include "Base/DataStructure/Container/Array.h"

#define PE_LOG_LOG(Log)   EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_LOG, Log);
#define PE_LOG_WARN(Log)  EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_WARNING, Log);
#define PE_LOG_ERROR(Log) EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_ERROR, Log);

namespace PigeonEngine
{
    enum ELogType : UINT8
    {
        ELT_LOG,
        ELT_WARNING,
        ELT_ERROR,
    };
    struct ELog
    {
        ELog(ELogType InType, const EString& InLog);

        void PrintLog() const;
        ELogType Type;
        EString Log;
    };
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

        void AddALog(ELogType Type, const EString& NewLog);
    private:
        TArray<ELog> Logs;

#endif
    };
}

