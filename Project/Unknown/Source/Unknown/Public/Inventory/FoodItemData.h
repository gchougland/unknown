#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "FoodItemData.generated.h"

class UStaticMesh;
class UItemDefinition;

/**
 * Food item data container, similar to UItemEquipEffect pattern.
 * Contains properties specific to food items: hunger restoration, uses, mesh variants, and replacement items.
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UFoodItemData : public UObject
{
	GENERATED_BODY()

public:
	// Amount of hunger restored per use
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Food", meta=(ClampMin="0.0"))
	float HungerRestoration = 50.0f;

	// Maximum number of times this food can be eaten
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Food", meta=(ClampMin="1"))
	int32 MaxUses = 1;

	// Mesh variants for different use counts
	// Index 0 = full (max uses), Index N-1 = 1 use remaining
	// If array is empty or shorter than MaxUses, uses PickupMesh from ItemDefinition
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Food|Visual")
	TArray<TObjectPtr<UStaticMesh>> UsesRemainingMeshVariants;

	// Optional item definition to replace this item with when uses run out
	// (e.g., empty can after eating canned food)
	// If null, item is removed when uses reach 0
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Food")
	TObjectPtr<UItemDefinition> ReplacementItemDefinition;

	// Get the mesh to use for the given number of uses remaining
	// Returns nullptr if no variant is available (should use default PickupMesh)
	UFUNCTION(BlueprintPure, Category="Food")
	UStaticMesh* GetMeshForUsesRemaining(int32 UsesRemaining) const;
};

