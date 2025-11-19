#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Inventory/EquipmentTypes.h"
#include "ItemEquipEffect.generated.h"

class UInventoryComponent;

/**
 * Base class for pluggable equipment effects.
 * Effects are referenced by item definitions and invoked by the equipment system when items are equipped/unequipped.
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UItemEquipEffect : public UObject
{
    GENERATED_BODY()
public:
    /** The target slot this effect is intended to occupy (used to route items without storing slot on the definition). */
    UFUNCTION(BlueprintNativeEvent, Category="Equipment")
    EEquipmentSlot GetTargetSlot() const;

    /** Called when the item is equipped; may mutate the owning inventory/component state. */
    UFUNCTION(BlueprintNativeEvent, Category="Equipment")
    void ApplyEffect(UObject* WorldContextObject, UInventoryComponent* Inventory) const;

    /**
     * Called before removing the effect; return false to block unequip (and optionally provide a message).
     * If allowed, RemoveEffect will be called next.
     */
    UFUNCTION(BlueprintNativeEvent, Category="Equipment")
    bool CanRemoveEffect(UObject* WorldContextObject, UInventoryComponent* Inventory, FText& OutError) const;

    /** Called after CanRemoveEffect approves removal; must revert any ApplyEffect changes. */
    UFUNCTION(BlueprintNativeEvent, Category="Equipment")
    void RemoveEffect(UObject* WorldContextObject, UInventoryComponent* Inventory) const;
};
