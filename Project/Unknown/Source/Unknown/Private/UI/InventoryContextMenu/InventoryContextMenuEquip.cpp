#include "UI/InventoryContextMenu/InventoryContextMenuEquip.h"
#include "UI/InventoryScreenWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/ItemTypes.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace InventoryContextMenuActions
{
    bool ExecuteEquipAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType)
    {
        if (!Widget || !ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: missing Widget or ItemType"));
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Equip requested: %s"), *GetNameSafe(ItemType));

        UInventoryComponent* Inventory = Widget->GetInventory();
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: Inventory is null"));
            return false;
        }

        // Resolve Equipment (prefer cached; else attempt to find on pawn)
        UEquipmentComponent* EquipComp = Widget->GetEquipment();
        if (!EquipComp)
        {
            if (APlayerController* PC = Widget->GetOwningPlayer())
            {
                if (APawn* Pawn = PC->GetPawn())
                {
                    EquipComp = Pawn->FindComponentByClass<UEquipmentComponent>();
                }
            }
        }
        if (!EquipComp)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: EquipmentComponent not found on pawn"));
            return false;
        }

        // Find one entry of this type to equip
        FGuid ItemIdToEquip;
        for (const FItemEntry& E : Inventory->GetEntries())
        {
            if (E.Def == ItemType)
            {
                ItemIdToEquip = E.ItemId;
                break;
            }
        }
        if (!ItemIdToEquip.IsValid())
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: no entry of type %s in inventory"), *GetNameSafe(ItemType));
            return false;
        }

        FText Error;
        if (EquipComp->EquipFromInventory(ItemIdToEquip, Error))
        {
            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Equipped %s"), *GetNameSafe(ItemType));
            // Refresh inventory view - widget will handle this via equipment event handlers
            Widget->RefreshInventoryView();
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip failed: %s"), *Error.ToString());
            return false;
        }
    }
}

