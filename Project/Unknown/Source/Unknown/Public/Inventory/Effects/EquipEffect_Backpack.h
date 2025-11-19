#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemEquipEffect.h"
#include "EquipEffect_Backpack.generated.h"

class UInventoryComponent;

/** Backpack effect: increases inventory MaxVolume while equipped. */
UCLASS(BlueprintType, EditInlineNew)
class UNKNOWN_API UEquipEffect_Backpack : public UItemEquipEffect
{
    GENERATED_BODY()
public:
    // Additional capacity granted while equipped
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Backpack")
    float VolumeBonus = 20.f;

    virtual EEquipmentSlot GetTargetSlot_Implementation() const override;
    virtual void ApplyEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const override;
    virtual bool CanRemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory, FText& OutError) const override;
    virtual void RemoveEffect_Implementation(UObject* WorldContextObject, UInventoryComponent* Inventory) const override;
};
