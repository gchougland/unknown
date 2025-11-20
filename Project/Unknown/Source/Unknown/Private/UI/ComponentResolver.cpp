#include "UI/ComponentResolver.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

namespace ComponentResolver
{
    APawn* GetPawnFromController(APlayerController* Controller)
    {
        if (!Controller)
        {
            return nullptr;
        }
        return Controller->GetPawn();
    }

    UInventoryComponent* ResolveInventoryComponent(APlayerController* Controller)
    {
        APawn* Pawn = GetPawnFromController(Controller);
        if (!Pawn)
        {
            return nullptr;
        }
        return Pawn->FindComponentByClass<UInventoryComponent>();
    }

    UEquipmentComponent* ResolveEquipmentComponent(APlayerController* Controller)
    {
        APawn* Pawn = GetPawnFromController(Controller);
        if (!Pawn)
        {
            return nullptr;
        }
        return Pawn->FindComponentByClass<UEquipmentComponent>();
    }
}

