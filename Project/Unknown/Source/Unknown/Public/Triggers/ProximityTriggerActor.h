#pragma once

#include "CoreMinimal.h"
#include "Triggers/TriggerActorBase.h"
#include "Components/BoxComponent.h"
#include "ProximityTriggerActor.generated.h"

/**
 * Proximity trigger actor that triggers targets when the player enters the trigger area
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API AProximityTriggerActor : public ATriggerActorBase
{
	GENERATED_BODY()

public:
	AProximityTriggerActor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Trigger box that detects player proximity
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Proximity")
	TObjectPtr<UBoxComponent> TriggerBox;

	// Whether to trigger again when player exits (false = one-time trigger)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Proximity")
	bool bRetriggerOnExit = false;

	// Whether this trigger has already been activated (for one-time triggers)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Proximity")
	bool bHasBeenTriggered = false;

protected:
	// Handle player entering trigger area
	UFUNCTION()
	void OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Handle player exiting trigger area
	UFUNCTION()
	void OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	virtual void OnTriggerActivated() override;
};
