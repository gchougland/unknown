#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Math/Vector.h"
#include "ItemAttackAction.generated.h"

class ACharacter;
class AActor;
struct FItemEntry;

/**
 * Base class for item attack actions
 * Similar to UItemUseAction but for attack behaviors
 */
UCLASS(Abstract, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UItemAttackAction : public UObject
{
	GENERATED_BODY()

public:
	UItemAttackAction();

	/**
	 * Execute the attack action
	 * @param User The character performing the attack
	 * @param Item The item entry being used to attack
	 * @param Target The target actor being attacked (must implement IAttackable)
	 * @param HitLocation World location where the attack hit
	 * @param HitDirection Direction of the attack (from user to hit location)
	 * @return true if attack was successful, false otherwise
	 */
	UFUNCTION(BlueprintNativeEvent, Category="Item")
	bool ExecuteAttack(ACharacter* User, UPARAM(ref) FItemEntry& Item, AActor* Target, const FVector& HitLocation, const FVector& HitDirection);
	virtual bool ExecuteAttack_Implementation(ACharacter* User, FItemEntry& Item, AActor* Target, const FVector& HitLocation, const FVector& HitDirection) { return false; }

	/**
	 * Check if the attack action is currently on cooldown (animation playing)
	 * @return true if attack is on cooldown and cannot be executed
	 */
	UFUNCTION(BlueprintPure, Category="Item")
	bool IsOnCooldown() const { return bIsOnCooldown; }

	/**
	 * Get the attack range for this action
	 * @return Maximum distance this attack can reach
	 */
	UFUNCTION(BlueprintPure, Category="Item")
	float GetAttackRange() const { return AttackRange; }

protected:
	/**
	 * Set the cooldown state (called when animation starts/ends)
	 */
	void SetCooldown(bool bInCooldown) { bIsOnCooldown = bInCooldown; }

	/**
	 * Called when attack animation completes (override to clear cooldown)
	 */
	UFUNCTION()
	virtual void OnAttackAnimationFinished();

	// Maximum attack range
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack", meta=(ClampMin="0.0"))
	float AttackRange = 250.0f;

	// Whether the attack is currently on cooldown (animation playing)
	UPROPERTY(Transient)
	bool bIsOnCooldown = false;
};

