#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interfaces/ITriggerable.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "SlidingDoor.generated.h"

/**
 * Door state enumeration
 */
UENUM(BlueprintType)
enum class EDoorState : uint8
{
	Closed		UMETA(DisplayName = "Closed"),
	Opening		UMETA(DisplayName = "Opening"),
	Open		UMETA(DisplayName = "Open"),
	Closing	UMETA(DisplayName = "Closing")
};

/**
 * Sliding door actor with two doors that slide in opposite directions
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API ASlidingDoor : public AActor, public ITriggerable
{
	GENERATED_BODY()

public:
	ASlidingDoor(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// ITriggerable interface implementation
	virtual void Trigger_Implementation() override;

	// Root scene component
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	TObjectPtr<USceneComponent> RootSceneComponent;

	// Left door mesh (slides left/negative direction)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	TObjectPtr<UStaticMeshComponent> LeftDoorMesh;

	// Right door mesh (slides right/positive direction)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	TObjectPtr<UStaticMeshComponent> RightDoorMesh;

	// Player detection box (checks if player is between doors)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	TObjectPtr<UBoxComponent> PlayerDetectionBox;

	// Current door state
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Door")
	EDoorState CurrentState = EDoorState::Closed;

	// Door slide distance (how far each door moves when opening)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door|Animation", meta=(ClampMin="0.0"))
	float DoorSlideDistance = 100.0f;

	// Door slide speed (units per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door|Animation", meta=(ClampMin="0.1"))
	float DoorSlideSpeed = 50.0f;

	// Auto-close delay in seconds
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door|AutoClose", meta=(ClampMin="0.0"))
	float AutoCloseDelay = 15.0f;

	// Slide direction axis (0 = X, 1 = Y, 2 = Z)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Door|Animation", meta=(ClampMin="0", ClampMax="2"))
	int32 SlideAxis = 0; // 0 = X axis (left/right), 1 = Y axis (forward/back), 2 = Z axis (up/down)

	// Open the door
	UFUNCTION(BlueprintCallable, Category="Door")
	void OpenDoor();

	// Close the door
	UFUNCTION(BlueprintCallable, Category="Door")
	void CloseDoor();

	// Check if door is open
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsOpen() const { return CurrentState == EDoorState::Open; }

	// Check if door is closed
	UFUNCTION(BlueprintPure, Category="Door")
	bool IsClosed() const { return CurrentState == EDoorState::Closed; }

protected:
	// Handle player entering detection box
	UFUNCTION()
	void OnPlayerDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Handle player exiting detection box
	UFUNCTION()
	void OnPlayerDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

	// Auto-close timer callback
	UFUNCTION()
	void OnAutoCloseTimer();

	// Update door animation
	void UpdateDoorAnimation(float DeltaTime);

	// Transition to a new state
	void SetDoorState(EDoorState NewState);

	// Get the target open progress (0.0 = closed, 1.0 = fully open)
	float GetTargetOpenProgress() const;

	// Get slide direction vector based on axis
	FVector GetSlideDirection(bool bIsLeftDoor) const;

private:
	// Current open progress for each door (0.0 = closed, 1.0 = fully open)
	float LeftDoorProgress = 0.0f;
	float RightDoorProgress = 0.0f;

	// Closed positions (relative to root) for each door
	FVector LeftDoorClosedPosition;
	FVector RightDoorClosedPosition;

	// Open positions (relative to root) for each door
	FVector LeftDoorOpenPosition;
	FVector RightDoorOpenPosition;

	// Auto-close timer handle
	FTimerHandle AutoCloseTimerHandle;

	// Track overlapping pawns
	TSet<TObjectPtr<APawn>> OverlappingPawns;

	// Helper to check if any pawns are overlapping
	bool HasOverlappingPawns() const;

	// Helper to start/stop auto-close timer
	void StartAutoCloseTimer();
	void StopAutoCloseTimer();
};
