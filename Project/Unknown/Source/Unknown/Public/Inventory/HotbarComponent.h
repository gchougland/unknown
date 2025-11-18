#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ItemTypes.h"
#include "HotbarComponent.generated.h"

class UInventoryComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct FHotbarSlot
{
	GENERATED_BODY()

	// Assigned item type for this slot (may be null if unassigned)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Hotbar")
	TObjectPtr<UItemDefinition> AssignedType = nullptr;

	// Currently held unit id for this slot (valid only when selected and available)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hotbar")
	FGuid ActiveItemId;

	bool HasActive() const { return ActiveItemId.IsValid(); }
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHotbarSlotAssigned, int32, Index, UItemDefinition*, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHotbarActiveChanged, int32, NewIndex, FGuid, ItemId);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UHotbarComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UHotbarComponent();

	UFUNCTION(BlueprintPure, Category="Hotbar")
	int32 GetNumSlots() const { return Slots.Num(); }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	int32 GetActiveIndex() const { return ActiveIndex; }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	FGuid GetActiveItemId() const { return ActiveItemId; }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	const FHotbarSlot& GetSlot(int32 Index) const;

	UFUNCTION(BlueprintCallable, Category="Hotbar")
	bool AssignSlot(int32 Index, UItemDefinition* Type);

	UFUNCTION(BlueprintCallable, Category="Hotbar")
	bool ClearSlot(int32 Index);

	// Selects a slot and attempts to hold a unit from the provided inventory that matches AssignedType
	UFUNCTION(BlueprintCallable, Category="Hotbar")
	bool SelectSlot(int32 Index, UInventoryComponent* Inventory);

	// Re-evaluates the currently selected slot against inventory to ensure ActiveItemId points to an existing unit
	UFUNCTION(BlueprintCallable, Category="Hotbar")
	void RefreshHeldFromInventory(UInventoryComponent* Inventory);

	// Events
	UPROPERTY(BlueprintAssignable, Category="Hotbar|Events")
	FOnHotbarSlotAssigned OnSlotAssigned;

	UPROPERTY(BlueprintAssignable, Category="Hotbar|Events")
	FOnHotbarActiveChanged OnActiveChanged;

protected:
	// 9 slots by default
	UPROPERTY(EditAnywhere, Category="Hotbar")
	TArray<FHotbarSlot> Slots;

	// Currently active slot index or INDEX_NONE
	UPROPERTY(VisibleAnywhere, Category="Hotbar")
	int32 ActiveIndex = INDEX_NONE;

	// Cached currently held item id (redundant to Slots[ActiveIndex].ActiveItemId but useful for quick queries)
	UPROPERTY(VisibleAnywhere, Category="Hotbar")
	FGuid ActiveItemId;

	// Helper to pick deterministic unit id from inventory for a given type
	FGuid PickFirstItemIdOfType(const UInventoryComponent* Inventory, const UItemDefinition* Type) const;
};
