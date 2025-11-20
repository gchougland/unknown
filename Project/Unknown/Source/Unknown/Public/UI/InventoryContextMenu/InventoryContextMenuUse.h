#pragma once

#include "CoreMinimal.h"

class UInventoryScreenWidget;
class UItemDefinition;

namespace InventoryContextMenuActions
{
    // Executes the "Use" action for an item
    // Returns true if the action was executed successfully
    UNKNOWN_API bool ExecuteUseAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType);
}

