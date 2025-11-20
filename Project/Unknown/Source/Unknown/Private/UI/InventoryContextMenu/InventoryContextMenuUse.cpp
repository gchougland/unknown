#include "UI/InventoryContextMenu/InventoryContextMenuUse.h"
#include "UI/InventoryScreenWidget.h"
#include "Inventory/ItemDefinition.h"

namespace InventoryContextMenuActions
{
    bool ExecuteUseAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType)
    {
        if (!Widget || !ItemType)
        {
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Use requested: %s"), *GetNameSafe(ItemType));
        // TODO: Implement actual use logic when item use system is ready
        return true;
    }
}

