#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemUseAction.generated.h"

class ACharacter;
class AItemPickup;
struct FItemEntry;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UItemUseAction : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="Item")
	bool Execute(ACharacter* User, UPARAM(ref) FItemEntry& Item, AItemPickup* WorldPickup = nullptr);
	virtual bool Execute_Implementation(ACharacter* User, FItemEntry& Item, AItemPickup* WorldPickup = nullptr) { return false; }
};
