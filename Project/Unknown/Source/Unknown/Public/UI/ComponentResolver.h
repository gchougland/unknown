#pragma once

#include "CoreMinimal.h"

class UInventoryComponent;
class UEquipmentComponent;
class APlayerController;
class APawn;

namespace ComponentResolver
{
    /**
     * Resolves the inventory component from the owning player controller's pawn
     * @param Controller The player controller (can be nullptr)
     * @return The inventory component if found, nullptr otherwise
     */
    UNKNOWN_API UInventoryComponent* ResolveInventoryComponent(APlayerController* Controller);

    /**
     * Resolves the equipment component from the owning player controller's pawn
     * @param Controller The player controller (can be nullptr)
     * @return The equipment component if found, nullptr otherwise
     */
    UNKNOWN_API UEquipmentComponent* ResolveEquipmentComponent(APlayerController* Controller);

    /**
     * Gets the pawn from a player controller
     * @param Controller The player controller (can be nullptr)
     * @return The pawn if found, nullptr otherwise
     */
    UNKNOWN_API APawn* GetPawnFromController(APlayerController* Controller);
}

