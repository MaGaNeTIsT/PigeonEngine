#pragma once
#include <Base/DataStructure/ObjectBase.h>
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"
#include "Base/Timer/Timer.h"
#if _EDITOR_ONLY
#define PE_LOG_LOG(Log)   EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_LOG, Log);
#define PE_LOG_WARN(Log)  EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_WARNING, Log);
#define PE_LOG_ERROR(Log) EEditorLogManager* LogsManager = EEditorLogManager::GetManagerSingleton();LogsManager->AddALog(ELogType::ELT_ERROR, Log);
#else
#define PE_LOG_LOG(Log)  
#define PE_LOG_WARN(Log)
#define PE_LOG_ERROR(Log) 
#endif
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
        ELog() = delete;
        ELog(ELogType InType, const EString& InLog, const EngineSystemTime::ESystemTime& InTimeStamp);

        void PrintLog() const;
        PE_NODISCARD EString AsString() const;
        ELogType Type;
        EString Log;
        EngineSystemTime::ESystemTime TimeStamp;
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
        void WriteDownLogs();
    private:
        TArray<TSharedPtr<ELog>> Logs;

#endif
    };
}

