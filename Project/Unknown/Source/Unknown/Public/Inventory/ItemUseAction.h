#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ItemUseAction.generated.h"

class ACharacter;
struct FItemEntry;

UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UItemUseAction : public UObject
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, Category="Item")
	bool Execute(ACharacter* User, UPARAM(ref) FItemEntry& Item);
	virtual bool Execute_Implementation(ACharacter* User, FItemEntry& Item) { return false; }
};
