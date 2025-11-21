#include "Inventory/ItemUseActions/ItemUseAction_OpenStorage.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Player/FirstPersonPlayerController.h"
#include "Player/FirstPersonCharacter.h"
#include "UI/InventoryScreenWidget.h"
#include "GameFramework/Character.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "Engine/OverlapResult.h"

bool UUseAction_OpenStorage::Execute_Implementation(ACharacter* User, FItemEntry& Item)
{
	if (!User || !Item.IsValid() || !Item.Def)
	{
		return false;
	}

	// Get the player controller
	AFirstPersonPlayerController* PC = Cast<AFirstPersonPlayerController>(User->GetController());
	if (!PC)
	{
		return false;
	}

	// Try to find storage component
	// For containers, the storage component should be on a world actor
	// We'll look for storage components on actors near the player
	UStorageComponent* StorageComp = nullptr;
	
	// First, check if the user's pawn has a storage component (for equipped containers)
	StorageComp = User->FindComponentByClass<UStorageComponent>();
	
	// If not found, try to find a nearby container actor in the world
	// Perform a line trace to find what the player is looking at
	if (!StorageComp)
	{
		FVector ViewLocation;
		FRotator ViewRotation;
		User->GetActorEyesViewPoint(ViewLocation, ViewRotation);
		FVector TraceEnd = ViewLocation + ViewRotation.Vector() * 500.f; // 5 meter range
		
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OpenStorage), false, User);
		FHitResult Hit;
		UWorld* World = User->GetWorld();
		if (World && World->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams))
		{
			if (AActor* HitActor = Hit.GetActor())
			{
				StorageComp = HitActor->FindComponentByClass<UStorageComponent>();
			}
		}
	}

	// If still not found, search nearby actors (within 2 meters)
	if (!StorageComp)
	{
		UWorld* World = User->GetWorld();
		if (World)
		{
			TArray<FOverlapResult> Overlaps;
			FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(OpenStorage), false, User);
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(200.f); // 2 meter radius
			World->OverlapMultiByChannel(Overlaps, User->GetActorLocation(), FQuat::Identity, ECC_WorldDynamic, SphereShape, QueryParams);
			
			for (const FOverlapResult& Overlap : Overlaps)
			{
				if (AActor* OverlapActor = Overlap.GetActor())
				{
					StorageComp = OverlapActor->FindComponentByClass<UStorageComponent>();
					if (StorageComp)
					{
						break;
					}
				}
			}
		}
	}

	if (!StorageComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenStorage] Storage component not found. Container must be a world actor with UStorageComponent."));
		return false;
	}

	// Get player inventory
	UInventoryComponent* PlayerInventory = nullptr;
	if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(User))
	{
		PlayerInventory = Char->GetInventory();
	}

	if (!PlayerInventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[OpenStorage] Player inventory not found"));
		return false;
	}

	// Open inventory screen with storage (will open if closed, or update if already open)
	PC->OpenInventoryWithStorage(StorageComp);
	
	UE_LOG(LogTemp, Display, TEXT("[OpenStorage] Opened inventory with storage"));
	
	return true;
}

