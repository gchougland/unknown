#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "DimensionInstanceData.generated.h"

/**
 * Runtime modifiable data for a dimension instance.
 * Each dimension instance has its own instance data that can be modified by the player.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UDimensionInstanceData : public UObject
{
	GENERATED_BODY()

public:
	// Current stability of the dimension (player can modify)
	// Affects how long the player can stay in the dimension
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Dimension Instance", meta=(ClampMin="0.0", ClampMax="100.0"))
	float Stability = 100.0f;

	// Unique instance identifier for this dimension instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dimension Instance")
	FGuid InstanceId;

	// Associated cartridge ID that owns this instance
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dimension Instance")
	FGuid CartridgeId;

	// Position in the open world where this dimension is loaded
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dimension Instance")
	FVector WorldPosition = FVector::ZeroVector;

	// Whether this dimension instance is currently loaded
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Dimension Instance")
	bool bIsLoaded = false;

	// Initialize the instance data with a new instance ID
	UFUNCTION(BlueprintCallable, Category="Dimension Instance")
	void InitializeInstance(const FGuid& InCartridgeId, const FVector& InWorldPosition, float InStability);

	// Get the instance ID
	UFUNCTION(BlueprintPure, Category="Dimension Instance")
	FGuid GetInstanceId() const { return InstanceId; }

	// Get the cartridge ID
	UFUNCTION(BlueprintPure, Category="Dimension Instance")
	FGuid GetCartridgeId() const { return CartridgeId; }
};
