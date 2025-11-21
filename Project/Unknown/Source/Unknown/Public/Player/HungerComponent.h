#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "HungerComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnHungerChanged, float, CurrentHunger, float, MaxHunger);

/**
 * Component that tracks player hunger level with configurable decay rate.
 * Hunger can exceed MaxHunger temporarily after eating, but decays normally.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UHungerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHungerComponent(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Restore hunger by the given amount (can exceed MaxHunger)
	UFUNCTION(BlueprintCallable, Category="Hunger")
	void RestoreHunger(float Amount);

	// Get current hunger value (may exceed MaxHunger)
	UFUNCTION(BlueprintPure, Category="Hunger")
	float GetCurrentHunger() const { return CurrentHunger; }

	// Get display hunger value (clamped to MaxHunger for UI)
	UFUNCTION(BlueprintPure, Category="Hunger")
	float GetDisplayHunger() const { return FMath::Min(CurrentHunger, MaxHunger); }

	// Get max hunger value
	UFUNCTION(BlueprintPure, Category="Hunger")
	float GetMaxHunger() const { return MaxHunger; }

	// Event fired when hunger changes
	UPROPERTY(BlueprintAssignable, Category="Hunger")
	FOnHungerChanged OnHungerChanged;

protected:
	// Current hunger value (can exceed MaxHunger)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Hunger")
	float CurrentHunger = 100.0f;

	// Maximum hunger value (used for UI display)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunger")
	float MaxHunger = 100.0f;

	// Initial hunger value on BeginPlay
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunger")
	float InitialHunger = 100.0f;

	// Hunger decay rate (hunger per second)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Hunger", meta=(ClampMin="0.0"))
	float DecayRate = 0.0167f; // ~1.0 per minute (1.0 / 60.0)

private:
	void NotifyHungerChanged();
};

