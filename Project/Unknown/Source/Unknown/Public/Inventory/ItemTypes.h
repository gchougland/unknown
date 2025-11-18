#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemTypes.generated.h"

class UItemDefinition;

/** Per-item entry stored in containers (no stacking in v1) */
USTRUCT(BlueprintType)
struct FItemEntry
{
	GENERATED_BODY()

	// Shared definition (immutable metadata)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TObjectPtr<UItemDefinition> Def = nullptr;

	// Unique runtime id for this unit (assigned on add if invalid)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FGuid ItemId;

	// Optional custom data (lightweight key/value strings for now)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TMap<FName, FString> CustomData;

	bool IsValid() const { return Def != nullptr; }
};
