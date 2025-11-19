#pragma once
#include "CoreMinimal.h"

#include "EquipmentTypes.generated.h"

/** Equipment slots available for equipping items. */
UENUM(BlueprintType)
enum class EEquipmentSlot : uint8
{
    Head      UMETA(DisplayName="Head"),
    Chest     UMETA(DisplayName="Chest"),
    Hands     UMETA(DisplayName="Hands"),
    Back      UMETA(DisplayName="Back"),
    Primary   UMETA(DisplayName="Primary"),
    Secondary UMETA(DisplayName="Secondary"),
    Utility   UMETA(DisplayName="Utility"),
    Gadget    UMETA(DisplayName="Gadget")
};
