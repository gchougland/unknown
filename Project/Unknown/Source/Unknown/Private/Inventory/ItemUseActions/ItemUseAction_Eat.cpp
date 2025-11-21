#include "Inventory/ItemUseActions/ItemUseAction_Eat.h"
#include "Inventory/FoodItemData.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Player/HungerComponent.h"
#include "Player/FirstPersonCharacter.h"
#include "Inventory/InventoryComponent.h"
#include "UI/MessageLogSubsystem.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Engine.h"

bool UUseAction_Eat::Execute_Implementation(ACharacter* User, FItemEntry& Item, AItemPickup* WorldPickup)
{
	if (!User || !Item.IsValid() || !Item.Def)
	{
		return false;
	}

	// Check if item has food data
	if (!Item.Def->FoodData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Eat] Item %s does not have FoodData"), *GetNameSafe(Item.Def));
		return false;
	}

	UFoodItemData* FoodData = Item.Def->FoodData;

	// Find hunger component on character
	UHungerComponent* HungerComp = User->FindComponentByClass<UHungerComponent>();
	if (!HungerComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Eat] Character does not have UHungerComponent"));
		return false;
	}

	// Check if player is already full (hunger >= 100)
	if (HungerComp->GetCurrentHunger() >= 100.0f)
	{
		// Display "I'm not hungry" message
		if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
		{
			MsgLog->PushMessage(FText::FromString(TEXT("I'm not hungry")), 2.0f);
		}
		return false;
	}

	// Get current uses remaining (default to MaxUses if not set)
	int32 UsesRemaining = FoodData->MaxUses;
	if (const FString* UsesStr = Item.CustomData.Find(TEXT("UsesRemaining")))
	{
		UsesRemaining = FCString::Atoi(**UsesStr);
	}

	// Restore hunger (can exceed 100)
	HungerComp->RestoreHunger(FoodData->HungerRestoration);

	// Decrement uses
	UsesRemaining = FMath::Max(0, UsesRemaining - 1);

	// Update uses in CustomData
	Item.CustomData.Add(TEXT("UsesRemaining"), FString::Printf(TEXT("%d"), UsesRemaining));

	// If this is a world pickup, update its item entry
	if (WorldPickup)
	{
		WorldPickup->SetItemEntry(Item);
	}

	// Update mesh if we have variants
	if (UsesRemaining > 0 && FoodData->UsesRemainingMeshVariants.Num() > 0)
	{
		UStaticMesh* NewMesh = FoodData->GetMeshForUsesRemaining(UsesRemaining);
		if (NewMesh)
		{
			UpdateItemMesh(User, Item, NewMesh);
			// If world pickup, update its mesh directly
			if (WorldPickup && WorldPickup->Mesh)
			{
				WorldPickup->Mesh->SetStaticMesh(NewMesh);
			}
		}
	}

	// Handle item replacement or removal when uses run out
	if (UsesRemaining <= 0)
	{
		if (FoodData->ReplacementItemDefinition)
		{
			// Replace with replacement item
			if (!ReplaceItem(User, Item, FoodData->ReplacementItemDefinition, WorldPickup))
			{
				UE_LOG(LogTemp, Warning, TEXT("[Eat] Failed to replace item with replacement definition"));
				return false;
			}
		}
		else
		{
			// Remove item from world, inventory, or held state
			if (WorldPickup)
			{
				// Destroy world pickup
				WorldPickup->Destroy();
			}
			else if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(User))
			{
				// Check if item is currently held
				FItemEntry HeldEntry = Char->GetHeldItemEntry();
				if (HeldEntry.ItemId == Item.ItemId)
				{
					// Release held item without putting it back
					Char->ReleaseHeldItem(false);
				}
				else
				{
					// Remove from inventory
					if (UInventoryComponent* Inventory = Char->GetInventory())
					{
						Inventory->RemoveById(Item.ItemId);
					}
				}
			}
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[Eat] Ate food, restored %.1f hunger, %d uses remaining"), 
		FoodData->HungerRestoration, UsesRemaining);

	return true;
}

void UUseAction_Eat::UpdateItemMesh(ACharacter* User, const FItemEntry& Item, UStaticMesh* NewMesh)
{
	if (!NewMesh)
	{
		return;
	}

	// Check if item is currently held
	if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(User))
	{
		FItemEntry HeldEntry = Char->GetHeldItemEntry();
		if (HeldEntry.ItemId == Item.ItemId)
		{
			// Update held item actor's mesh directly
			if (AItemPickup* HeldActor = Char->GetHeldItemActor())
			{
				if (HeldActor->Mesh)
				{
					HeldActor->Mesh->SetStaticMesh(NewMesh);
					UE_LOG(LogTemp, Verbose, TEXT("[Eat] Updated held item mesh"));
				}
			}
		}
	}

	// Note: For world pickups, the mesh is updated directly in Execute_Implementation
	// For items in inventory, the mesh will be updated when they are next held/picked up
}

bool UUseAction_Eat::ReplaceItem(ACharacter* User, const FItemEntry& OldItem, UItemDefinition* ReplacementDef, AItemPickup* WorldPickup)
{
	if (!ReplacementDef)
	{
		return false;
	}

	// If this is a world pickup, replace it in the world
	if (WorldPickup)
	{
		// Create replacement entry
		FItemEntry ReplacementEntry;
		ReplacementEntry.Def = ReplacementDef;
		ReplacementEntry.ItemId = FGuid::NewGuid();
		// Don't copy CustomData - replacement item starts fresh

		// Update the pickup actor with the replacement item
		WorldPickup->SetItemEntry(ReplacementEntry);
		return true;
	}

	// Otherwise, handle inventory/held item replacement
	if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(User))
	{
		UInventoryComponent* Inventory = Char->GetInventory();
		if (!Inventory)
		{
			return false;
		}

		// Check if item is currently held
		FItemEntry HeldEntry = Char->GetHeldItemEntry();
		if (HeldEntry.ItemId == OldItem.ItemId)
		{
			// Release held item
			Char->ReleaseHeldItem(false);

			// Create replacement entry
			FItemEntry ReplacementEntry;
			ReplacementEntry.Def = ReplacementDef;
			ReplacementEntry.ItemId = FGuid::NewGuid();
			// Don't copy CustomData - replacement item starts fresh

			// Add replacement to inventory
			if (Inventory->TryAdd(ReplacementEntry))
			{
				return true;
			}
		}
		else
		{
			// Remove old item from inventory
			if (Inventory->RemoveById(OldItem.ItemId))
			{
				// Create replacement entry
				FItemEntry ReplacementEntry;
				ReplacementEntry.Def = ReplacementDef;
				ReplacementEntry.ItemId = FGuid::NewGuid();

				// Add replacement to inventory
				if (Inventory->TryAdd(ReplacementEntry))
				{
					return true;
				}
				else
				{
					// Rollback: try to add old item back
					FItemEntry RollbackEntry = OldItem;
					Inventory->TryAdd(RollbackEntry);
				}
			}
		}
	}

	return false;
}

