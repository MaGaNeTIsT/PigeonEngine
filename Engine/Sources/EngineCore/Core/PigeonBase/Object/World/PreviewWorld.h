#pragma once

#if _EDITOR_ONLY

#include "World.h"

namespace PigeonEngine
{
    class PPreviewWorld final : public PWorld
    {
    public:
        CLASS_VIRTUAL_NOCOPY_BODY(PPreviewWorld)
        void Init() override;
    protected:
        PActor* CreateRootActor() override;
        void CreateDefaultController() override;
        BOOL32 ShouldAffectGlobalPhysics() const override;
    };
}

#endif