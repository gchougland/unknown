#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dimensions/DimensionDefinition.h"
#include "DimensionScanningSubsystem.generated.h"

class UDimensionDefinition;

/**
 * Subsystem for managing dimension scanning logic.
 * Handles dimension discovery, requirement checking, and weighted random selection.
 */
UCLASS()
class UNKNOWN_API UDimensionScanningSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Scan for a dimension based on rarity weights and player requirements
	UFUNCTION(BlueprintCallable, Category="Dimension Scanning")
	UDimensionDefinition* ScanForDimension();

	// Get all available dimension definitions
	UFUNCTION(BlueprintCallable, Category="Dimension Scanning")
	TArray<UDimensionDefinition*> GetAvailableDimensions() const;

	// Get dimensions that the player is eligible to scan (meets requirements)
	UFUNCTION(BlueprintCallable, Category="Dimension Scanning")
	TArray<UDimensionDefinition*> GetEligibleDimensions() const;

	// Check if player meets requirements for a dimension
	UFUNCTION(BlueprintCallable, Category="Dimension Scanning")
	bool CheckPlayerRequirements(UDimensionDefinition* DimensionDef) const;

	// Load all dimension definitions from data assets
	UFUNCTION(BlueprintCallable, Category="Dimension Scanning")
	void LoadDimensionDefinitions();

private:
	// All available dimension definitions
	UPROPERTY()
	TArray<TObjectPtr<UDimensionDefinition>> AvailableDimensions;

	// Perform weighted random selection from eligible dimensions
	UDimensionDefinition* SelectDimensionByWeight(const TArray<UDimensionDefinition*>& EligibleDimensions) const;
};
