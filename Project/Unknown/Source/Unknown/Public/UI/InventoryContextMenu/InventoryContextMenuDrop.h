#pragma once

#include "CoreMinimal.h"

class UInventoryScreenWidget;
class UItemDefinition;

namespace InventoryContextMenuActions
{
    // Executes the "Drop" action for an item
    // Handles inventory removal, pickup spawning, and rollback on failure
    // Returns true if the action was executed successfully
    UNKNOWN_API bool ExecuteDropAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType);
}

