#pragma once
#include <CoreMinimal.h>

namespace PR_SlideHero
{
    struct EBuff;
    using namespace PigeonEngine;
    struct ESkillAcquirement
    {
        UINT8 Level;
        UINT8 Cost;
    };
    
    struct ESkill
    {
        UINT8 MaxTarget = 1;
        UINT8 Range = 0;
        TArray<EString> AddBuffIds;
    };
}