#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ITriggerable.generated.h"

/**
 * Interface for objects that can be triggered by trigger actors
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UTriggerable : public UInterface
{
	GENERATED_BODY()
};

class UNKNOWN_API ITriggerable
{
	GENERATED_BODY()

public:
	/**
	 * Called when this object is triggered by a trigger actor
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Trigger")
	void Trigger();
	virtual void Trigger_Implementation() {}
};
