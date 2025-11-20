#include "UI/InventoryContextMenu/InventoryContextMenuDrop.h"
#include "UI/InventoryScreenWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemPickup.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

namespace InventoryContextMenuActions
{
    bool ExecuteDropAction(UInventoryScreenWidget* Widget, UItemDefinition* ItemType)
    {
        if (!Widget || !ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: Widget or ItemType is null"));
            return false;
        }

        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Drop requested: %s"), *GetNameSafe(ItemType));

        UInventoryComponent* Inventory = Widget->GetInventory();
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: Inventory is null"));
            return false;
        }

        // Find one entry of this type to remove
        FItemEntry EntryToDrop;
        bool bFound = false;
        for (const FItemEntry& E : Inventory->GetEntries())
        {
            if (E.Def == ItemType)
            {
                EntryToDrop = E;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: No entry of type %s in inventory"), *GetNameSafe(ItemType));
            return false;
        }

        // Remove from inventory first
        if (!Inventory->RemoveById(EntryToDrop.ItemId))
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop failed: RemoveById(%s) returned false"), *EntryToDrop.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
            return false;
        }

        UWorld* World = Widget->GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Drop error: World is null; restoring item to inventory"));
            Inventory->TryAdd(EntryToDrop);
            Widget->RefreshInventoryView();
            return false;
        }

        APlayerController* PC = Widget->GetOwningPlayer();
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;

        FActorSpawnParameters Params;
        // Do not spawn if colliding; we will roll back the inventory removal to avoid item loss
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

        const AActor* Context = Pawn ? static_cast<AActor*>(Pawn) : static_cast<AActor*>(PC);
        AItemPickup* Pickup = AItemPickup::SpawnDropped(World, Context, ItemType, Params);
        if (!Pickup)
        {
            UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Failed to spawn AItemPickup at safe location; restoring item to inventory"));
            Inventory->TryAdd(EntryToDrop);
            Widget->RefreshInventoryView();
            return false;
        }
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Dropped one %s at %s"), *GetNameSafe(ItemType), *Pickup->GetActorLocation().ToCompactString());

        // Update UI
        Widget->RefreshInventoryView();
        return true;
    }
}

