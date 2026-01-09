#include "Components/SaveableActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "EngineUtils.h"

USaveableActorComponent::USaveableActorComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = false;
	bSaveTransform = true;
	bSavePhysicsState = true;
}

void USaveableActorComponent::PostInitProperties()
{
	Super::PostInitProperties();

	// Generate GUID if not set (for both editor-placed and runtime-spawned actors)
	// This ensures each instance gets a unique ID, even if added to a blueprint
	// Note: If loading from save, the GUID will already be set via serialization
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid();
		UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] Generated new GUID in PostInitProperties for %s: %s"), 
			GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), 
			*PersistentId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] Using existing GUID in PostInitProperties for %s: %s"), 
			GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), 
			*PersistentId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	}
}

void USaveableActorComponent::BeginPlay()
{
	Super::BeginPlay();

	// Ensure GUID is set (fallback for edge cases)
	// Note: If loading from save, the GUID should already be set via SetPersistentId() before BeginPlay
	if (!PersistentId.IsValid())
	{
		PersistentId = FGuid::NewGuid();
		UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] Generated new GUID in BeginPlay for %s: %s"), 
			*GetOwner()->GetName(), *PersistentId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] Using existing GUID in BeginPlay for %s: %s"), 
			*GetOwner()->GetName(), *PersistentId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
	}

	// Store original transform if not already set
	// Check if location is zero (which indicates it hasn't been set yet)
	// We don't check scale because it's usually (1,1,1) by default
	if (OriginalTransform.GetLocation().IsNearlyZero())
	{
		if (AActor* Owner = GetOwner())
		{
			OriginalTransform = Owner->GetActorTransform();
			UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] Set OriginalTransform for %s: %s"), 
				*Owner->GetName(), *OriginalTransform.GetLocation().ToString());
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[SaveableActorComponent] OriginalTransform already set for %s: %s"), 
			GetOwner() ? *GetOwner()->GetName() : TEXT("Unknown"), *OriginalTransform.GetLocation().ToString());
	}
}

AActor* USaveableActorComponent::FindActorByGuid(UWorld* World, const FGuid& ActorId)
{
	if (!World || !ActorId.IsValid())
	{
		return nullptr;
	}

	// Iterate through all actors in the world
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		// Check if this actor has a SaveableActorComponent with matching GUID
		if (USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>())
		{
			if (SaveableComp->PersistentId == ActorId)
			{
				return Actor;
			}
		}
	}

	return nullptr;
}

AActor* USaveableActorComponent::FindActorByGuidOrMetadata(
	UWorld* World, 
	const FGuid& ActorId,
	const FString& ActorClassPath,
	const FTransform& OriginalTransform,
	float TransformTolerance)
{
	if (!World || !ActorId.IsValid() || ActorClassPath.IsEmpty())
	{
		return nullptr;
	}

	// First try GUID match (fast path)
	AActor* FoundActor = FindActorByGuid(World, ActorId);
	if (FoundActor)
	{
		return FoundActor;
	}

	// Fallback: Match by class path and original transform
	// This handles cases where GUIDs were regenerated on level load
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		// Check if class path matches
		FString ActorClassPathCurrent = Actor->GetClass()->GetPathName();
		if (ActorClassPathCurrent != ActorClassPath)
		{
			continue;
		}

		// Check if this actor has a SaveableActorComponent
		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (!SaveableComp)
		{
			continue;
		}

		// Skip if this actor already has a valid GUID (was already matched to a different saved state)
		if (SaveableComp->GetPersistentId().IsValid() && SaveableComp->GetPersistentId() != ActorId)
		{
			continue;
		}
		
		// Get transform to compare (use OriginalTransform if set, otherwise use current transform)
		FTransform TransformToCompare = SaveableComp->OriginalTransform;
		if (TransformToCompare.GetLocation().IsNearlyZero() && 
			TransformToCompare.GetRotation().IsIdentity() && 
			TransformToCompare.GetScale3D().IsNearlyZero())
		{
			// OriginalTransform not set yet - use current actor transform
			TransformToCompare = Actor->GetActorTransform();
		}
		
		// Check if transform matches (within tolerance)
		FVector LocationDelta = TransformToCompare.GetLocation() - OriginalTransform.GetLocation();
		if (LocationDelta.Size() <= TransformTolerance)
		{
			// Check rotation (within tolerance)
			FRotator RotationDelta = (TransformToCompare.GetRotation() * OriginalTransform.GetRotation().Inverse()).Rotator();
			if (FMath::Abs(RotationDelta.Pitch) <= TransformTolerance &&
				FMath::Abs(RotationDelta.Yaw) <= TransformTolerance &&
				FMath::Abs(RotationDelta.Roll) <= TransformTolerance)
			{
				// Match found - restore the GUID and ensure OriginalTransform is set
				SaveableComp->SetPersistentId(ActorId);
				if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero())
				{
					SaveableComp->OriginalTransform = OriginalTransform;
				}
				return Actor;
			}
		}
	}

	return nullptr;
}

