#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Dimensions/DimensionDefinition.h"
#include "Dimensions/DimensionInstanceData.h"
#include "DimensionCartridgeData.generated.h"

/**
 * Data container for dimension cartridges.
 * Stores the dimension definition and instance data for a cartridge.
 * Similar to UFoodItemData pattern.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UDimensionCartridgeData : public UObject
{
	GENERATED_BODY()

public:
	// Reference to the dimension definition (immutable metadata)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cartridge")
	TObjectPtr<UDimensionDefinition> DimensionDef = nullptr;

	// Runtime instance data (modifiable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Cartridge", Instanced)
	TObjectPtr<UDimensionInstanceData> InstanceData = nullptr;

	// Unique cartridge identifier
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Cartridge")
	FGuid CartridgeId;

	// Initialize the cartridge with a dimension definition
	UFUNCTION(BlueprintCallable, Category="Cartridge")
	void InitializeCartridge(UDimensionDefinition* InDimensionDef, const FVector& SpawnPosition);

	// Get the dimension definition
	UFUNCTION(BlueprintPure, Category="Cartridge")
	UDimensionDefinition* GetDimensionDefinition() const { return DimensionDef; }

	// Get the instance data
	UFUNCTION(BlueprintPure, Category="Cartridge")
	UDimensionInstanceData* GetInstanceData() const { return InstanceData; }

	// Get the cartridge ID
	UFUNCTION(BlueprintPure, Category="Cartridge")
	FGuid GetCartridgeId() const { return CartridgeId; }

	// Check if the cartridge is valid
	UFUNCTION(BlueprintPure, Category="Cartridge")
	bool IsValid() const { return DimensionDef != nullptr && InstanceData != nullptr; }
};
