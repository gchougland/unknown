#include "UI/InventoryContextMenu/InventoryContextMenuHold.h"
#include "UI/InventoryScreenWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypes.h"
#include "Player/FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace InventoryContextMenuActions
{
    bool ExecuteHoldAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType)
    {
        if (!Widget || !ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: Widget or ItemType is null"));
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Hold requested: %s"), *GetNameSafe(ItemType));

        // Access inventory through widget
        UInventoryComponent* Inventory = Widget->GetInventory();
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: Inventory is null"));
            return false;
        }

        // Get the character
        APlayerController* PC = Widget->GetOwningPlayer();
        if (!PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: PlayerController is null"));
            return false;
        }

        APawn* Pawn = PC->GetPawn();
        AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
        if (!Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: Pawn is not a FirstPersonCharacter"));
            return false;
        }

        // Check if character is already holding an item
        if (Character->IsHoldingItem())
        {
            // Put the currently held item back first
            Character->PutHeldItemBack();
        }

        // Find the first item entry of this type in the inventory
        FItemEntry ItemToHold;
        bool bFound = false;
        for (const FItemEntry& Entry : Inventory->GetEntries())
        {
            if (Entry.Def == ItemType)
            {
                ItemToHold = Entry;
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: No item of type %s found in inventory"), *GetNameSafe(ItemType));
            return false;
        }

        // Hold the item (this will remove from inventory, spawn actor, and attach to socket)
        if (Character->HoldItem(ItemToHold))
        {
            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Successfully holding %s"), *GetNameSafe(ItemType));
            // Refresh inventory view after hold operation
            Widget->RefreshInventoryView();
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold failed: HoldItem returned false"));
            return false;
        }
    }
}

