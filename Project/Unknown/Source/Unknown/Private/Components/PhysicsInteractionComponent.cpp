#include "Components/PhysicsInteractionComponent.h"

#include "Components/PrimitiveComponent.h"

UPhysicsInteractionComponent::UPhysicsInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true; // We drive hold/rotation in tick while holding
	SetComponentTickEnabled(false);
}

float UPhysicsInteractionComponent::ClampHoldDistance(float InDistance) const
{
	return FMath::Clamp(InDistance, MinHoldDistance, MaxHoldDistance);
}

bool UPhysicsInteractionComponent::BeginHold(UPrimitiveComponent* InComponent, const FVector& PivotWorld, float PickupDistance)
{
	if (!InComponent)
	{
		return false;
	}

	HeldComponent = InComponent;
	// Cache the pivot in the component's local space so rotation can be applied about this point in the future
	PivotPointLocal = InComponent->GetComponentTransform().InverseTransformPosition(PivotWorld);
	HoldDistance = ClampHoldDistance(PickupDistance);
	AccumulatedRotationInput = FVector2D::ZeroVector;
	bWantsRotateHeld = false;
	SetComponentTickEnabled(true);

	// Save and modify physics settings for a stable hold
	bSavedGravityEnabled = InComponent->IsGravityEnabled();
	SavedLinearDamping = InComponent->GetLinearDamping();
	SavedAngularDamping = InComponent->GetAngularDamping();
	InComponent->SetEnableGravity(false);
	InComponent->SetLinearDamping(FMath::Max(SavedLinearDamping, 5.f));
	InComponent->SetAngularDamping(FMath::Max(SavedAngularDamping, 5.f));

	OnPickedUp.Broadcast(InComponent, PivotWorld);
	return true;
}

void UPhysicsInteractionComponent::Release()
{
	if (UPrimitiveComponent* Comp = HeldComponent.Get())
	{
		// Restore physics settings
		Comp->SetEnableGravity(bSavedGravityEnabled);
		Comp->SetLinearDamping(SavedLinearDamping);
		Comp->SetAngularDamping(SavedAngularDamping);

		OnReleased.Broadcast(Comp);
	}
	ClearState();
}

void UPhysicsInteractionComponent::Throw(const FVector& Direction)
{
	if (UPrimitiveComponent* Comp = HeldComponent.Get())
	{
		// Apply an impulse at the pivot point in world space
		const FVector PivotWorld = Comp->GetComponentTransform().TransformPosition(PivotPointLocal);
		Comp->AddImpulseAtLocation(Direction.GetSafeNormal() * ThrowImpulse, PivotWorld, NAME_None);

		// Restore physics settings before notifying
		Comp->SetEnableGravity(bSavedGravityEnabled);
		Comp->SetLinearDamping(SavedLinearDamping);
		Comp->SetAngularDamping(SavedAngularDamping);

		OnThrown.Broadcast(Comp, Direction);
	}
	ClearState();
}

void UPhysicsInteractionComponent::UpdateHoldTarget(const FVector& ViewLocation, const FVector& ViewDirection)
{
	const FVector Dir = ViewDirection.GetSafeNormal();
	TargetHoldLocation = ViewLocation + Dir * HoldDistance;
	// Precompute view right/up for rotation axes using world up (Z)
	const FVector WorldUp = FVector::UpVector;
	FVector Right = FVector::CrossProduct(Dir, WorldUp);
	if (!Right.Normalize())
	{
		Right = FVector::RightVector; // fallback
	}
	const FVector Up = FVector::CrossProduct(Right, Dir).GetSafeNormal();
	// Store on instance for tick use
	// Note: add members ViewRightAxis/ViewUpAxis in header
	ViewRightAxis = Right;
	ViewUpAxis = Up;
}

void UPhysicsInteractionComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UPrimitiveComponent* Comp = HeldComponent.Get();
	if (!Comp)
	{
		return;
	}

	// Move toward target using a critically-damped spring approximation
	const FVector PivotWorld = Comp->GetComponentTransform().TransformPosition(PivotPointLocal);
	const FVector ToTarget = TargetHoldLocation - PivotWorld;

	// Velocity of the point where we apply the force
	const FVector PointVel = Comp->GetPhysicsLinearVelocityAtPoint(PivotWorld);
	const FVector DesiredVel = FVector::ZeroVector; // hold steady at target
	const FVector RelVel = PointVel - DesiredVel;

	FVector Force = (ToTarget * SpringStiffness) + (-RelVel * SpringDamping);
	const float ForceSize = Force.Size();
	if (ForceSize > MaxLinearForce && ForceSize > KINDA_SMALL_NUMBER)
	{
		Force *= (MaxLinearForce / ForceSize);
	}
	Comp->AddForceAtLocation(Force, PivotWorld);

	// Apply rotation torque from accumulated input when rotate mode is active
	if (bWantsRotateHeld && !AccumulatedRotationInput.IsNearlyZero())
	{
		// Pitch input (Y) rotates around view right; Yaw input (X) rotates around view up
		const FVector2D ScaledInput = AccumulatedRotationInput * RotationInputScale;
		const FVector TorqueAxis = (ViewRightAxis * -ScaledInput.Y) + (ViewUpAxis * ScaledInput.X);
		const FVector Torque = TorqueAxis.GetClampedToMaxSize(1.f) * RotationTorqueStrength;
		Comp->AddTorqueInRadians(Torque, NAME_None, true);
		AccumulatedRotationInput = FVector2D::ZeroVector; // consume
	}
}

void UPhysicsInteractionComponent::ClearState()
{
	SetComponentTickEnabled(false);
	HeldComponent = nullptr;
	PivotPointLocal = FVector::ZeroVector;
	HoldDistance = 0.f;
	bWantsRotateHeld = false;
	AccumulatedRotationInput = FVector2D::ZeroVector;
	TargetHoldLocation = FVector::ZeroVector;
	bSavedGravityEnabled = true;
	SavedLinearDamping = 0.f;
	SavedAngularDamping = 0.f;
}
