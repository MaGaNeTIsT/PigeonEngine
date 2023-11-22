#pragma once

#include <Base/DataStructure/ObjectBase.h>

#if _EDITOR_ONLY
#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"
#include "Base/Timer/Timer.h"

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

    public:
        void EditorInit();
        void EditorUpdate();
        
        void AddALog(ELogType Type, const EString& NewLog);
        void WriteDownLogs();
    private:
        TArray<TSharedPtr<ELog>> Logs;


    };
#define PE_LOG_LOG(__Log)       { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_LOG, (__Log)); }
#define PE_LOG_WARN(__Log)      { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_WARNING, (__Log)); }
#define PE_LOG_ERROR(__Log)     { EEditorLogManager::GetManagerSingleton()->AddALog(ELogType::ELT_ERROR, (__Log)); }

}

#endif


