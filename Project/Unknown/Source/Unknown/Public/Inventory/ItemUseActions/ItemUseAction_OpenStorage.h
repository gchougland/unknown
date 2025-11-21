#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemUseAction.h"
#include "ItemUseAction_OpenStorage.generated.h"

/**
 * USE action that opens a storage container's inventory screen.
 * The item entry should reference a container actor that has a UStorageComponent.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API UUseAction_OpenStorage : public UItemUseAction
{
	GENERATED_BODY()

public:
	virtual bool Execute_Implementation(ACharacter* User, UPARAM(ref) FItemEntry& Item) override;
};

