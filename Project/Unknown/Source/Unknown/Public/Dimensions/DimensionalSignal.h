#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DimensionalSignal.generated.h"

class UNiagaraComponent;
class UNiagaraSystem;
class USceneComponent;

/**
 * Represents a scannable dimensional signal in the 3D scanning space.
 * Has a lifetime and can be scanned by the player.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API ADimensionalSignal : public AActor
{
	GENERATED_BODY()

public:
	ADimensionalSignal(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Signal")
	TObjectPtr<USceneComponent> RootSceneComponent;

	// Niagara effect component for visual representation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Signal")
	TObjectPtr<UNiagaraComponent> SignalEffect;

	// Initialize the signal with a lifetime
	UFUNCTION(BlueprintCallable, Category="Signal")
	void InitializeSignal(float Lifetime);

	// Called when signal is scanned
	UFUNCTION(BlueprintCallable, Category="Signal")
	void OnScanned();

	// Check if signal has expired
	UFUNCTION(BlueprintPure, Category="Signal")
	bool IsExpired() const { return RemainingLifetime <= 0.0f; }

	// Get remaining lifetime
	UFUNCTION(BlueprintPure, Category="Signal")
	float GetRemainingLifetime() const { return RemainingLifetime; }

	// Check if signal has been scanned
	UFUNCTION(BlueprintPure, Category="Signal")
	bool HasBeenScanned() const { return bHasBeenScanned; }

	// Niagara effect asset (can be set in blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Signal")
	TObjectPtr<UNiagaraSystem> SignalNiagaraSystem;

protected:
	// Remaining lifetime in seconds
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Signal")
	float RemainingLifetime = 0.0f;

	// Flag to track if signal has been scanned
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Signal")
	bool bHasBeenScanned = false;

	// Timer handle for delayed destruction after scanning
	FTimerHandle DelayedDestroyTimerHandle;

	// Callback for delayed destruction
	void OnDelayedDestroy();
};
