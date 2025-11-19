#include "Inventory/Effects/EquipEffect_Backpack.h"
#include "Inventory/InventoryComponent.h"

EEquipmentSlot UEquipEffect_Backpack::GetTargetSlot_Implementation() const
{
    return EEquipmentSlot::Back;
}

void UEquipEffect_Backpack::ApplyEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const
{
    if (!Inventory)
    {
        return;
    }
    const float Bonus = FMath::Max(0.f, VolumeBonus);
    Inventory->MaxVolume += Bonus;
}

bool UEquipEffect_Backpack::CanRemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory, FText& OutError) const
{
    if (!Inventory)
    {
        return true;
    }
    const float Bonus = FMath::Max(0.f, VolumeBonus);
    const float NewMax = FMath::Max(0.f, Inventory->MaxVolume - Bonus);
    const float Used = Inventory->GetUsedVolume();
    if (Used > NewMax + KINDA_SMALL_NUMBER)
    {
        OutError = FText::FromString(TEXT("Inventory over capacity; drop items first."));
        return false; // Block unequip (per requirements)
    }
    return true;
}

void UEquipEffect_Backpack::RemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const
{
    if (!Inventory)
    {
        return;
    }
    const float Bonus = FMath::Max(0.f, VolumeBonus);
    Inventory->MaxVolume = FMath::Max(0.f, Inventory->MaxVolume - Bonus);
}
