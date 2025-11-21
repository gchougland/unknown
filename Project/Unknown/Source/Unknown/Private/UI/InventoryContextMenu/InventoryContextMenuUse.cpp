#include "UI/InventoryContextMenu/InventoryContextMenuUse.h"
#include "UI/InventoryScreenWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemUseAction.h"
#include "Player/FirstPersonCharacter.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace InventoryContextMenuActions
{
    bool ExecuteUseAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType)
    {
        if (!Widget || !ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: Widget or ItemType is null"));
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Use requested: %s"), *GetNameSafe(ItemType));

        // Check if item has a use action
        if (!ItemType->DefaultUseAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: Item %s has no DefaultUseAction"), *GetNameSafe(ItemType));
            return false;
        }

        // Get inventory
        UInventoryComponent* Inventory = Widget->GetInventory();
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: Inventory is null"));
            return false;
        }

        // Find one entry of this type
        FItemEntry EntryToUse;
        bool bFound = false;
        for (const FItemEntry& Entry : Inventory->GetEntries())
        {
            if (Entry.Def == ItemType)
            {
                EntryToUse = Entry;
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: No entry of type %s in inventory"), *GetNameSafe(ItemType));
            return false;
        }

        // Get character
        APlayerController* PC = Widget->GetOwningPlayer();
        if (!PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: PlayerController is null"));
            return false;
        }

        APawn* Pawn = PC->GetPawn();
        AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
        if (!Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: Pawn is not a FirstPersonCharacter"));
            return false;
        }

        // Create use action instance
        UItemUseAction* UseAction = NewObject<UItemUseAction>(PC, ItemType->DefaultUseAction);
        if (!UseAction)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use aborted: Failed to create UseAction instance"));
            return false;
        }

        // Execute use action (pass nullptr for WorldPickup since this is from inventory)
        if (UseAction->Execute(Character, EntryToUse, nullptr))
        {
            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Successfully used %s"), *GetNameSafe(ItemType));
            // Refresh inventory view after use
            Widget->RefreshInventoryView();
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Use failed: Execute returned false"));
            return false;
        }
    }
}

