#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IAttackable.generated.h"

class ACharacter;
class UItemDefinition;

/**
 * Interface for objects that can be attacked by items
 */
UINTERFACE(MinimalAPI, BlueprintType)
class UAttackable : public UInterface
{
	GENERATED_BODY()
};

class UNKNOWN_API IAttackable
{
	GENERATED_BODY()

public:
	/**
	 * Called when this object is attacked by a character with a weapon
	 * @param Attacker The character performing the attack
	 * @param Weapon The item definition of the weapon used
	 * @param HitLocation World location where the attack hit
	 * @param HitDirection Direction of the attack (from attacker to hit location)
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category="Attack")
	void OnAttacked(ACharacter* Attacker, UItemDefinition* Weapon, const FVector& HitLocation, const FVector& HitDirection);
	virtual void OnAttacked_Implementation(ACharacter* Attacker, UItemDefinition* Weapon, const FVector& HitLocation, const FVector& HitDirection) {}
};

