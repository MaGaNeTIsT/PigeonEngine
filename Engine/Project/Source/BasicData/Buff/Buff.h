#pragma once
#include "../Property.h"
#include <CoreMinimal.h>
namespace PR_SlideHero
{
    struct EBuff
    {
        PigeonEngine::EString BuffId;
        PigeonEngine::EString BuffName;
        DOUBLE    TotalDuration   = 0.0;
        DOUBLE    DurationPerTick = 0.0; 
        DOUBLE    Interval        = 0.0;
        PigeonEngine::TArray<EProperty> Properties;
    };
    
}

