#pragma once

#include "CoreMinimal.h"
#include "DimensionTypes.generated.h"

/**
 * Types of dimensions that can be scanned and loaded
 */
UENUM(BlueprintType)
enum class EDimensionType : uint8
{
    Standard     UMETA(DisplayName="Standard"),
    Rare         UMETA(DisplayName="Rare"),
    Event        UMETA(DisplayName="Event"),
    Special      UMETA(DisplayName="Special")
};
