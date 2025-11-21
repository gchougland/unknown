#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemAttackAction.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Curves/CurveFloat.h"
#include "AttackAction_Melee.generated.h"

class AItemPickup;

/**
 * Melee attack action for items like crowbars
 * Uses timer-based cooldown (no skeletal mesh required)
 * Animates the held item itself with a swing animation
 */
UCLASS(BlueprintType, Blueprintable, EditInlineNew, DefaultToInstanced)
class UNKNOWN_API UAttackAction_Melee : public UItemAttackAction
{
	GENERATED_BODY()

public:
	UAttackAction_Melee();

	virtual bool ExecuteAttack_Implementation(ACharacter* User, FItemEntry& Item, AActor* Target, const FVector& HitLocation, const FVector& HitDirection) override;

protected:
	/**
	 * Handle attack cooldown timer completion
	 */
	UFUNCTION()
	void OnAttackCooldownFinished();

	/**
	 * Update swing animation (called by timer tick)
	 */
	void UpdateSwingAnimation(float ElapsedTime);

	/**
	 * Handle swing animation completion
	 */
	UFUNCTION()
	void OnSwingAnimationFinished();

	/**
	 * Apply force to the target actor
	 */
	void ApplyForceToTarget(AActor* Target, const FVector& HitLocation, const FVector& HitDirection);

	/**
	 * Get the held item actor from the character
	 */
	AItemPickup* GetHeldItemActor(ACharacter* User) const;

	// Duration of the attack cooldown (in seconds)
	// This simulates the attack animation duration
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Timing", meta=(ClampMin="0.1"))
	float AttackDuration = 0.5f;

	// Whether to animate the item swing
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Animation")
	bool bAnimateSwing = true;

	// Swing animation curve (controls the swing motion over time)
	// X-axis: time (0-1), Y-axis: swing progress (0-1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Animation", meta=(EditCondition="bAnimateSwing"))
	TObjectPtr<UCurveFloat> SwingCurve;

	// Maximum swing rotation angle (in degrees)
	// Positive values swing forward/right, negative swing backward/left
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Animation", meta=(EditCondition="bAnimateSwing"))
	float SwingAngleDegrees = 45.0f;

	// Swing axis (relative to item's forward direction)
	// 0 = Pitch (up/down), 1 = Yaw (left/right), 2 = Roll (twist)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Animation", meta=(EditCondition="bAnimateSwing"))
	int32 SwingAxis = 1; // Default: Yaw (horizontal swing)

	// Force magnitude to apply to attackable targets
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Physics", meta=(ClampMin="0.0"))
	float ForceMagnitude = 1000.0f;

	// Whether to use impulse (instant force) or continuous force
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Attack|Physics")
	bool bUseImpulse = true;

private:
	// Timer handle for swing animation updates
	FTimerHandle SwingTimerHandle;

	// Original relative transform of the held item (before swing)
	FTransform OriginalRelativeTransform;

	// Reference to the held item actor being animated
	UPROPERTY()
	TWeakObjectPtr<AItemPickup> AnimatedItemActor;

	// Reference to the character (needed for timer callbacks)
	UPROPERTY()
	TWeakObjectPtr<ACharacter> AnimatingCharacter;

	// Elapsed time for swing animation
	float SwingElapsedTime = 0.0f;
};

