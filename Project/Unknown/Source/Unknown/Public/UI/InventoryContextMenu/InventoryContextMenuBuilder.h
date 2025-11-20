#pragma once

#include "CoreMinimal.h"
#include "Math/Vector2D.h"

class UInventoryScreenWidget;
class UItemDefinition;

namespace InventoryContextMenuBuilder
{
    // Builds and displays the context menu for an item
    // Widget: The inventory screen widget
    // ItemType: The item definition to show actions for
    // ScreenPosition: Screen position where the menu should appear
    UNKNOWN_API void BuildContextMenu(UInventoryScreenWidget* Widget, UItemDefinition* ItemType, const FVector2D& ScreenPosition);
}

