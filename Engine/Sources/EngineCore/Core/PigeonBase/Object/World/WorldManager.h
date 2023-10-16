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
#if _EDITOR_ONLY
    public:
        void EditorInit();
        void EditorUpdate();
#endif
    public:
        PE_NODISCARD PWorld* GetWorld()const;
    private:
        PWorld*  World;

    };
}

