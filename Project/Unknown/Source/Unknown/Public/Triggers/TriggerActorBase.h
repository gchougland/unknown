#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ITriggerable.h"
#include "TriggerActorBase.generated.h"

/**
 * Base class for all trigger actors that can trigger multiple target actors
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class UNKNOWN_API ATriggerActorBase : public AActor
{
	GENERATED_BODY()

public:
	ATriggerActorBase(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Array of target actors to trigger (editable in editor)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Trigger", meta=(AllowedClasses="Actor"))
	TArray<TObjectPtr<AActor>> TargetActors;

	// Trigger all target actors
	UFUNCTION(BlueprintCallable, Category="Trigger")
	void TriggerTargets();

	// Check if all target actors are valid and implement ITriggerable
	UFUNCTION(BlueprintPure, Category="Trigger")
	bool AreTargetsValid() const;

protected:
	// Called when this trigger is activated (implemented by derived classes)
	virtual void OnTriggerActivated();

	// Helper to validate and trigger a single target actor
	void TriggerTargetActor(AActor* TargetActor);
};
