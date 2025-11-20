#include "UI/InventoryContextMenu/InventoryContextMenuBuilder.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/ProjectStyle.h"
#include "UI/InventoryContextMenu/InventoryContextMenuUse.h"
#include "UI/InventoryContextMenu/InventoryContextMenuHold.h"
#include "UI/InventoryContextMenu/InventoryContextMenuEquip.h"
#include "UI/InventoryContextMenu/InventoryContextMenuDrop.h"
#include "UI/InventoryContextMenu/InventoryContextMenuAssign.h"
#include "Inventory/ItemDefinition.h"
#include "Widgets/SWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Templates/Function.h"

namespace InventoryContextMenuBuilder
{
    void BuildContextMenu(UInventoryScreenWidget* Widget, UItemDefinition* ItemType, const FVector2D& ScreenPosition)
    {
        if (!Widget || !ItemType)
        {
            return;
        }

        // Create shared menu state for Assign submenu anchor management
        TSharedRef<InventoryContextMenuActions::FLocalMenuState> MenuState = MakeShared<InventoryContextMenuActions::FLocalMenuState>();

        // Build menu rows using ProjectStyle helpers
        TSharedRef<SVerticalBox> MenuVBox = SNew(SVerticalBox);
        TWeakObjectPtr<UInventoryScreenWidget> WeakWidget(Widget);

        // Use action
        MenuVBox->AddSlot().AutoHeight().Padding(2.f)
        [
            ProjectStyle::MakeTerminalMenuRow(TEXT("Use"), [WeakWidget, ItemType]()
            {
                if (UInventoryScreenWidget* W = WeakWidget.Get())
                {
                    InventoryContextMenuActions::ExecuteUseAction(W, ItemType);
                    FSlateApplication::Get().DismissAllMenus();
                    UWidgetBlueprintLibrary::SetFocusToGameViewport();
                    W->SetKeyboardFocus();
                }
            })
        ];

        // Hold action
        MenuVBox->AddSlot().AutoHeight().Padding(2.f)
        [
            ProjectStyle::MakeTerminalMenuRow(TEXT("Hold"), [WeakWidget, ItemType]()
            {
                if (UInventoryScreenWidget* W = WeakWidget.Get())
                {
                    InventoryContextMenuActions::ExecuteHoldAction(W, ItemType);
                    FSlateApplication::Get().DismissAllMenus();
                    UWidgetBlueprintLibrary::SetFocusToGameViewport();
                    W->SetKeyboardFocus();
                }
            })
        ];

        // Equip action (only for equippable items)
        if (ItemType && ItemType->bEquippable)
        {
            MenuVBox->AddSlot().AutoHeight().Padding(2.f)
            [
                ProjectStyle::MakeTerminalMenuRow(TEXT("Equip"), [WeakWidget, ItemType]()
                {
                    if (UInventoryScreenWidget* W = WeakWidget.Get())
                    {
                        InventoryContextMenuActions::ExecuteEquipAction(W, ItemType);
                        FSlateApplication::Get().DismissAllMenus();
                        UWidgetBlueprintLibrary::SetFocusToGameViewport();
                        W->SetKeyboardFocus();
                    }
                })
            ];
        }

        // Drop action
        MenuVBox->AddSlot().AutoHeight().Padding(2.f)
        [
            ProjectStyle::MakeTerminalMenuRow(TEXT("Drop"), [WeakWidget, ItemType]()
            {
                if (UInventoryScreenWidget* W = WeakWidget.Get())
                {
                    InventoryContextMenuActions::ExecuteDropAction(W, ItemType);
                    FSlateApplication::Get().DismissAllMenus();
                    UWidgetBlueprintLibrary::SetFocusToGameViewport();
                    W->SetKeyboardFocus();
                }
            })
        ];

        // Assign to Hotbar (hover to open submenu)
        MenuVBox->AddSlot().AutoHeight().Padding(2.f)
        [
            InventoryContextMenuActions::BuildAssignRow(ItemType, Widget, MenuState)
        ];

        // Wrap menu vbox with terminal-styled frame
        TSharedRef<SWidget> MenuWidget = ProjectStyle::MakeTerminalMenuFrame(MenuVBox);

        // Display the menu
        if (TSharedPtr<SWidget> Cached = Widget->GetCachedWidget())
        {
            FSlateApplication::Get().PushMenu(
                Cached.ToSharedRef(),
                FWidgetPath(),
                MenuWidget,
                ScreenPosition,
                FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
            );
        }
    }
}

