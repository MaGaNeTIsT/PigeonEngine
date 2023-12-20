#pragma once
#include <CoreMinimal.h>

namespace PR_SlideHero
{
    using namespace PigeonEngine;
    enum PropertyType : UINT8
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

    enum PropertyValueType : UINT8
    {
        PVT_NUMBER,
        PVT_PERCENT
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
        EProperty(){}
        PropertyType      PropertyType;
        PropertyValueType ValueType;
        DOUBLE            Value;
    };

    struct EResistance : EProperty
    {
        ResistanceType ResistanceType = RT_COUNT;
    };
    
    
}



