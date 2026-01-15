#pragma once

#include "CoreMinimal.h"
#include "Dimensions/DimensionCartridgeData.h"
#include "Inventory/ItemTypes.h"
#include "DimensionCartridgeHelpers.generated.h"

class AItemPickup;
class UDimensionCartridgeData;

/**
 * Helper functions for working with dimension cartridges in items.
 * These functions handle serialization/deserialization of cartridge data from item CustomData.
 */
UCLASS()
class UNKNOWN_API UDimensionCartridgeHelpers : public UObject
{
	GENERATED_BODY()

public:
	// Get dimension cartridge data from an item entry
	UFUNCTION(BlueprintCallable, Category="Dimension Cartridge")
	static UDimensionCartridgeData* GetCartridgeDataFromItemEntry(const FItemEntry& ItemEntry);

	// Get dimension cartridge data from an item pickup actor
	UFUNCTION(BlueprintCallable, Category="Dimension Cartridge")
	static UDimensionCartridgeData* GetCartridgeDataFromItem(AItemPickup* Item);

	// Set dimension cartridge data in an item entry
	UFUNCTION(BlueprintCallable, Category="Dimension Cartridge")
	static void SetCartridgeDataInItemEntry(FItemEntry& ItemEntry, UDimensionCartridgeData* CartridgeData);

	// Check if an item entry contains dimension cartridge data
	UFUNCTION(BlueprintPure, Category="Dimension Cartridge")
	static bool HasCartridgeData(const FItemEntry& ItemEntry);

	// Create a new dimension cartridge from a dimension definition (Blueprint-friendly)
	// This creates a new cartridge with a new instance ID and sets it up ready to use
	UFUNCTION(BlueprintCallable, Category="Dimension Cartridge", meta=(CallInEditor=true))
	static UDimensionCartridgeData* CreateCartridgeFromDimension(UDimensionDefinition* DimensionDef, const FVector& SpawnPosition = FVector::ZeroVector);

	// Create an ItemEntry with dimension cartridge data already set (most convenient for Blueprints)
	// This is a one-step function that creates the cartridge and sets it in the ItemEntry
	UFUNCTION(BlueprintCallable, Category="Dimension Cartridge", meta=(CallInEditor=true))
	static FItemEntry CreateItemEntryWithCartridge(UItemDefinition* CartridgeItemDef, UDimensionDefinition* DimensionDef, const FVector& SpawnPosition = FVector::ZeroVector);

private:
	// Key used in CustomData to store cartridge data
	static const FName CartridgeDataKey;
};
