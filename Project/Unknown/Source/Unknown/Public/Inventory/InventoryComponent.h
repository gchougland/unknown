#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ItemTypes.h"
#include "InventoryComponent.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemAdded, const FItemEntry&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryItemRemoved, const FGuid&, ItemId);

UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UInventoryComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UInventoryComponent();

	// Maximum volume capacity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Inventory")
	float MaxVolume = 30.f;

	// Current entries (no stacks)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory")
	TArray<FItemEntry> Entries;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	float GetUsedVolume() const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool CanAdd(const FItemEntry& Entry) const;

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool TryAdd(const FItemEntry& Entry);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool RemoveById(const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	int32 CountByDef(const UItemDefinition* Def) const;

	UFUNCTION(BlueprintPure, Category="Inventory")
	const TArray<FItemEntry>& GetEntries() const { return Entries; }

	// Events
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryItemRemoved OnItemRemoved;
};
