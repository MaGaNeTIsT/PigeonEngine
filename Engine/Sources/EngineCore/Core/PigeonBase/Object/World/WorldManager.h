#pragma once
#include "Base/DataStructure/ObjectBase.h"

namespace PigeonEngine
{
    class PWorld;
    class EWorldManager final : public EManagerBase
    {
    public:
        void Initialize() override;
        void ShutDown() override;

        void Init();
        void Uninit();
#if _EDITOR_ONLY
    public:
        void EditorUpdate();
#endif
    public:
        PE_NODISCARD PWorld* GetWorld()const;
    private:
        PWorld*  World;

    public:
        static EWorldManager* GetManagerSingleton()
        {
            static EWorldManager _StaticManagerSingletonObject;
            return (&_StaticManagerSingletonObject);
        }
        CLASS_VIRTUAL_NOCOPY_BODY(EWorldManager)

    };
}

