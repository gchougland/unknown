// Physics interaction component to support pickup/hold/rotate/throw mechanics
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhysicsInteractionComponent.generated.h"

class UPrimitiveComponent;

// Delegates for state changes
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPhysicsPickedUp, UPrimitiveComponent* /*Component*/, const FVector& /*PivotWorld*/);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnPhysicsReleased, UPrimitiveComponent* /*Component*/);
DECLARE_MULTICAST_DELEGATE_TwoParams(FOnPhysicsThrown, UPrimitiveComponent* /*Component*/, const FVector& /*Direction*/);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UPhysicsInteractionComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	UPhysicsInteractionComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Defaults and accessors (used by tests)
	float GetMaxPickupDistance() const { return MaxPickupDistance; }
	float GetMinHoldDistance() const { return MinHoldDistance; }
	float GetMaxHoldDistance() const { return MaxHoldDistance; }
	float GetThrowImpulse() const { return ThrowImpulse; }
	float GetRotationSensitivity() const { return RotationSensitivity; }

	// Utility
	float ClampHoldDistance(float InDistance) const;

	// Holding API
	bool BeginHold(UPrimitiveComponent* InComponent, const FVector& PivotWorld, float PickupDistance);
	void Release();
	void Throw(const FVector& Direction);
	bool IsHolding() const { return HeldComponent.IsValid(); }

	// Rotation input accumulation for held object (processed externally per-frame)
	void SetRotateHeld(bool bInRotate) { bWantsRotateHeld = bInRotate; }
	void AddRotationInput(const FVector2D& Delta) { if (bWantsRotateHeld) AccumulatedRotationInput += Delta; }
	FVector2D GetAccumulatedRotationInput() const { return AccumulatedRotationInput; }

	// Per-frame update of the desired hold target in world space (computed from view)
	void UpdateHoldTarget(const FVector& ViewLocation, const FVector& ViewDirection);
	FVector GetTargetHoldLocation() const { return TargetHoldLocation; }

	// Info
	FVector GetPivotLocal() const { return PivotPointLocal; }

	// Delegates
	FOnPhysicsPickedUp OnPickedUp;
	FOnPhysicsReleased OnReleased;
	FOnPhysicsThrown OnThrown;

protected:
	// Configurable properties
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float MaxPickupDistance = 300.f;
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float MinHoldDistance = 50.f;
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float MaxHoldDistance = 300.f;
	// Increase default throw impulse for a snappier feel
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float ThrowImpulse = 4000.f;
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float RotationSensitivity = 1.f;

	// Spring/rotation tuning
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float SpringStiffness = 4000.f; // N/m approximate (uu treated as cm)
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float SpringDamping = 200.f;    // N*s/m
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float MaxLinearForce = 200000.f; // clamp to avoid explosions
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float RotationTorqueStrength = 3000.f; // torque magnitude scaling (reduced for saner default speed)
	UPROPERTY(EditAnywhere, Category="PhysicsInteraction")
	float RotationInputScale = 0.02f; // scales mouse delta to torque axes

	// State
	TWeakObjectPtr<UPrimitiveComponent> HeldComponent;
	FVector PivotPointLocal = FVector::ZeroVector;
	float HoldDistance = 0.f;
	bool bWantsRotateHeld = false;
	FVector2D AccumulatedRotationInput = FVector2D::ZeroVector;
	FVector TargetHoldLocation = FVector::ZeroVector;

 // Saved physics settings while holding
	bool bSavedGravityEnabled = true;
	float SavedLinearDamping = 0.f;
	float SavedAngularDamping = 0.f;

	// Cached view axes for rotation application
	FVector ViewRightAxis = FVector::RightVector;
	FVector ViewUpAxis = FVector::UpVector;

	void ClearState();

	// UActorComponent
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
};
