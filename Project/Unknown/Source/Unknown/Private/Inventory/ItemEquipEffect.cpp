#include "Inventory/ItemEquipEffect.h"

// Provide default no-op implementations for BlueprintNativeEvents

EEquipmentSlot UItemEquipEffect::GetTargetSlot_Implementation() const
{
    return EEquipmentSlot::Back; // Sensible default; concrete effects should override
}

void UItemEquipEffect::ApplyEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const
{
    // No-op by default
}

bool UItemEquipEffect::CanRemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory, FText& OutError) const
{
    return true; // Allow by default
}

void UItemEquipEffect::RemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const
{
    // No-op by default
}
