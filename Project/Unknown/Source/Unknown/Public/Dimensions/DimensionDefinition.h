#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Dimensions/DimensionTypes.h"
#include "DimensionDefinition.generated.h"

class UWorld;

/**
 * Immutable metadata for a dimension type.
 * Defines the properties of a dimension that can be scanned and loaded.
 */
UCLASS(BlueprintType)
class UNKNOWN_API UDimensionDefinition : public UDataAsset
{
	GENERATED_BODY()

public:
	UDimensionDefinition();

	// Stable Guid for this definition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	FGuid Guid;

	// Display name for UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	FText DisplayName;

	// Description of the dimension
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	FText Description;

	// Reference to the level asset to load for this dimension
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	TSoftObjectPtr<UWorld> DimensionLevel;

	// Type of dimension (affects rarity and behavior)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	EDimensionType Type = EDimensionType::Standard;

	// Dimension size (constant, immutable)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension", meta=(ClampMin="0.0"))
	float Size = 1000.0f;

	// Default stability value when dimension is first scanned
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension", meta=(ClampMin="0.0", ClampMax="100.0"))
	float DefaultStability = 100.0f;

	// Weight for random selection (higher = more likely to be scanned)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension|Scanning", meta=(ClampMin="0.0"))
	float RarityWeight = 1.0f;

	// Player requirements that must be met to scan this dimension
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension|Scanning")
	FGameplayTagContainer RequiredTags;

	// Tags that describe this dimension's characteristics
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Dimension")
	FGameplayTagContainer DimensionTags;
};
