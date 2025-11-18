#pragma once

#include "Kismet/BlueprintFunctionLibrary.h"
#include "ItemContainerLibrary.generated.h"

class UInventoryComponent;
class UStorageComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct FTransferResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	bool bSuccess = false;

	UPROPERTY(BlueprintReadOnly, Category="Inventory")
	int32 MovedCount = 0;
};

/**
 * Helper library for moving items between containers with shared validation.
 */
UCLASS()
class UNKNOWN_API UItemContainerLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	// Compute free volume for a container
	UFUNCTION(BlueprintPure, Category="Inventory|Helpers")
	static float GetFreeVolume(const UInventoryComponent* Container);

	UFUNCTION(BlueprintPure, Category="Inventory|Helpers")
	static float GetFreeVolumeStorage(const UStorageComponent* Container);

	// Inventory -> Storage by ItemId
	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static bool CanTransfer_ItemId(const UInventoryComponent* Source, const UStorageComponent* Dest, const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static bool Transfer_ItemId(UInventoryComponent* Source, UStorageComponent* Dest, const FGuid& ItemId);

	// Storage -> Inventory by ItemId
	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static bool CanTransfer_ItemId_StorageToInv(const UStorageComponent* Source, const UInventoryComponent* Dest, const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static bool Transfer_ItemId_StorageToInv(UStorageComponent* Source, UInventoryComponent* Dest, const FGuid& ItemId);

	// Move all units of a type from Inv to Storage (subject to capacity)
	UFUNCTION(BlueprintCallable, Category="Inventory|Helpers")
	static FTransferResult MoveAllOfType(UInventoryComponent* Source, UStorageComponent* Dest, const UItemDefinition* Def);
};
