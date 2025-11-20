#pragma once

#include "CoreMinimal.h"

class UInventoryScreenWidget;
class UItemDefinition;

namespace InventoryContextMenuActions
{
    // Executes the "Equip" action for an item
    // Handles equipment resolution and EquipFromInventory() call
    // Includes refresh logic for UI updates
    // Returns true if the action was executed successfully
    UNKNOWN_API bool ExecuteEquipAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType);
}

