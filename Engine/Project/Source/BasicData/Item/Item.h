#pragma once
#include "../Property.h"
#include "Base/DataStructure/Pointer/SharedPtr.h"

namespace PR_SlideHero
{
    using namespace PigeonEngine;
    enum EItemType : UINT8
    {
        EIT_POTION,
        EIT_CONSUMEBLE,
        EIT_EQUIPMENT,
        EIT_QUEST,
        EIT_MISC,
    };
    
    enum EEquipmentType : UINT8
    {
        EET_WEAPON,
        EET_ARMOR,
        EET_ACCESSORIES,
    };
    
    struct EItem 
    {
        EItem(const EString& InItemId, const EString& InItemName, const EString& InItemDescription, EItemType InType, const TArray<EProperty>& InProperties, UINT8 InMaxStackCount);
        EString            ItemId;
        EString            ItemName;
        EString            ItemDescription;
        EItemType          Type;
        TArray<EProperty>  Properties;
        UINT8              MaxStackCount;
    };
    
    struct EEquipment : EItem
    {
        EEquipment(const EString& InItemId, const EString& InItemName, const EString& InItemDescription, const TArray<EProperty>& InProperties, EEquipmentType InEquipmentType);
        EEquipmentType EquipmentType;
    };

    
    struct EPotion : EItem
    {
        
    };
}
