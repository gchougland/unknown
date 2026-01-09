#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/EquipmentTypes.h"
#include "EquipmentComponent.generated.h"

class UInventoryComponent;
class UItemDefinition;

/** Broadcast when an item is equipped into a slot */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemEquipped, EEquipmentSlot, Slot, const FItemEntry&, Item);
/** Broadcast when an item is unequipped from a slot */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemUnequipped, EEquipmentSlot, Slot, const FItemEntry&, Item);

/** Manages equipping/unequipping items and applying pluggable equip effects. */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UEquipmentComponent : public UActorComponent
{
    GENERATED_BODY()
public:
    UEquipmentComponent();

    /** Reference to the owning player's inventory. Not owned; must be set by the character after components are constructed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Equipment")
    TObjectPtr<UInventoryComponent> Inventory;

	/** Get the currently equipped entry for a slot, if any. */
	UFUNCTION(BlueprintPure, Category="Equipment")
	bool GetEquipped(EEquipmentSlot Slot, FItemEntry& OutEntry) const;

	/** Get all equipped items as a map of slot to item entry. Useful for save system. */
	UFUNCTION(BlueprintCallable, Category="Equipment")
	TMap<EEquipmentSlot, FItemEntry> GetAllEquippedItems() const;

    /** Equip an item instance from the inventory (by ItemId). Replaces existing item in the slot; returns/re-drops that item if needed. */
    UFUNCTION(BlueprintCallable, Category="Equipment")
    bool EquipFromInventory(const FGuid& ItemId, FText& OutError);

    /** Unequip the current item in a slot back into inventory; may be blocked by effect rules (e.g., backpack over-capacity). */
    UFUNCTION(BlueprintCallable, Category="Equipment")
    bool TryUnequipToInventory(EEquipmentSlot Slot, FText& OutError);

    /** Events */
    UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
    FOnItemEquipped OnItemEquipped;

    UPROPERTY(BlueprintAssignable, Category="Equipment|Events")
    FOnItemUnequipped OnItemUnequipped;

    /** Persistence hooks (no-op for now) */
    UFUNCTION(BlueprintCallable, Category="Equipment|Save")
    void WriteToSave(/*out*/ TArray<uint8>& OutData) const {}
    UFUNCTION(BlueprintCallable, Category="Equipment|Save")
    void ReadFromSave(const TArray<uint8>& InData) {}

private:
    // Internal storage; not exposed to reflection to avoid UENUM requirements on EEquipmentSlot
    TMap<EEquipmentSlot, FItemEntry> Equipped;

    /** Find the first entry in Inventory that matches ItemId; returns index or INDEX_NONE. */
    int32 FindEntryIndexById(const FGuid& ItemId) const;

    /** Apply all equip effects for the given definition. */
    void ApplyEffects(const UItemDefinition* Def) const;

    /** Query CanRemove on all effects; if any blocks, return false and set OutError. */
    bool CanRemoveEffects(const UItemDefinition* Def, /*out*/ FText& OutError) const;

    /** Remove all effects for the given definition. */
    void RemoveEffects(const UItemDefinition* Def) const;

    /** Determine the target slot for an item definition by consulting its first equip effect's GetTargetSlot. */
    bool ResolveTargetSlot(const UItemDefinition* Def, /*out*/ EEquipmentSlot& OutSlot) const;

    /** Spawn a world pickup for an item (used when returning an equipped item cannot fit in inventory). */
    void DropToWorld(const FItemEntry& Entry) const;
};
