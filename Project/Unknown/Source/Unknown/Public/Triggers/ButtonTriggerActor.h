#pragma once

#include "CoreMinimal.h"
#include "Triggers/TriggerActorBase.h"
#include "Components/StaticMeshComponent.h"
#include "ButtonTriggerActor.generated.h"

class UBoxComponent;

/**
 * Button trigger actor that triggers targets when the player interacts with it via line trace
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API AButtonTriggerActor : public ATriggerActorBase
{
	GENERATED_BODY()

public:
	AButtonTriggerActor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Static mesh component for button visual
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Button")
	TObjectPtr<UStaticMeshComponent> ButtonMesh;

	// Interaction collider (for line trace detection)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Button")
	TObjectPtr<UBoxComponent> InteractionCollider;

	// Check if player is looking at this button (called from player controller)
	UFUNCTION(BlueprintCallable, Category="Button")
	bool CanBeInteractedWith(APawn* InteractingPawn) const;

	// Called when player interacts with button
	UFUNCTION(BlueprintCallable, Category="Button")
	void OnInteracted();

protected:
	virtual void OnTriggerActivated() override;
};
