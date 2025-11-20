#pragma once

#include "CoreMinimal.h"

class UInventoryScreenWidget;
class UItemDefinition;

namespace InventoryContextMenuActions
{
    // Executes the "Hold" action for an item
    // Finds the item in inventory and calls Character->HoldItem()
    // Returns true if the action was executed successfully
    UNKNOWN_API bool ExecuteHoldAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType);
}

