#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Inventory/ItemTypes.h"
#include "StorageComponent.generated.h"

class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageItemAdded, const FItemEntry&, Item);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageItemRemoved, const FGuid&, ItemId);

/**
 * Simple volume-based storage component for world containers (chests, lockers, etc.).
 * Mirrors UInventoryComponent API to enable shared helper logic and UI reuse.
 */
UCLASS(ClassGroup=(Inventory), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UStorageComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UStorageComponent();

	// Maximum volume capacity for this storage container
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Storage")
	float MaxVolume = 60.f;

	// Current entries (no stacks)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Storage")
	TArray<FItemEntry> Entries;

	UFUNCTION(BlueprintCallable, Category="Storage")
	float GetUsedVolume() const;

	UFUNCTION(BlueprintCallable, Category="Storage")
	bool CanAdd(const FItemEntry& Entry) const;

	UFUNCTION(BlueprintCallable, Category="Storage")
	bool TryAdd(const FItemEntry& Entry);

	UFUNCTION(BlueprintCallable, Category="Storage")
	bool RemoveById(const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category="Storage")
	int32 CountByDef(const UItemDefinition* Def) const;

	UFUNCTION(BlueprintPure, Category="Storage")
	const TArray<FItemEntry>& GetEntries() const { return Entries; }

	// Events
	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageItemAdded OnItemAdded;

	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageItemRemoved OnItemRemoved;
};
