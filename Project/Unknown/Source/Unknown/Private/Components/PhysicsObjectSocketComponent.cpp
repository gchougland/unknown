#include "Components/PhysicsObjectSocketComponent.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"

UPhysicsObjectSocketComponent::UPhysicsObjectSocketComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	SetComponentTickEnabled(false); // Only enable when socketing/interpolating
}

void UPhysicsObjectSocketComponent::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap events to the trigger box if it exists
	if (SocketTriggerBox)
	{
		SocketTriggerBox->OnComponentBeginOverlap.AddDynamic(this, &UPhysicsObjectSocketComponent::OnSocketBeginOverlap);
		SocketTriggerBox->OnComponentEndOverlap.AddDynamic(this, &UPhysicsObjectSocketComponent::OnSocketEndOverlap);
	}
}

void UPhysicsObjectSocketComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AItemPickup* Item = SocketedItem.Get();
	if (!Item || !Item->IsValidLowLevel())
	{
		// No item socketed, disable tick
		SetComponentTickEnabled(false);
		return;
	}

	UPrimitiveComponent* ItemMesh = Item->Mesh;
	if (!ItemMesh)
	{
		SetComponentTickEnabled(false);
		return;
	}

	// Get target transform in world space (use this component's transform)
	const FTransform TargetTransform = GetComponentTransform();
	const FTransform CurrentTransform = ItemMesh->GetComponentTransform();

	// Interpolate position
	const FVector CurrentLocation = CurrentTransform.GetLocation();
	const FVector TargetLocation = TargetTransform.GetLocation();
	const FVector NewLocation = FMath::VInterpTo(CurrentLocation, TargetLocation, DeltaTime, SnapSpeed);

	// Interpolate rotation
	const FRotator CurrentRotation = CurrentTransform.GetRotation().Rotator();
	const FRotator TargetRotation = TargetTransform.GetRotation().Rotator();
	const FRotator NewRotation = FMath::RInterpTo(CurrentRotation, TargetRotation, DeltaTime, SnapRotationSpeed);

	// Apply new transform
	ItemMesh->SetWorldLocation(NewLocation);
	ItemMesh->SetWorldRotation(NewRotation);

	// Check if we've reached the target (within small tolerance)
	const float LocationTolerance = 1.0f; // 1 unit
	const float RotationTolerance = 1.0f; // 1 degree
	const FRotator RotationDelta = (NewRotation - TargetRotation).GetNormalized();
	const bool bReachedTarget = 
		FVector::Dist(NewLocation, TargetLocation) < LocationTolerance &&
		FMath::Abs(RotationDelta.Pitch) < RotationTolerance &&
		FMath::Abs(RotationDelta.Yaw) < RotationTolerance &&
		FMath::Abs(RotationDelta.Roll) < RotationTolerance;

	if (bReachedTarget)
	{
		// Snap to exact target
		ItemMesh->SetWorldLocation(TargetLocation);
		ItemMesh->SetWorldRotation(TargetRotation);
		// Keep tick enabled to maintain position (in case socket moves)
	}
}

void UPhysicsObjectSocketComponent::OnSocketBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Ignore if we already have a socketed item
	if (SocketedItem.IsValid())
	{
		return;
	}

	// Check if the overlapping actor is an ItemPickup
	AItemPickup* ItemPickup = Cast<AItemPickup>(OtherActor);
	if (!ItemPickup)
	{
		return;
	}

	// Check if this item was manually released (prevent reattachment until it leaves and re-enters)
	if (ManuallyReleasedItem.IsValid() && ManuallyReleasedItem.Get() == ItemPickup)
	{
		return;
	}

	// Get the item's definition
	UItemDefinition* ItemDef = ItemPickup->GetItemDef();
	if (!ItemDef)
	{
		return;
	}

	// Check if item tags match accepted tags (if tags are specified)
	if (!AcceptedItemTags.IsEmpty())
	{
		if (!ItemDef->Tags.HasAll(AcceptedItemTags))
		{
			return; // Item doesn't match tag requirements
		}
	}

	// Start socketing the item
	UPrimitiveComponent* ItemMesh = ItemPickup->Mesh;
	if (!ItemMesh)
	{
		return;
	}

	// Save physics state
	bSavedSimulatePhysics = ItemMesh->IsSimulatingPhysics();
	bSavedGravityEnabled = ItemMesh->IsGravityEnabled();

	// If this item is currently being held by a PhysicsInteractionComponent, release it
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<APawn> PawnIterator(World); PawnIterator; ++PawnIterator)
		{
			APawn* Pawn = *PawnIterator;
			if (!Pawn)
			{
				continue;
			}

			UPhysicsInteractionComponent* PIC = Pawn->FindComponentByClass<UPhysicsInteractionComponent>();
			if (PIC && PIC->IsHoldingComponent(ItemMesh))
			{
				// This item is being held - release it from physics interaction
				PIC->Release();
				break; // Only one component should be holding it
			}
		}
	}

	// Disable physics
	ItemMesh->SetSimulatePhysics(false);
	ItemMesh->SetEnableGravity(false);

	// Store reference to socketed item
	SocketedItem = ItemPickup;

	// Enable tick for interpolation
	SetComponentTickEnabled(true);

	// Broadcast delegate
	OnItemSocketed.Broadcast(ItemPickup);
}

void UPhysicsObjectSocketComponent::OnSocketEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	AItemPickup* ItemPickup = Cast<AItemPickup>(OtherActor);
	if (!ItemPickup)
	{
		return;
	}

	// If the item that left is the currently socketed item, release it
	if (SocketedItem.IsValid() && SocketedItem.Get() == ItemPickup)
	{
		UPrimitiveComponent* ItemMesh = ItemPickup->Mesh;
		if (ItemMesh)
		{
			// Re-enable physics when item leaves trigger box
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetEnableGravity(true);
		}

		// Broadcast delegate before clearing reference
		OnItemReleased.Broadcast(ItemPickup);

		// Clear socketed item reference
		SocketedItem = nullptr;

		// Disable tick
		SetComponentTickEnabled(false);
	}

	// If the item that left is the one that was manually released, clear the release tracking
	// This allows reattachment when the item re-enters
	if (ManuallyReleasedItem.IsValid() && ManuallyReleasedItem.Get() == ItemPickup)
	{
		ManuallyReleasedItem = nullptr;
	}
}

bool UPhysicsObjectSocketComponent::IsItemSocketed(AItemPickup* Item) const
{
	return SocketedItem.IsValid() && SocketedItem.Get() == Item;
}

void UPhysicsObjectSocketComponent::ReleaseItem()
{
	AItemPickup* Item = SocketedItem.Get();
	if (!Item || !Item->IsValidLowLevel())
	{
		return;
	}

	UPrimitiveComponent* ItemMesh = Item->Mesh;
	if (ItemMesh)
	{
		// Always re-enable physics when releasing (items should have physics when not socketed)
		ItemMesh->SetSimulatePhysics(true);
		ItemMesh->SetEnableGravity(true);
	}

	// Mark as manually released (so it won't reattach until it leaves and re-enters)
	ManuallyReleasedItem = Item;

	// Broadcast delegate before clearing reference
	OnItemReleased.Broadcast(Item);

	// Clear socketed item reference
	SocketedItem = nullptr;

	// Disable tick
	SetComponentTickEnabled(false);
}


UPhysicsObjectSocketComponent* UPhysicsObjectSocketComponent::FindSocketWithItem(AItemPickup* Item, UWorld* World)
{
	if (!Item || !World)
	{
		return nullptr;
	}

	// Search all actors for socket components
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		// Find all socket components on this actor
		TArray<UPhysicsObjectSocketComponent*> SocketComponents;
		Actor->GetComponents<UPhysicsObjectSocketComponent>(SocketComponents);

		for (UPhysicsObjectSocketComponent* SocketComp : SocketComponents)
		{
			if (SocketComp && SocketComp->IsItemSocketed(Item))
			{
				return SocketComp;
			}
		}
	}

	return nullptr;
}

