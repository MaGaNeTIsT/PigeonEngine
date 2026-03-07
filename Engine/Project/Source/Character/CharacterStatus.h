#pragma once

#include "Base/DataStructure/Container/Array.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PR_SlideHero
{
    struct ESkill;
    struct EBuff;
    using namespace PigeonEngine;
    struct ECharacterStatus
    {
        UINT32 CurrentHP;
        UINT32 MaxHP;
        TArray<TSharedPtr<EBuff>>  Buffs;
        TArray<TSharedPtr<ESkill>> Skills;
        // TArray<TSharedPtr<ESkill>> Skills;
    };
}
