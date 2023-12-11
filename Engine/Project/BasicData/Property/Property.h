#pragma once
#include <CoreMinimal.h>

namespace PR_SlideHero
{
    enum PropertyType
    {
        PT_HP,    // Health
        PT_MS,    // Moving Speed
        PT_AP,    // Attack Power
        PT_AS,    // Action Speed
        PT_CR,    // Critical Rate
        PT_CD,    // Critical Damage
        PT_DEF,   // Defence
        PT_RES,   // Resistance
        PT_COUNT, // End of this enum
    };
    
    // to be finished
    enum ResistanceType
    {
        RT_PHYSICS,
        RT_FIRE,
        RT_COUNT,
    };

    struct EProperty
    {
        PropertyType PropertyType = PT_COUNT;
        DOUBLE       Value        = 0.0;
    };

    struct EResistance : EProperty
    {
        ResistanceType ResistanceType = RT_COUNT;
    };

    
}



