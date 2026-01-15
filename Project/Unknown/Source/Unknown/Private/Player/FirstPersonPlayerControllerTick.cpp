#include "Player/FirstPersonPlayerController.h"

#include "Player/FirstPersonCharacter.h"
#include "Components/PhysicsInteractionComponent.h"
#include "UI/DropProgressBarWidget.h"
#include "UI/InteractHighlightWidget.h"
#include "UI/InteractInfoWidget.h"
#include "UI/StatBarWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "Player/HungerComponent.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemUseAction.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/StorageSerialization.h"
#include "UI/MessageLogSubsystem.h"

// Helper struct for PlayerTick subsystems
struct FPlayerControllerTickHandler
{
	// Tick movement and hotbar selection
	static void TickMovement(AFirstPersonPlayerController* PC, float DeltaTime)
	{
		if (!PC)
		{
			return;
		}

		// WASD movement from raw keys (simple, avoids complex 2D mapping setup)
		const bool bInventoryOpen = (PC->InventoryScreen && PC->InventoryScreen->IsOpen());
		PC->bInventoryUIOpen = bInventoryOpen;
		FVector2D MoveAxis(0.f, 0.f);
		// Don't process movement if inventory is open or if in computer mode
		if (!bInventoryOpen && !PC->bInComputerMode)
		{
			if (PC->IsInputKeyDown(EKeys::W)) MoveAxis.X += 1.f;
			if (PC->IsInputKeyDown(EKeys::S)) MoveAxis.X -= 1.f;
			if (PC->IsInputKeyDown(EKeys::D)) MoveAxis.Y += 1.f;
			if (PC->IsInputKeyDown(EKeys::A)) MoveAxis.Y -= 1.f;
		}

		if (!MoveAxis.IsNearlyZero())
		{
			if (APawn* P = PC->GetPawn())
			{
				const FRotator YawRot(0.f, PC->GetControlRotation().Yaw, 0.f);
				const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
				const FVector Rt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
				P->AddMovementInput(Fwd, MoveAxis.X);
				P->AddMovementInput(Rt, MoveAxis.Y);
			}
		}

		// Number keys 1-9 select hotbar slots on just-press
		if (!bInventoryOpen)
		{
			if (AFirstPersonCharacter* CharForHotbar = Cast<AFirstPersonCharacter>(PC->GetPawn()))
			{
				if (PC->WasInputKeyJustPressed(EKeys::One)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 0")); CharForHotbar->SelectHotbarSlot(0); }
				if (PC->WasInputKeyJustPressed(EKeys::Two)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 1")); CharForHotbar->SelectHotbarSlot(1); }
				if (PC->WasInputKeyJustPressed(EKeys::Three)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 2")); CharForHotbar->SelectHotbarSlot(2); }
				if (PC->WasInputKeyJustPressed(EKeys::Four)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 3")); CharForHotbar->SelectHotbarSlot(3); }
				if (PC->WasInputKeyJustPressed(EKeys::Five)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 4")); CharForHotbar->SelectHotbarSlot(4); }
				if (PC->WasInputKeyJustPressed(EKeys::Six)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 5")); CharForHotbar->SelectHotbarSlot(5); }
				if (PC->WasInputKeyJustPressed(EKeys::Seven)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 6")); CharForHotbar->SelectHotbarSlot(6); }
				if (PC->WasInputKeyJustPressed(EKeys::Eight)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 7")); CharForHotbar->SelectHotbarSlot(7); }
				if (PC->WasInputKeyJustPressed(EKeys::Nine)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 8")); CharForHotbar->SelectHotbarSlot(8); }
			}
		}

		// Toggle inventory screen with Tab (primary) or I (fallback)
		// Don't allow inventory toggle when in computer mode
		if (!PC->bInComputerMode && (PC->WasInputKeyJustPressed(EKeys::Tab) || PC->WasInputKeyJustPressed(EKeys::I)))
		{
			PC->ToggleInventory();
		}
	}

	// Tick hold-to-drop progress bar
	static void TickHoldToDrop(AFirstPersonPlayerController* PC, float DeltaTime)
	{
		if (!PC || !PC->bIsHoldingDrop || PC->bInstantActionExecuted || PC->bInventoryUIOpen)
		{
			return;
		}

		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C)
		{
			PC->bIsHoldingDrop = false;
			PC->HoldDropTimer = 0.0f;
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetVisible(false);
			}
			return;
		}

		// Trace to see what player is looking at
		FVector CamLoc; FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector Start = CamLoc;
		const FVector End = Start + CamRot.Vector() * 300.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
		Params.AddIgnoredActor(C);
		
		bool bLookingAtPickup = false;
		AItemPickup* TargetPickup = nullptr;
		if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
		{
			TargetPickup = Cast<AItemPickup>(Hit.GetActor());
			bLookingAtPickup = (TargetPickup != nullptr);
		}

		// Update timer
		PC->HoldDropTimer += DeltaTime;
		
		// Only show progress bar after tap threshold (0.25s) has passed
		const float TapThreshold = 0.25f;
		if (PC->HoldDropTimer >= TapThreshold)
		{
			if (PC->DropProgressBarWidget && !PC->DropProgressBarWidget->GetIsVisible())
			{
				PC->DropProgressBarWidget->SetVisible(true);
			}
			
			// Calculate progress from tap threshold onwards (0.25s to 0.75s = 0.5s range)
			const float EffectiveHoldTime = PC->HoldDropTimer - TapThreshold;
			const float EffectiveHoldDuration = PC->HoldDropDuration - TapThreshold;
			const float Progress = FMath::Clamp(EffectiveHoldTime / EffectiveHoldDuration, 0.0f, 1.0f);
			
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetProgress(Progress);
			}
		}

		// Check if we've held long enough (total time including tap threshold)
		if (PC->HoldDropTimer >= PC->HoldDropDuration)
		{
			if (bLookingAtPickup && TargetPickup)
			{
				// Hold the pickup item
				FItemEntry PickupEntry = TargetPickup->GetItemEntry();
				if (PickupEntry.IsValid())
				{
					// Generate a new GUID if the pickup has an invalid one (all zeros)
					if (!PickupEntry.ItemId.IsValid())
					{
						PickupEntry.ItemId = FGuid::NewGuid();
					}
					
					// If already holding an item, store it first
					if (C->IsHoldingItem())
					{
						C->PutHeldItemBack();
					}
					
					// Check if item can be stored
					bool bCanBeStored = PickupEntry.Def && PickupEntry.Def->bCanBeStored;
					
					UInventoryComponent* Inv = C->GetInventory();
					
					// If this is a storage container, save its storage data before holding
					if (UStorageComponent* StorageComp = TargetPickup->FindComponentByClass<UStorageComponent>())
					{
						StorageSerialization::SaveStorageToItemEntry(StorageComp, PickupEntry);
					}
					
					// If item cannot be stored, hold it directly without adding to inventory
					if (!bCanBeStored)
					{
						// Hold directly from pickup (no inventory needed)
						if (C->HoldItem(PickupEntry))
						{
							UE_LOG(LogTemp, Display, TEXT("[Pickup] Holding %s directly (cannot be stored)"), *GetNameSafe(PickupEntry.Def));
							TargetPickup->Destroy();
							PC->bInstantActionExecuted = true;
						}
					}
					// If item can be stored, add to inventory first, then hold it
					else if (Inv && Inv->TryAdd(PickupEntry))
					{
						// Now hold it from inventory
						if (C->HoldItem(PickupEntry))
						{
							UE_LOG(LogTemp, Display, TEXT("[Pickup] Holding %s after progress"), *GetNameSafe(PickupEntry.Def));
							TargetPickup->Destroy();
							PC->bInstantActionExecuted = true;
						}
						else
						{
							// Failed to hold, remove from inventory
							Inv->RemoveById(PickupEntry.ItemId);
						}
					}
				}
			}
			else if (C->IsHoldingItem())
			{
				// Drop held item at location
				C->DropHeldItemAtLocation();
				PC->bInstantActionExecuted = true;
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

	// Tick hold-to-use progress bar
	static void TickHoldToUse(AFirstPersonPlayerController* PC, float DeltaTime)
	{
		if (!PC || !PC->bIsHoldingUse || PC->bInstantUseExecuted || PC->bInventoryUIOpen)
		{
			return;
		}

		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn());
		if (!C)
		{
			PC->bIsHoldingUse = false;
			PC->HoldUseTimer = 0.0f;
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetVisible(false);
			}
			return;
		}

		// Trace to see what player is looking at
		FVector CamLoc; FRotator CamRot;
		PC->GetPlayerViewPoint(CamLoc, CamRot);
		const FVector Start = CamLoc;
		const FVector End = Start + CamRot.Vector() * 300.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
		Params.AddIgnoredActor(C);
		
		bool bLookingAtPickup = false;
		AItemPickup* TargetPickup = nullptr;
		if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
		{
			TargetPickup = Cast<AItemPickup>(Hit.GetActor());
			bLookingAtPickup = (TargetPickup != nullptr);
		}

		// Update timer
		PC->HoldUseTimer += DeltaTime;
		
		// Only show progress bar after tap threshold (0.25s) has passed
		const float TapThreshold = 0.25f;
		if (PC->HoldUseTimer >= TapThreshold)
		{
			if (PC->DropProgressBarWidget && !PC->DropProgressBarWidget->GetIsVisible())
			{
				PC->DropProgressBarWidget->SetVisible(true);
			}
			
			// Calculate progress from tap threshold onwards (0.25s to 0.75s = 0.5s range)
			const float EffectiveHoldTime = PC->HoldUseTimer - TapThreshold;
			const float EffectiveHoldDuration = PC->HoldDropDuration - TapThreshold;
			const float Progress = FMath::Clamp(EffectiveHoldTime / EffectiveHoldDuration, 0.0f, 1.0f);
			
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetProgress(Progress);
			}
		}

		// Check if we've held long enough (total time including tap threshold)
		if (PC->HoldUseTimer >= PC->HoldDropDuration)
		{
			if (bLookingAtPickup && TargetPickup)
			{
				// Try to use the pickup item
				FItemEntry PickupEntry = TargetPickup->GetItemEntry();
				if (PickupEntry.IsValid() && PickupEntry.Def)
				{
					// Check if item has a USE action
					if (PickupEntry.Def->DefaultUseAction)
					{
						// Create instance of use action and execute it
						UItemUseAction* UseAction = NewObject<UItemUseAction>(PC, PickupEntry.Def->DefaultUseAction);
						if (UseAction)
						{
							// Pass the pickup actor so the use action can update/replace it in the world
							if (UseAction->Execute(C, PickupEntry, TargetPickup))
							{
								UE_LOG(LogTemp, Display, TEXT("[Interact] Used %s"), *GetNameSafe(PickupEntry.Def));
								// Use action succeeded - item may have been consumed or modified
								// Don't destroy pickup here - let the use action handle it if needed
								PC->bInstantUseExecuted = true;
							}
							else
							{
								UE_LOG(LogTemp, Display, TEXT("[Interact] Use action failed for %s"), *GetNameSafe(PickupEntry.Def));
							}
						}
					}
					else
					{
						// No USE action - show "Cannot use" message
						if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
						{
							MsgLog->PushMessage(FText::FromString(TEXT("Cannot use")));
						}
						UE_LOG(LogTemp, Display, TEXT("[Interact] Item %s has no USE action"), *GetNameSafe(PickupEntry.Def));
					}
				}
			}
			else
			{
				// Not looking at a pickup - nothing to use
				if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
				{
					MsgLog->PushMessage(FText::FromString(TEXT("Cannot use")));
				}
			}
			
			// Reset state
			PC->bIsHoldingUse = false;
			PC->bInstantUseExecuted = true; // Prevent OnInteractReleased from doing anything
			PC->HoldUseTimer = 0.0f;
			if (PC->DropProgressBarWidget)
			{
				PC->DropProgressBarWidget->SetVisible(false);
			}
		}
	}

	// Tick physics interaction updates
	static void TickPhysicsInteraction(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		// Update hold target for physics interaction so held object follows view
		UPhysicsInteractionComponent* PIC = nullptr;
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			PIC = C->FindComponentByClass<UPhysicsInteractionComponent>();
			if (PIC && PIC->IsHolding())
			{
				FVector CamLoc; FRotator CamRot;
				PC->GetPlayerViewPoint(CamLoc, CamRot);
				PIC->UpdateHoldTarget(CamLoc, CamRot.Vector());
			}
		}
	}

	// Tick interact highlight computation
	static void TickInteractHighlight(AFirstPersonPlayerController* PC)
	{
		if (!PC || !PC->InteractHighlightWidget)
		{
			return;
		}

		const bool bInventoryOpen = (PC->InventoryScreen && PC->InventoryScreen->IsOpen());
		UPhysicsInteractionComponent* PIC = nullptr;
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			PIC = C->FindComponentByClass<UPhysicsInteractionComponent>();
		}

		// Hide highlight entirely while inventory UI is open
		if (bInventoryOpen)
		{
			PC->InteractHighlightWidget->SetVisible(false);
			if (PC->InteractInfoWidget)
			{
				PC->InteractInfoWidget->SetVisible(false);
			}
		}
		// Suppress highlight while holding a physics object
		else if (PIC && PIC->IsHolding())
		{
			PC->InteractHighlightWidget->SetVisible(false);
			if (PC->InteractInfoWidget)
			{
				PC->InteractInfoWidget->SetVisible(false);
			}
		}
		else
		{
			bool bHasTarget = false;
			FVector2D TL(0, 0), BR(0, 0);

			FVector CamLoc; FRotator CamRot;
			PC->GetPlayerViewPoint(CamLoc, CamRot);
			const FVector Dir = CamRot.Vector();
			const float MaxDist = (PIC ? PIC->GetMaxPickupDistance() : 300.f);
			const FVector Start = CamLoc;
			const FVector End = Start + Dir * MaxDist;

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractHighlight), /*bTraceComplex*/ false);
			if (APawn* PawnToIgnore = PC->GetPawn()) { Params.AddIgnoredActor(PawnToIgnore); }

			if (PC->GetWorld() && PC->GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
			{
				UPrimitiveComponent* Prim = Hit.GetComponent();
				if (Prim)
				{
					const FBoxSphereBounds& B = Prim->Bounds;
					const FVector Ctr = B.Origin;
					const FVector Ext = B.BoxExtent;
					FVector Corners[8] = {
						Ctr + FVector(-Ext.X, -Ext.Y, -Ext.Z),
						Ctr + FVector( Ext.X, -Ext.Y, -Ext.Z),
						Ctr + FVector(-Ext.X,  Ext.Y, -Ext.Z),
						Ctr + FVector( Ext.X,  Ext.Y, -Ext.Z),
						Ctr + FVector(-Ext.X, -Ext.Y,  Ext.Z),
						Ctr + FVector( Ext.X, -Ext.Y,  Ext.Z),
						Ctr + FVector(-Ext.X,  Ext.Y,  Ext.Z),
						Ctr + FVector( Ext.X,  Ext.Y,  Ext.Z)
					};

					FVector2D MinPt(FLT_MAX, FLT_MAX), MaxPt(-FLT_MAX, -FLT_MAX);
					bool bAnyProjected = false;
					for (int32 i = 0; i < 8; ++i)
					{
						FVector2D ScreenPt;
						// Use UWidgetLayoutLibrary to get DPI-aware, viewport-relative widget coordinates
						if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(PC, Corners[i], ScreenPt, /*bPlayerViewportRelative*/ true))
						{
							bAnyProjected = true;
							MinPt.X = FMath::Min(MinPt.X, ScreenPt.X);
							MinPt.Y = FMath::Min(MinPt.Y, ScreenPt.Y);
							MaxPt.X = FMath::Max(MaxPt.X, ScreenPt.X);
							MaxPt.Y = FMath::Max(MaxPt.Y, ScreenPt.Y);
						}
					}

					if (bAnyProjected)
					{
						// Small padding
						const float Pad = 6.f;
						TL = MinPt - FVector2D(Pad, Pad);
						BR = MaxPt + FVector2D(Pad, Pad);
						bHasTarget = true;
					}
				}
			}

			if (bHasTarget)
			{
				PC->InteractHighlightWidget->SetHighlightRect(TL, BR);
				PC->InteractHighlightWidget->SetVisible(true);

				// Update interact info widget if we're looking at an item pickup
				if (PC->InteractInfoWidget)
				{
					AActor* HitActor = Hit.GetActor();
					AItemPickup* ItemPickup = Cast<AItemPickup>(HitActor);
					
					if (ItemPickup && ItemPickup->GetItemDef())
					{
						// Set the item data
						PC->InteractInfoWidget->SetInteractableItem(ItemPickup);
						
						// Get key names and set them
						FString InteractKeyName = PC->GetKeyDisplayName(PC->InteractAction);
						FString PickupKeyName = PC->GetKeyDisplayName(PC->PickupAction);
						PC->InteractInfoWidget->SetKeybindings(InteractKeyName, PickupKeyName);
						
						// Set position and visibility
						PC->InteractInfoWidget->SetPosition(TL, BR);
						PC->InteractInfoWidget->SetVisible(true);
					}
					else
					{
						PC->InteractInfoWidget->SetVisible(false);
					}
				}
			}
			else
			{
				PC->InteractHighlightWidget->SetVisible(false);
				if (PC->InteractInfoWidget)
				{
					PC->InteractInfoWidget->SetVisible(false);
				}
			}
		}
	}

	// Tick hunger bar updates
	static void TickHungerBar(AFirstPersonPlayerController* PC)
	{
		if (!PC || !PC->HungerBarWidget)
		{
			return;
		}

		if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(PC->GetPawn()))
		{
			if (UHungerComponent* HungerComp = Char->GetHunger())
			{
				const float CurrentHunger = HungerComp->GetCurrentHunger();
				const float MaxHunger = HungerComp->GetMaxHunger();
				PC->HungerBarWidget->SetValue(CurrentHunger, MaxHunger);
			}
		}
	}
};

