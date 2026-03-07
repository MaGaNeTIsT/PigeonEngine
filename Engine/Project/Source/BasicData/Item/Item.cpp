#include "Item.h"
namespace PR_SlideHero
{
    EItem::EItem(const EString& InItemId, const EString& InItemName, const EString& InItemDescription, EItemType InType, const TArray<EProperty>& InProperties, UINT8 InMaxStackCount)
        :
    ItemId(InItemId), ItemName(InItemName), ItemDescription(InItemDescription), Type(InType), Properties(InProperties), MaxStackCount(InMaxStackCount)
    {    
    }

    PR_SlideHero::EEquipment::EEquipment(const EString& InItemId, const EString& InItemName, const EString& InItemDescription, const TArray<EProperty>& InProperties, EEquipmentType InEquipmentType)
            :
    EItem(InItemId, InItemName, InItemDescription, EIT_EQUIPMENT, InProperties, 1), EquipmentType(InEquipmentType)
    {
    }
}