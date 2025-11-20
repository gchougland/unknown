#pragma once

#include "CoreMinimal.h"
#include "Widgets/SWidget.h"

class UInventoryScreenWidget;
class UItemDefinition;
struct FLocalMenuState;

namespace InventoryContextMenuActions
{
    // Shared state for menu anchor management
    struct FLocalMenuState
    {
        TWeakPtr<class SMenuAnchor> AssignAnchor;
        TWeakPtr<class SButton> AssignOpener;
        TWeakPtr<SWidget> MainMenuRoot;
        FDelegateHandle TickerHandle;
    };

    // Builds the Assign submenu widget (slots 1-9)
    // ItemType: The item type to assign
    // Widget: The inventory screen widget (for accessing character/hotbar)
    // Returns: Shared widget reference for the submenu
    UNKNOWN_API TSharedRef<SWidget> BuildAssignSubmenu(UItemDefinition* ItemType, UInventoryScreenWidget* Widget);

    // Builds the Assign row widget with menu anchor
    // ItemType: The item type to assign
    // Widget: The inventory screen widget
    // MenuState: Shared menu state for anchor management
    // Returns: Shared widget reference for the assign row
    UNKNOWN_API TSharedRef<SWidget> BuildAssignRow(UItemDefinition* ItemType, UInventoryScreenWidget* Widget, TSharedRef<FLocalMenuState> MenuState);
}

