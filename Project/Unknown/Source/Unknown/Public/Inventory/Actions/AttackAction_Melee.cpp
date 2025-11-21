#include "Inventory/Actions/AttackAction_Melee.h"
#include "GameFramework/Character.h"
#include "Interfaces/IAttackable.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemPickup.h"
#include "Player/FirstPersonCharacter.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Curves/CurveFloat.h"
#include "Engine/World.h"

UAttackAction_Melee::UAttackAction_Melee()
{
	ForceMagnitude = 1000.0f;
	bUseImpulse = true;
	AttackRange = 250.0f;
	AttackDuration = 0.5f;
	bAnimateSwing = true;
	SwingAngleDegrees = 45.0f;
	SwingAxis = 1; // Yaw (horizontal swing)
	SwingElapsedTime = 0.0f;
}

bool UAttackAction_Melee::ExecuteAttack_Implementation(ACharacter* User, FItemEntry& Item, AActor* Target, const FVector& HitLocation, const FVector& HitDirection)
{
	if (!User)
	{
		return false;
	}

	// Check if already on cooldown
	if (bIsOnCooldown)
	{
		return false;
	}

	// Set cooldown immediately
	bIsOnCooldown = true;

	// Get the held item actor
	AItemPickup* HeldItem = GetHeldItemActor(User);
	
	// Start swing animation if enabled
	if (bAnimateSwing && HeldItem)
	{
		AnimatedItemActor = HeldItem;
		AnimatingCharacter = User;
		SwingElapsedTime = 0.0f;
		
		// Get relative transform from root component
		if (USceneComponent* RootComp = HeldItem->GetRootComponent())
		{
			OriginalRelativeTransform = RootComp->GetRelativeTransform();
		}
		else
		{
			OriginalRelativeTransform = FTransform::Identity;
		}

		// Set up timer to update swing animation every frame
		UWorld* World = User->GetWorld();
		if (World)
		{
			// Clear any existing timer
			World->GetTimerManager().ClearTimer(SwingTimerHandle);
			
			// Set up tick timer for smooth animation (60fps = 0.016s)
			World->GetTimerManager().SetTimer(
				SwingTimerHandle,
				[this]()
				{
					if (AnimatingCharacter.IsValid())
					{
						UWorld* World = AnimatingCharacter->GetWorld();
						if (World)
						{
							const float DeltaTime = World->GetDeltaSeconds();
							SwingElapsedTime += DeltaTime;
							const float NormalizedTime = FMath::Clamp(SwingElapsedTime / AttackDuration, 0.0f, 1.0f);
							
							UpdateSwingAnimation(NormalizedTime);
							
							// Stop timer when animation completes
							if (NormalizedTime >= 1.0f)
							{
								World->GetTimerManager().ClearTimer(SwingTimerHandle);
								OnSwingAnimationFinished();
							}
						}
					}
				},
				0.016f, // ~60fps update rate
				true // Loop until manually cleared
			);
		}
	}

	// Set up timer to clear cooldown after attack duration
	UWorld* World = User->GetWorld();
	if (World)
	{
		FTimerHandle CooldownTimerHandle;
		World->GetTimerManager().SetTimer(
			CooldownTimerHandle,
			this,
			&UAttackAction_Melee::OnAttackCooldownFinished,
			AttackDuration,
			false // Don't loop
		);
	}
	else
	{
		// No world, clear cooldown immediately (shouldn't happen in normal gameplay)
		bIsOnCooldown = false;
	}

	// Only apply force and notify if we have a valid attackable target
	if (Target)
	{
		// Verify target implements IAttackable
		IAttackable* AttackableTarget = Cast<IAttackable>(Target);
		if (AttackableTarget)
		{
			// Apply force to target
			ApplyForceToTarget(Target, HitLocation, HitDirection);

			// Notify target it was attacked
			if (Item.Def)
			{
				IAttackable::Execute_OnAttacked(Target, User, Item.Def, HitLocation, HitDirection);
			}
		}
	}
	// Note: Swing animation plays regardless of whether target was hit

	// Attack executed (animation played) regardless of whether target was hit
	return true;
}

void UAttackAction_Melee::OnAttackCooldownFinished()
{
	// Clear cooldown when timer finishes
	bIsOnCooldown = false;
}

void UAttackAction_Melee::UpdateSwingAnimation(float NormalizedTime)
{
	if (!AnimatedItemActor.IsValid())
	{
		return;
	}

	AItemPickup* Item = AnimatedItemActor.Get();
	if (!Item || !Item->GetRootComponent())
	{
		return;
	}

	// Calculate swing rotation based on NormalizedTime (0-1) and swing curve
	// NormalizedTime goes from 0 (start) to 1 (end)
	// The curve can make it ease in/out or have custom timing
	
	// Convert swing angle to radians
	float SwingAngleRadians = FMath::DegreesToRadians(SwingAngleDegrees);
	
	// Apply curve if available (otherwise use linear)
	float CurveValue = NormalizedTime;
	if (SwingCurve)
	{
		CurveValue = SwingCurve->GetFloatValue(NormalizedTime);
	}
	
	// Create a swing motion that goes forward then back
	// Use a sine wave for smooth forward-back motion: sin(pi * time) gives 0->1->0
	float SwingProgress = FMath::Sin(PI * NormalizedTime);
	
	// Calculate rotation offset based on swing axis
	FRotator SwingRotation = FRotator::ZeroRotator;
	switch (SwingAxis)
	{
	case 0: // Pitch (up/down)
		SwingRotation.Pitch = SwingAngleRadians * SwingProgress * CurveValue;
		break;
	case 1: // Yaw (left/right) - default
		SwingRotation.Yaw = SwingAngleRadians * SwingProgress * CurveValue;
		break;
	case 2: // Roll (twist)
		SwingRotation.Roll = SwingAngleRadians * SwingProgress * CurveValue;
		break;
	default:
		SwingRotation.Yaw = SwingAngleRadians * SwingProgress * CurveValue;
		break;
	}

	// Apply swing rotation to original transform
	FTransform SwungTransform = OriginalRelativeTransform;
	SwungTransform.SetRotation((SwingRotation.Quaternion() * OriginalRelativeTransform.GetRotation()).GetNormalized());
	
	// Update item's relative transform via root component
	Item->GetRootComponent()->SetRelativeTransform(SwungTransform);
}

void UAttackAction_Melee::OnSwingAnimationFinished()
{
	// Clear the swing timer
	if (AnimatingCharacter.IsValid())
	{
		UWorld* World = AnimatingCharacter->GetWorld();
		if (World)
		{
			World->GetTimerManager().ClearTimer(SwingTimerHandle);
		}
	}
	
	// Reset item to original transform
	if (AnimatedItemActor.IsValid())
	{
		AItemPickup* Item = AnimatedItemActor.Get();
		if (Item && Item->GetRootComponent())
		{
			Item->GetRootComponent()->SetRelativeTransform(OriginalRelativeTransform);
		}
	}
	
	AnimatedItemActor.Reset();
	AnimatingCharacter.Reset();
	SwingElapsedTime = 0.0f;
}

AItemPickup* UAttackAction_Melee::GetHeldItemActor(ACharacter* User) const
{
	if (!User)
	{
		return nullptr;
	}

	// Try to cast to FirstPersonCharacter to access HeldItemActor
	if (AFirstPersonCharacter* FPChar = Cast<AFirstPersonCharacter>(User))
	{
		return FPChar->GetHeldItemActor();
	}

	return nullptr;
}

void UAttackAction_Melee::ApplyForceToTarget(AActor* Target, const FVector& HitLocation, const FVector& HitDirection)
{
	if (!Target)
	{
		return;
	}

	// Find primitive component on target to apply force to
	UPrimitiveComponent* PrimitiveComp = Target->FindComponentByClass<UPrimitiveComponent>();
	if (!PrimitiveComp)
	{
		// Try to get root component as primitive
		PrimitiveComp = Cast<UPrimitiveComponent>(Target->GetRootComponent());
	}

	if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
	{
		// Normalize direction and scale by force magnitude
		FVector ForceVector = HitDirection.GetSafeNormal() * ForceMagnitude;

		if (bUseImpulse)
		{
			// Apply impulse (instant force)
			PrimitiveComp->AddImpulse(ForceVector, NAME_None, true);
		}
		else
		{
			// Apply continuous force
			PrimitiveComp->AddForce(ForceVector, NAME_None, true);
		}
	}
}

