#include "Player/FirstPersonPlayerController.h"

#include "Player/FirstPersonCharacter.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "UI/DropProgressBarWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/MessageLogSubsystem.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemUseAction.h"
#include "Inventory/ItemAttackAction.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/StorageSerialization.h"
#include "Inventory/InventoryComponent.h"
#include "Interfaces/IAttackable.h"

// Helper struct for interaction logic
struct FPlayerControllerInteractionHandler
{
	// OnInteractPressed handler
	static void OnInteractPressed(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC || PC->bInventoryUIOpen)
		{
			return;
		}

		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C)
		{
			return;
		}

		// Reset state
		PC->bInstantUseExecuted = false;
		PC->bIsHoldingUse = false;
		PC->HoldUseTimer = 0.0f;

		// Check if holding physics object - release it immediately on tap
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (PIC->IsHolding())
			{
				// This is a tap - release immediately
				UE_LOG(LogTemp, Display, TEXT("[Interact] Releasing currently held object."));
				PC->bRotateHeld = false;
				PIC->SetRotateHeld(false);
				PIC->Release();
				return;
			}
		}

		// Start hold timer for use action
		PC->bIsHoldingUse = true;
		// Don't show progress bar yet - wait for tap threshold to pass
	}

	// OnInteractOngoing handler
	static void OnInteractOngoing(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		// This is called every frame while E is held
		// Actual timer update happens in PlayerTick
	}

	// OnInteractReleased handler
	static void OnInteractReleased(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC)
		{
			return;
		}

		if (PC->bInstantUseExecuted)
		{
			// Reset flag for next press
			PC->bInstantUseExecuted = false;
			return;
		}

		// Check if this was a tap (very short press) vs a hold
		const float TapThreshold = 0.25f;
		const bool bWasTap = (PC->HoldUseTimer < TapThreshold);

		if (PC->bIsHoldingUse)
		{
			if (bWasTap)
			{
				// This was a tap - execute tap behavior (physics interaction)
				if (PC->DropProgressBarWidget)
				{
					PC->DropProgressBarWidget->SetVisible(false);
				}

				AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
				if (C && !PC->bInventoryUIOpen)
				{
					if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
					{
						// Perform a line trace from the camera to try to pick a physics object
						FVector CamLoc; FRotator CamRot;
						PC->GetPlayerViewPoint(CamLoc, CamRot);
						const FVector Dir = CamRot.Vector();
						const float MaxDist = PIC->GetMaxPickupDistance();
						const FVector Start = CamLoc;
						const FVector End = Start + Dir * MaxDist;

						FHitResult Hit;
						FCollisionQueryParams Params(SCENE_QUERY_STAT(PhysicsInteract), false);
						Params.AddIgnoredActor(C);

						if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
						{
							UPrimitiveComponent* Prim = Hit.GetComponent();
							if (Prim && Prim->IsSimulatingPhysics())
							{
								// Prefer authored socket "HoldPivot"; fallback to Center of Mass
								FVector PivotWorld = Hit.ImpactPoint;
								if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim))
								{
									static const FName HoldPivotSocket(TEXT("HoldPivot"));
									if (SMC->DoesSocketExist(HoldPivotSocket))
									{
										PivotWorld = SMC->GetSocketLocation(HoldPivotSocket);
									}
								}
								// If still equal to impact point, try Center of Mass
								if (PivotWorld.Equals(Hit.ImpactPoint, KINDA_SMALL_NUMBER))
								{
									PivotWorld = Prim->GetCenterOfMass();
								}
								const float DistanceToPivot = (PivotWorld - Start).Length();
								PIC->BeginHold(Prim, PivotWorld, DistanceToPivot);
							}
						}
					}
				}
			}
			// If it was a hold that was canceled, just cancel without doing anything

			// Reset state
			PC->bIsHoldingUse = false;
			PC->HoldUseTimer = 0.0f;
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetVisible(false);
			}
		}
	}

	// OnRotateHeldPressed handler
	static void OnRotateHeldPressed(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC || PC->bInventoryUIOpen)
		{
			return;
		}
		
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			// Priority 1: Check if physics object is being held -> rotate it
			if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				if (PIC->IsHolding())
				{
					PC->bRotateHeld = true;
					PIC->SetRotateHeld(true);
					return;
				}
			}
			
			// Priority 2: Check if item is being held -> use it
			if (C->IsHoldingItem())
			{
				FItemEntry HeldEntry = C->GetHeldItemEntry();
				if (HeldEntry.IsValid() && HeldEntry.Def && HeldEntry.Def->DefaultUseAction)
				{
					// Create use action instance and execute it
					UItemUseAction* UseAction = NewObject<UItemUseAction>(PC, HeldEntry.Def->DefaultUseAction);
					if (UseAction)
					{
						// Pass nullptr for WorldPickup since this is a held item (not a world pickup)
						if (UseAction->Execute(C, HeldEntry, nullptr))
						{
							UE_LOG(LogTemp, Display, TEXT("[UseHeldItem] Used %s"), *GetNameSafe(HeldEntry.Def));
							
							// Update the held item entry and actor with the modified data
							// (Execute modifies HeldEntry by reference, so we need to sync it back)
							C->UpdateHeldItemEntry(HeldEntry);
							
							// Refresh inventory UI if open
							if (PC->InventoryScreen && PC->InventoryScreen->IsOpen())
							{
								PC->InventoryScreen->RefreshInventoryView();
							}
						}
						else
						{
							UE_LOG(LogTemp, Display, TEXT("[UseHeldItem] Use action failed for %s"), *GetNameSafe(HeldEntry.Def));
						}
					}
				}
			}
		}
	}

	// OnRotateHeldReleased handler
	static void OnRotateHeldReleased(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC || PC->bInventoryUIOpen)
		{
			return;
		}
		
		// Only disable rotation if physics object is being held
		PC->bRotateHeld = false;
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				if (PIC->IsHolding())
				{
					PIC->SetRotateHeld(false);
				}
			}
		}
	}

	// OnThrow handler
	static void OnThrow(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC || PC->bInventoryUIOpen)
		{
			return;
		}
		
		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C)
		{
			return;
		}

		// PRIORITY 1: Check if physics object is being held - if yes, throw it and return early
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (PIC->IsHolding())
			{
				const FVector Dir = PC->GetControlRotation().Vector();
				PIC->Throw(Dir);
				// Clear rotate state on throw to restore camera look immediately
				PC->bRotateHeld = false;
				PIC->SetRotateHeld(false);
				return; // Early return - physics throw takes priority
			}
		}

		// Check if Shift is held and player is holding an item - if so, try to store in container
		const bool bShiftHeld = PC->IsInputKeyDown(EKeys::LeftShift);
		const bool bHoldingItem = C->IsHoldingItem();
		
		if (bShiftHeld && bHoldingItem)
		{
			// Perform line trace to find interactables with storage components
			FVector CamLoc; FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			const FVector Dir = CamRot.Vector();
			const float MaxDist = 300.f; // Match interactable detection range
			const FVector Start = CamLoc;
			const FVector End = Start + Dir * MaxDist;

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(StoreItem), false);
			Params.AddIgnoredActor(C);
			// Note: Held item actor is attached to character and has collision disabled, so it's naturally ignored

			if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
			{
				AActor* HitActor = Hit.GetActor();
				if (HitActor)
				{
					// Check if the hit actor has a storage component
					UStorageComponent* StorageComp = HitActor->FindComponentByClass<UStorageComponent>();
					if (StorageComp)
					{
						// Get the held item entry
						FItemEntry HeldEntry = C->GetHeldItemEntry();
						if (HeldEntry.IsValid() && HeldEntry.Def)
						{
							// Check if item can be stored
							if (!HeldEntry.Def->bCanBeStored)
							{
								// Item cannot be stored
								if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
								{
									MsgLog->PushMessage(FText::FromString(FString::Printf(TEXT("%s cannot be stored"), *HeldEntry.Def->DisplayName.ToString())));
								}
								UE_LOG(LogTemp, Display, TEXT("[StoreItem] Item %s cannot be stored (bCanBeStored is false)"), *GetNameSafe(HeldEntry.Def));
								return; // Don't throw, just return
							}

							// Attempt to store the item
							if (StorageComp->TryAdd(HeldEntry))
							{
								// Success - remove item from held state
								UE_LOG(LogTemp, Display, TEXT("[StoreItem] Successfully stored %s in container"), *GetNameSafe(HeldEntry.Def));
								C->ReleaseHeldItem(false); // Don't try to put back, it's already stored
								
								// Refresh UI if inventory is open
								if (PC->InventoryScreen && PC->InventoryScreen->IsOpen())
								{
									PC->InventoryScreen->RefreshInventoryView();
								}
								return; // Success - don't throw
							}
							else
							{
								// Storage is full or failed
								if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
								{
									MsgLog->PushMessage(FText::FromString(TEXT("Storage full")));
								}
								UE_LOG(LogTemp, Display, TEXT("[StoreItem] Failed to store %s - storage full or invalid"), *GetNameSafe(HeldEntry.Def));
								return; // Don't throw, just return
							}
						}
					}
				}
			}
			// If we get here, Shift was held but no valid storage was found - fall through to attack/throw behavior
		}

		// PRIORITY 2: Check if holding an item with attack action
		if (bHoldingItem)
		{
			FItemEntry HeldEntry = C->GetHeldItemEntry();
			if (HeldEntry.IsValid() && HeldEntry.Def && HeldEntry.Def->DefaultAttackAction)
			{
				// Get or create attack action instance (cached to maintain cooldown state)
				UItemAttackAction* AttackAction = PC->CachedAttackAction;
				
				// Check if we need to create a new instance or if the cached one is for a different item
				if (!AttackAction || !AttackAction->GetClass()->IsChildOf(HeldEntry.Def->DefaultAttackAction))
				{
					AttackAction = NewObject<UItemAttackAction>(PC, HeldEntry.Def->DefaultAttackAction);
					PC->CachedAttackAction = AttackAction;
				}

				if (AttackAction)
				{
					// Check if attack is on cooldown
					if (AttackAction->IsOnCooldown())
					{
						UE_LOG(LogTemp, Verbose, TEXT("[Attack] Attack action is on cooldown, ignoring input"));
						return; // Attack on cooldown, do nothing
					}

					// Perform line trace to find attackable targets
					FVector CamLoc; FRotator CamRot;
					PC->GetPlayerViewPoint(CamLoc, CamRot);
					const FVector Dir = CamRot.Vector();
					const float AttackRange = AttackAction->GetAttackRange();
					const FVector Start = CamLoc;
					const FVector End = Start + Dir * AttackRange;

					FHitResult Hit;
					FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemAttack), false);
					Params.AddIgnoredActor(C);
					// Ignore held item actor
					if (AItemPickup* HeldActor = C->GetHeldItemActor())
					{
						Params.AddIgnoredActor(HeldActor);
					}

					AActor* HitActor = nullptr;
					FVector HitLocation = End; // Default to end of trace if nothing hit
					FVector HitDirection = Dir; // Default to trace direction

					// Try to find an attackable target
					if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
					{
						HitActor = Hit.GetActor();
						if (HitActor)
						{
							// Check if hit actor implements IAttackable
							IAttackable* AttackableTarget = Cast<IAttackable>(HitActor);
							if (AttackableTarget)
							{
								// Calculate hit location and direction
								HitLocation = Hit.ImpactPoint;
								HitDirection = (HitLocation - Start).GetSafeNormal();
							}
							else
							{
								// Hit something but it's not attackable - still use hit location for animation
								HitLocation = Hit.ImpactPoint;
								HitDirection = (HitLocation - Start).GetSafeNormal();
								HitActor = nullptr; // Don't pass non-attackable actor
								UE_LOG(LogTemp, Verbose, TEXT("[Attack] Hit actor %s does not implement IAttackable"), *GetNameSafe(Hit.GetActor()));
							}
						}
					}
					else
					{
						UE_LOG(LogTemp, Verbose, TEXT("[Attack] No target found in range - playing attack animation anyway"));
					}

					// Always execute attack action to play animation, even if no valid target
					// The attack action will handle null target gracefully
					if (AttackAction->ExecuteAttack(C, HeldEntry, HitActor, HitLocation, HitDirection))
					{
						if (HitActor)
						{
							UE_LOG(LogTemp, Display, TEXT("[Attack] Successfully attacked %s with %s"), 
								*GetNameSafe(HitActor), *GetNameSafe(HeldEntry.Def));
						}
						else
						{
							UE_LOG(LogTemp, Verbose, TEXT("[Attack] Attack animation played (no valid target)"));
						}
						return; // Attack executed (animation played), don't fall through to throw
					}
					else
					{
						UE_LOG(LogTemp, Verbose, TEXT("[Attack] Attack action failed for %s"), *GetNameSafe(HeldEntry.Def));
					}
				}
			}
		}

		// FALLBACK: If no physics object and no attack action, do nothing
		// (Original throw behavior for physics objects is already handled above)
	}

	// OnPickupPressed handler
	static void OnPickupPressed(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC || PC->bInventoryUIOpen)
		{
			return;
		}

		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C)
		{
			return;
		}

		// Reset state
		PC->bInstantActionExecuted = false;
		PC->bIsHoldingDrop = false;
		PC->HoldDropTimer = 0.0f;

		// Start hold timer - will check what we're looking at in PlayerTick
		// If looking at pickup: hold it after progress bar fills
		// If looking at empty space and holding item: drop it after progress bar fills
		// Progress bar won't show until after tap threshold (0.25s) to avoid flicker on taps
		PC->bIsHoldingDrop = true;
		// Don't show progress bar yet - wait for tap threshold to pass
	}

	// OnPickupOngoing handler
	static void OnPickupOngoing(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		// This is called every frame while R is held
		// Actual timer update happens in PlayerTick
	}

	// OnPickupReleased handler
	static void OnPickupReleased(AFirstPersonPlayerController* PC, const FInputActionValue& Value)
	{
		if (!PC)
		{
			return;
		}

		if (PC->bInstantActionExecuted)
		{
			// Reset flag for next press
			PC->bInstantActionExecuted = false;
			return;
		}

		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C || PC->bInventoryUIOpen)
		{
			// Cancel hold if no character or inventory open
			if (PC->bIsHoldingDrop)
			{
				PC->bIsHoldingDrop = false;
				PC->HoldDropTimer = 0.0f;
				if (PC->DropProgressBarWidget)
				{
					PC->DropProgressBarWidget->SetVisible(false);
				}
			}
			return;
		}

		// Check if this was a tap (very short press) vs a hold
		const float TapThreshold = 0.25f; // Consider it a tap if released within 0.25 seconds
		const bool bWasTap = (PC->HoldDropTimer < TapThreshold);

		if (PC->bIsHoldingDrop)
		{
			if (bWasTap)
			{
				// This was a tap - execute tap behavior
				// Hide progress bar first
				if (PC->DropProgressBarWidget)
				{
					PC->DropProgressBarWidget->SetVisible(false);
				}

				// Trace to see what player is looking at
				FVector CamLoc; FRotator CamRot;
				PC->GetPlayerViewPoint(CamLoc, CamRot);
				const FVector Start = CamLoc;
				const FVector End = Start + CamRot.Vector() * 300.f;
				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
				Params.AddIgnoredActor(C);
				
				if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
				{
					if (AItemPickup* Pickup = Cast<AItemPickup>(Hit.GetActor()))
					{
						// Tap on pickup: add to inventory (if it can be stored)
						UItemDefinition* Def = Pickup->GetItemDef();
						if (Def)
						{
							// Check if item can be stored
							if (!Def->bCanBeStored)
							{
								UE_LOG(LogTemp, Display, TEXT("[Pickup] Item %s cannot be stored (bCanBeStored is false)"), *Def->GetName());
								// Push message to message log
								if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
								{
									MsgLog->PushMessage(FText::FromString(FString::Printf(TEXT("%s cannot be stored"), *Def->DisplayName.ToString())));
								}
							}
							else
							{
								UInventoryComponent* Inv = C->GetInventory();
								if (Inv)
								{
									// Get the full ItemEntry from the pickup to preserve CustomData (like UsesRemaining)
									FItemEntry Entry = Pickup->GetItemEntry();
									if (!Entry.IsValid())
									{
										// Fallback: create entry from Def if GetItemEntry() failed
										Entry.Def = Def;
										Entry.ItemId = FGuid::NewGuid();
									}
									
									// Generate a new GUID if the pickup has an invalid one (all zeros)
									if (!Entry.ItemId.IsValid())
									{
										Entry.ItemId = FGuid::NewGuid();
									}
									
									// If this is a storage container, save its storage data before adding to inventory
									if (UStorageComponent* StorageComp = Pickup->FindComponentByClass<UStorageComponent>())
									{
										StorageSerialization::SaveStorageToItemEntry(StorageComp, Entry);
									}
									
									if (Inv->TryAdd(Entry))
									{
										UE_LOG(LogTemp, Display, TEXT("[Pickup] Added %s to inventory (CustomData: %d entries)"), 
											*Def->GetName(), Entry.CustomData.Num());
										Pickup->Destroy();
										if (PC->bInventoryUIOpen && PC->InventoryScreen)
										{
											PC->InventoryScreen->RefreshInventoryView();
										}
									}
									else
									{
										UE_LOG(LogTemp, Display, TEXT("[Pickup] Inventory full or invalid item; could not add %s"), *Def->GetName());
									}
								}
							}
						}
					}
				}
				else if (C->IsHoldingItem())
				{
					// Tap while holding item and not looking at anything: put item back in inventory
					C->PutHeldItemBack();
				}
			}
			else
			{
				// This was a hold that was canceled (released before timer completed)
				// Just cancel, don't do anything
			}

			// Reset state
			PC->bIsHoldingDrop = false;
			PC->HoldDropTimer = 0.0f;
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetVisible(false);
			}
		}
	}
};

