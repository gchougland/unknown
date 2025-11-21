#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemUseAction.h"
#include "ItemUseAction_Eat.generated.h"

class UItemDefinition;

/**
 * USE action for eating food items.
 * Restores hunger to the player, tracks uses, updates meshes, and replaces items when exhausted.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API UUseAction_Eat : public UItemUseAction
{
	GENERATED_BODY()

public:
	virtual bool Execute_Implementation(ACharacter* User, UPARAM(ref) FItemEntry& Item, AItemPickup* WorldPickup = nullptr) override;

private:
	// Helper to update mesh for an item entry (if held or in world)
	void UpdateItemMesh(ACharacter* User, const FItemEntry& Item, UStaticMesh* NewMesh);
	
	// Helper to replace item in inventory or held state, or in world if WorldPickup is provided
	bool ReplaceItem(ACharacter* User, const FItemEntry& OldItem, UItemDefinition* ReplacementDef, AItemPickup* WorldPickup = nullptr);
};

