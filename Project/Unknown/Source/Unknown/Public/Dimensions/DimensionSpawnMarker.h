#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DimensionSpawnMarker.generated.h"

/**
 * Editor-only placeholder actor to mark dimension spawn positions.
 * Designers place these in the main level to define where dimensions load.
 */
UCLASS(BlueprintType, Blueprintable, Placeable)
class UNKNOWN_API ADimensionSpawnMarker : public AActor
{
	GENERATED_BODY()

public:
	ADimensionSpawnMarker(const FObjectInitializer& ObjectInitializer);

	// Optional offset from marker location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn Marker")
	FVector SpawnOffset = FVector::ZeroVector;

	// Optional rotation for dimension
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Spawn Marker")
	FRotator SpawnRotation = FRotator::ZeroRotator;

	// Get the final spawn position (location + offset)
	UFUNCTION(BlueprintPure, Category="Spawn Marker")
	FVector GetSpawnPosition() const;

	// Get the spawn rotation
	UFUNCTION(BlueprintPure, Category="Spawn Marker")
	FRotator GetSpawnRotation() const { return SpawnRotation; }

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual bool ShouldTickIfViewportsOnly() const override { return true; }
	virtual void Tick(float DeltaSeconds) override;
#endif
};
