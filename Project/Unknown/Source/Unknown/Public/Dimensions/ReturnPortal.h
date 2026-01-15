#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/BoxComponent.h"
#include "ReturnPortal.generated.h"

/**
 * Return portal actor that teleports the player back to the main world.
 * Spawned in dimension levels to allow players to return.
 */
UCLASS(BlueprintType)
class UNKNOWN_API AReturnPortal : public AActor
{
	GENERATED_BODY()
	
public:	
	AReturnPortal(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// Trigger box for teleportation
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<UBoxComponent> TriggerBox;

	// Location in main world where player will be teleported
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	FVector ExitLocation = FVector::ZeroVector;

	// Rotation for player when exiting (optional, uses current rotation if zero)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	FRotator ExitRotation = FRotator::ZeroRotator;

	// Teleport the player to the exit location
	UFUNCTION(BlueprintCallable, Category="Portal")
	void TeleportPlayer();

protected:
	// Handle player overlap
	UFUNCTION()
	void OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
