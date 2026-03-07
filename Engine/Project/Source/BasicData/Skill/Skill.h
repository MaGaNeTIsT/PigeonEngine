#pragma once
#include <CoreMinimal.h>

namespace PR_SlideHero
{
    using namespace PigeonEngine;
    struct EBuff;
    struct ESkillAcquirement
    {
        UINT8 Level;
        UINT8 Cost;
    };
    
    struct ESkill
    {
        EString  SkillId;
        EString  SkillName;
        EString  SkillDescription;
        ESkillAcquirement Acquirement;
        UINT8 MaxTarget = 1;
        UINT8 Range = 0;
        TArray<EString> AddBuffIds;
    };
}