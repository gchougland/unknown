#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"
#include "Inventory/EquipmentTypes.h"

class UTexture2D;
class UItemUseAction;
class UStaticMesh;
class UBlueprint;
class UItemEquipEffect;
class UFoodItemData;
class AActor;

#include "ItemDefinition.generated.h"

/** Immutable metadata for an item type */
UCLASS(BlueprintType)
class UNKNOWN_API UItemDefinition : public UDataAsset
{
	GENERATED_BODY()
public:
    UItemDefinition();

	// Stable Guid for this definition (optional if using PrimaryAssetId)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FGuid Guid;

	// Programmatic name (PrimaryAssetId.SubPath); DisplayName for UI
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText DisplayName;

	// Default description
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item")
	FText Description;

 // Visuals for pickups/UI
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Visual")
 TObjectPtr<UStaticMesh> PickupMesh;

 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Visual")
 TObjectPtr<UTexture2D> IconOverride;

  // Icon capture pose (separate from hold pose). Applied to the pickup mesh in the icon preview scene.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Visual|Icon")
  FTransform IconCaptureTransform;

	// Volume per unit (liters or project units)
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
 float VolumePerUnit = 1.0f;

 // Physical mass in kilograms used to override the pickup's simulated body mass.
 // Set to <= 0 to use the mesh's default mass from its BodySetup.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics", meta=(ClampMin="0.0"))
 float MassKg = 1.0f;

 // Whether world pickups of this item should collide with Pawn objects (players/NPCs).
 // When false, the pickup will ignore the Pawn channel so characters can walk through it.
 // This only affects the pickup actor's collision setup; it does not change trace visibility on custom channels.
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Physics")
 bool bCollideWithPawns = true;

	// Tags/capabilities
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
 FGameplayTagContainer Tags;

	// Optional default use action class
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
 TSubclassOf<UItemUseAction> DefaultUseAction;

	// Default relative transform for how the item should appear when held in-hand (applied relative to HoldPoint or socket)
 UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Transforms")
 FTransform DefaultHoldTransform;

	// Default relative transform used when dropping/spawning the item in front of the player (applied relative to the computed base drop transform)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Transforms")
    FTransform DefaultDropTransform;

    // Equipment metadata
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Equipment")
    bool bEquippable = false;

    // Pluggable equip effects to apply/remove on equip/unequip. Instanced so per-item assets can tune effect properties.
    UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Item|Equipment", meta=(EditCondition="bEquippable"))
    TArray<TObjectPtr<UItemEquipEffect>> EquipEffects;

    // Whether this item can be stored in inventory
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
    bool bCanBeStored = true;

    // Whether this item can be held in hand
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
    bool bCanBeHeld = true;

    // Optional blueprint override for spawning instead of AItemPickup (must inherit from AItemPickup or implement compatible interface)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Item|Rules")
    TSubclassOf<AActor> PickupActorClass;

    // Food metadata
    UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly, Category="Item|Food")
    TObjectPtr<UFoodItemData> FoodData;
};
