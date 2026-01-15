#include "Player/FirstPersonPlayerController.h"

#include "Player/FirstPersonCharacter.h"
#include "Player/HungerComponent.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/StorageSerialization.h"
#include "Inventory/InventoryComponent.h"
#include "Save/SaveSystemHelpers.h"
#include "Components/SaveableActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UI/HotbarWidget.h"
#include "UI/LoadingFadeWidget.h"
#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

// Include helper module implementations (must be before first use)
#include "FirstPersonPlayerControllerInputSetup.cpp"
#include "FirstPersonPlayerControllerUI.cpp"
#include "FirstPersonPlayerControllerInteraction.cpp"
#include "FirstPersonPlayerControllerTick.cpp"
#include "UI/PauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AFirstPersonPlayerController::AFirstPersonPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// Delegate input asset creation to helper
	FPlayerControllerInputSetup::InitializeInputAssets(this);
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Ensure input assets exist and are configured
	FPlayerControllerInputSetup::EnsureInputAssets(this);
	
	// Apply mapping context to enhanced input subsystem
	FPlayerControllerInputSetup::ApplyMappingContext(this);

	// Configure camera pitch limits (clamp handled by PlayerCameraManager)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -89.f;
		PlayerCameraManager->ViewPitchMax = 89.f;
	}

	// Initialize all widgets
	FPlayerControllerUIManager::InitializeWidgets(this);

	// Lock input to game only and hide cursor for FPS
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
	
	// Check for pending new game save (if we just loaded MainMap from main menu)
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				// Check after a short delay to ensure level is fully loaded
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, [this, SaveSystem]()
				{
					SaveSystem->CheckAndCreatePendingNewGameSave();
					
					// Fade in after new game save is created
					if (LoadingFadeWidget)
					{
						// Add a small delay to ensure everything is visually settled
						if (UWorld* World = GetWorld())
						{
							FTimerHandle FadeInTimer;
							World->GetTimerManager().SetTimer(FadeInTimer, [this]()
							{
								if (LoadingFadeWidget)
								{
									// Fade in smoothly (1.0 second) to reveal the new game
									LoadingFadeWidget->FadeIn(1.0f);
								}
							}, 0.2f, false); // 0.2 second delay before fading in
						}
					}
				}, 0.5f, false);
			}
		}
	}
	
	// Ensure fade widget is black if we're doing a level transition restore
	// This prevents flash of level before fade-in
	FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
	if (UGameplayStatics::DoesSaveGameExist(TempSlotName, 0))
	{
		if (LoadingFadeWidget)
		{
			LoadingFadeWidget->SetOpacity(1.0f);
			UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Ensured fade widget is black at BeginPlay for level transition"));
		}
	}
	
	// Check for position restore after level load
	// This happens when loading a save that required a level transition
	RestorePlayerPositionAfterLevelLoad();
}

void AFirstPersonPlayerController::RestorePlayerPositionAfterLevelLoad()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check if there's a temp save with position data
	FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
	if (UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(TempSlotName, 0)))
	{
		// CRITICAL: Set CurrentSaveData immediately so dimension loading can find save data
		// This must be done BEFORE waiting for the timer, otherwise cartridges in sockets
		// will trigger dimension loading before save data is available
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				SaveSystem->CurrentSaveData = TempSave;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Set CurrentSaveData from temp save for dimension restoration"));
			}
		}

		// Wait a bit for the player to fully spawn and level to be ready
		FTimerHandle RestoreTimer;
		World->GetTimerManager().SetTimer(RestoreTimer, [this, World, TempSlotName]()
		{
			// Load the temporary save to restore all game state
			if (UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(TempSlotName, 0)))
			{
				if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
				{
					if (AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn))
					{
						// Create a set of baseline GUIDs for quick lookup
						TSet<FGuid> BaselineActorIds;
						for (const FGuid& BaselineId : TempSave->BaselineActorIds)
						{
							BaselineActorIds.Add(BaselineId);
						}
						
						// === STEP 1: Early GUID Restoration (Before BeginPlay) ===
						// Restore GUIDs to actors using metadata matching before they generate new GUIDs
						// This must happen as early as possible to prevent GUID regeneration
						// Track which actors have already been matched to prevent duplicate matches
						TSet<AActor*> MatchedActors;
						int32 EarlyGUIDRestoredCount = 0;
						
						// First pass: Restore GUIDs for existing actors (bExists = true)
						for (const FActorStateSaveData& ActorState : TempSave->ActorStates)
						{
							// Skip removed actors in first pass
							if (!ActorState.bExists || ActorState.ActorClassPath.IsEmpty())
							{
								continue;
							}
							
							// Only match actors that are in baseline (existing level actors)
							if (!BaselineActorIds.Contains(ActorState.ActorId))
							{
								continue; // Skip new objects - they'll be spawned in STEP 3
							}
							
							// Try to find actor by GUID first
							AActor* Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
							
							// If not found, try metadata fallback matching
							if (!Actor)
							{
								// Find actor by metadata, but skip if already matched
								int32 CandidatesChecked = 0;
								int32 CandidatesWithComponent = 0;
								int32 CandidatesWithValidGUID = 0;
								
								for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
								{
									AActor* Candidate = *ActorIterator;
									if (!Candidate || MatchedActors.Contains(Candidate))
									{
										continue;
									}
									
									// Check if class path matches
									FString CandidateClassPath = Candidate->GetClass()->GetPathName();
									if (CandidateClassPath != ActorState.ActorClassPath)
									{
										continue;
									}
									CandidatesChecked++;
									
									
									// Check if this actor has a SaveableActorComponent
									USaveableActorComponent* SaveableComp = Candidate->FindComponentByClass<USaveableActorComponent>();
									if (!SaveableComp)
									{
										continue;
									}
									CandidatesWithComponent++;
									
									// If actor already has a valid GUID that matches, we're done
									if (SaveableComp->GetPersistentId().IsValid() && SaveableComp->GetPersistentId() == ActorState.ActorId)
									{
										// Already matched - mark and break
										Actor = Candidate;
										MatchedActors.Add(Candidate);
										break;
									}
									
									// If actor has a different GUID, we'll still try to match by transform
									// (the GUID might have been regenerated on level load)
									if (SaveableComp->GetPersistentId().IsValid() && SaveableComp->GetPersistentId() != ActorState.ActorId)
									{
										CandidatesWithValidGUID++;
										// Continue to check transform match - if it matches, we'll update the GUID
									}
									
									// Get transform to compare (use OriginalTransform if set, otherwise use current transform)
									FTransform TransformToCompare = SaveableComp->OriginalTransform;
									if (TransformToCompare.GetLocation().IsNearlyZero() && 
										TransformToCompare.GetRotation().IsIdentity() && 
										TransformToCompare.GetScale3D().IsNearlyZero())
									{
										// OriginalTransform not set yet - use current actor transform
										TransformToCompare = Candidate->GetActorTransform();
										// Also set it for future use
										SaveableComp->OriginalTransform = TransformToCompare;
									}
									
									// Check if transform matches (within tolerance)
									FVector LocationDelta = TransformToCompare.GetLocation() - ActorState.OriginalSpawnTransform.GetLocation();
									if (LocationDelta.Size() <= 10.0f)  // 10cm tolerance
									{
										// Check rotation (within tolerance)
										FRotator RotationDelta = (TransformToCompare.GetRotation() * ActorState.OriginalSpawnTransform.GetRotation().Inverse()).Rotator();
										if (FMath::Abs(RotationDelta.Pitch) <= 10.0f &&
											FMath::Abs(RotationDelta.Yaw) <= 10.0f &&
											FMath::Abs(RotationDelta.Roll) <= 10.0f)
										{
											// Match found - restore the GUID and ensure OriginalTransform is set
											SaveableComp->SetPersistentId(ActorState.ActorId);
											if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero())
											{
												SaveableComp->OriginalTransform = ActorState.OriginalSpawnTransform;
											}
											Actor = Candidate;
											MatchedActors.Add(Candidate);
											EarlyGUIDRestoredCount++;
											break;
										}
										else
										{
										}
									}
									else
									{
									}
								}
							}
							else
							{
								// Actor already has correct GUID - mark as matched
								MatchedActors.Add(Actor);
							}
						}
						
						// Second pass: Restore GUIDs for removed actors (bExists = false) so we can find them to destroy
						int32 RemovedActorGUIDRestoredCount = 0;
						for (const FActorStateSaveData& ActorState : TempSave->ActorStates)
						{
							// Only handle removed actors
							if (ActorState.bExists || ActorState.ActorClassPath.IsEmpty() || ActorState.OriginalSpawnTransform.GetLocation().IsNearlyZero())
							{
								continue;
							}
							
							// Only match actors that are in baseline (they should exist in the level)
							if (!BaselineActorIds.Contains(ActorState.ActorId))
							{
								continue; // Not in baseline - skip
							}
							
							// Find actor by metadata to restore its GUID
							for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
							{
								AActor* Candidate = *ActorIterator;
								if (!Candidate || MatchedActors.Contains(Candidate))
								{
									continue;
								}
								
								// Check if class path matches
								FString CandidateClassPath = Candidate->GetClass()->GetPathName();
								if (CandidateClassPath != ActorState.ActorClassPath)
								{
									continue;
								}
								
								// Check if this actor has a SaveableActorComponent
								USaveableActorComponent* SaveableComp = Candidate->FindComponentByClass<USaveableActorComponent>();
								if (!SaveableComp)
								{
									continue;
								}
								
								// Get transform to compare
								FTransform TransformToCompare = SaveableComp->OriginalTransform;
								if (TransformToCompare.GetLocation().IsNearlyZero())
								{
									TransformToCompare = Candidate->GetActorTransform();
									SaveableComp->OriginalTransform = TransformToCompare;
								}
								
								// Check if transform matches (within tolerance)
								FVector LocationDelta = TransformToCompare.GetLocation() - ActorState.OriginalSpawnTransform.GetLocation();
								if (LocationDelta.Size() <= 10.0f)
								{
									// Check rotation
									FRotator RotationDelta = (TransformToCompare.GetRotation() * ActorState.OriginalSpawnTransform.GetRotation().Inverse()).Rotator();
									if (FMath::Abs(RotationDelta.Pitch) <= 10.0f &&
										FMath::Abs(RotationDelta.Yaw) <= 10.0f &&
										FMath::Abs(RotationDelta.Roll) <= 10.0f)
									{
										// Match found - restore the GUID for removal
										SaveableComp->SetPersistentId(ActorState.ActorId);
										MatchedActors.Add(Candidate);
										RemovedActorGUIDRestoredCount++;
										break;
									}
								}
							}
						}
						
						// === STEP 2: Destroy removed actors ===
						// Destroy actors that are marked as not existing in the save
						int32 RemovedCount = 0;
						for (const FActorStateSaveData& ActorState : TempSave->ActorStates)
						{
							if (!ActorState.bExists)
							{
								// Try to find by GUID first (GUID should have been restored in early restoration)
								AActor* ActorToRemove = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
								
								// If not found by GUID, try metadata fallback
								if (!ActorToRemove && !ActorState.ActorClassPath.IsEmpty() && !ActorState.OriginalSpawnTransform.GetLocation().IsNearlyZero())
								{
									ActorToRemove = USaveableActorComponent::FindActorByGuidOrMetadata(
										World,
										ActorState.ActorId,
										ActorState.ActorClassPath,
										ActorState.OriginalSpawnTransform,
										10.0f
									);
								}
								
								if (ActorToRemove)
								{
									ActorToRemove->Destroy();
									RemovedCount++;
								}
								else
								{
								}
							}
						}

						// Restore position (unless player was in dimension - will be restored after dimension loads)
						bool bPlayerWasInDimension = TempSave->LoadedDimensionInstanceId.IsValid();
						if (!bPlayerWasInDimension)
						{
							FVector RestoreLocation = TempSave->PlayerData.Location.IsNearlyZero() ? 
								TempSave->PlayerLocation : TempSave->PlayerData.Location;
							FRotator RestoreRotation = TempSave->PlayerData.Rotation.IsZero() ? 
								TempSave->PlayerRotation : TempSave->PlayerData.Rotation;
							
							PlayerCharacter->SetActorLocation(RestoreLocation);
							PlayerCharacter->SetActorRotation(RestoreRotation);
							
							// Also set controller rotation for first-person camera
							SetControlRotation(RestoreRotation);
						}
						else
						{
							UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Player was in dimension when saving - position will be restored after dimension loads"));
						}
						

						// Restore player's dimension instance ID (which dimension the player is currently in)
						// This must be done BEFORE dimension loading so items dropped get the correct dimension ID
						if (UGameInstance* GameInstance = World->GetGameInstance())
						{
							if (UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>())
							{
								if (TempSave->PlayerData.PlayerDimensionInstanceId.IsValid())
								{
									DimensionManager->SetPlayerDimensionInstanceId(TempSave->PlayerData.PlayerDimensionInstanceId);
									UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored player dimension instance ID: %s"), 
										*TempSave->PlayerData.PlayerDimensionInstanceId.ToString());
								}
							}
						}
						
						// Restore hunger
						if (UHungerComponent* HungerComp = PlayerCharacter->GetHunger())
						{
							float SavedHunger = TempSave->PlayerData.CurrentHunger;
							float CurrentHunger = HungerComp->GetCurrentHunger();
							float HungerDelta = SavedHunger - CurrentHunger;
							
							// Use reflection to set CurrentHunger directly since RestoreHunger adds to current value
							FProperty* CurrentHungerProp = UHungerComponent::StaticClass()->FindPropertyByName(TEXT("CurrentHunger"));
							if (CurrentHungerProp)
							{
								float* CurrentHungerPtr = CurrentHungerProp->ContainerPtrToValuePtr<float>(HungerComp);
								if (CurrentHungerPtr)
								{
									*CurrentHungerPtr = SavedHunger;
									HungerComp->OnHungerChanged.Broadcast(SavedHunger, TempSave->PlayerData.MaxHunger);
								}
							}
							else
							{
								// Fallback: use RestoreHunger if reflection fails
								HungerComp->RestoreHunger(HungerDelta);
							}
							
						}

						// Check if new save data exists
						bool bHasNewSaveData = !TempSave->InventoryData.SerializedEntries.IsEmpty() || 
							TempSave->HotbarData.AssignedItemPaths.Num() > 0 ||
							TempSave->EquipmentData.SerializedEquippedItems.Num() > 0;
						

						// Restore inventory
						if (UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory())
						{
							TArray<FItemEntry> RestoredEntries = StorageSerialization::DeserializeStorageEntries(
								TempSave->InventoryData.SerializedEntries);
							
							TArray<FGuid> ItemIdsToRemove;
							for (const FItemEntry& Entry : InventoryComp->GetEntries())
							{
								ItemIdsToRemove.Add(Entry.ItemId);
							}
							for (const FGuid& ItemId : ItemIdsToRemove)
							{
								InventoryComp->RemoveById(ItemId);
							}

							int32 AddedCount = 0;
							for (const FItemEntry& Entry : RestoredEntries)
							{
								if (InventoryComp->TryAdd(Entry))
								{
									AddedCount++;
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to add inventory entry: %s"), 
										Entry.Def ? *Entry.Def->GetPathName() : TEXT("NULL"));
								}
							}
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Player character has no InventoryComponent"));
						}

						// Restore hotbar
						if (UHotbarComponent* HotbarComp = PlayerCharacter->GetHotbar())
						{
							
							for (int32 i = 0; i < HotbarComp->GetNumSlots(); ++i)
							{
								HotbarComp->ClearSlot(i);
							}

							int32 AssignedCount = 0;
							for (int32 i = 0; i < TempSave->HotbarData.AssignedItemPaths.Num() && 
								i < HotbarComp->GetNumSlots(); ++i)
							{
								const FString& ItemPath = TempSave->HotbarData.AssignedItemPaths[i];
								if (!ItemPath.IsEmpty())
								{
									UItemDefinition* ItemDef = LoadObject<UItemDefinition>(nullptr, *ItemPath);
									if (ItemDef)
									{
										HotbarComp->AssignSlot(i, ItemDef);
										AssignedCount++;
									}
									else
									{
										UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to load ItemDefinition from path: %s"), *ItemPath);
									}
								}
							}

							if (TempSave->HotbarData.ActiveIndex != INDEX_NONE && 
								TempSave->HotbarData.ActiveIndex >= 0 && 
								TempSave->HotbarData.ActiveIndex < HotbarComp->GetNumSlots())
							{
								HotbarComp->SelectSlot(TempSave->HotbarData.ActiveIndex, PlayerCharacter->GetInventory());
							}
						}

						// Restore equipment
						if (UEquipmentComponent* EquipmentComp = PlayerCharacter->GetEquipment())
						{
							for (const FString& SerializedEquipment : TempSave->EquipmentData.SerializedEquippedItems)
							{
								EEquipmentSlot Slot;
								FItemEntry Entry;
								if (SaveSystemHelpers::DeserializeEquipmentSlot(SerializedEquipment, Slot, Entry))
								{
									bool bItemInInventory = false;
									if (UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory())
									{
										for (const FItemEntry& InvEntry : InventoryComp->GetEntries())
										{
											if (InvEntry.ItemId == Entry.ItemId)
											{
												bItemInInventory = true;
												break;
											}
										}

										if (!bItemInInventory)
										{
											InventoryComp->TryAdd(Entry);
										}

										FText ErrorText;
										EquipmentComp->EquipFromInventory(Entry.ItemId, ErrorText);
									}
								}
							}
						}

						// === STEP 3: Two-Phase Actor Matching and State Restoration ===
						// Track which actors have already been matched to prevent duplicate matches
						TSet<AActor*> RestoredActors;
						int32 ActorRestoredCount = 0;
						int32 ActorNotFoundCount = 0;
						int32 NewObjectSpawnedCount = 0;
						int32 PhysicsRestoredCount = 0;
						int32 StorageRestoredCount = 0;
						int32 MetadataFallbackCount = 0;
						
						for (const FActorStateSaveData& ActorState : TempSave->ActorStates)
						{
							if (!ActorState.bExists)
							{
								continue; // Skip actors that were marked as not existing (already handled in STEP 2)
							}
							
							// === Phase 1: Primary Matching (By GUID) ===
							AActor* Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
							
							// Check if this actor was already matched to a different saved state
							if (Actor && RestoredActors.Contains(Actor))
							{
								// This actor was already matched - skip this saved state
								UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Actor %s already matched to a different saved state, skipping GUID %s"), 
									*Actor->GetName(), *ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
								ActorNotFoundCount++;
								continue;
							}
							
							// === Phase 2: Fallback Matching (By Metadata) ===
							if (!Actor)
							{
								bool bIsInBaseline = BaselineActorIds.Contains(ActorState.ActorId);
								
								if (bIsInBaseline)
								{
									// Actor is in baseline but not found by GUID - try metadata fallback
									// But only if it wasn't already matched in early GUID restoration
									if (!ActorState.ActorClassPath.IsEmpty())
									{
										// Try to find by GUID again (in case it was restored in early step)
										Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
										
										if (!Actor)
										{
											// Still not found - try metadata fallback
											Actor = USaveableActorComponent::FindActorByGuidOrMetadata(
												World,
												ActorState.ActorId,
												ActorState.ActorClassPath,
												ActorState.OriginalSpawnTransform,
												10.0f  // 10cm tolerance
											);
											
											if (Actor)
											{
												// Check if this actor was already matched
												if (RestoredActors.Contains(Actor))
												{
													// Actor already matched - skip
													UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Actor %s already matched, skipping metadata fallback for GUID %s"), 
														*Actor->GetName(), *ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
													Actor = nullptr;
												}
												else
												{
													MetadataFallbackCount++;
													UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Used metadata fallback to match actor %s (GUID: %s) - GUID was regenerated"), 
														*Actor->GetName(), *ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
												}
											}
										}
									}
									
									if (!Actor)
									{
										ActorNotFoundCount++;
										UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Actor with ID %s not found in world (was in baseline, class: %s)"), 
											*ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces), *ActorState.ActorClassPath);
										continue;
									}
								}
								else
								{
									// === New Object Handling ===
									// This is a new object that was added after baseline (e.g., dropped item)
									if (ActorState.bIsNewObject && !ActorState.SpawnActorClassPath.IsEmpty())
									{
										// Load the actor class
										UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorState.SpawnActorClassPath);
										if (ActorClass)
										{
											// Spawn the actor at saved location
											FTransform SpawnTransform(ActorState.Rotation, ActorState.Location, ActorState.Scale);
											FActorSpawnParameters SpawnParams;
											SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
											
											Actor = World->SpawnActor<AActor>(ActorClass, SpawnTransform, SpawnParams);
											
											if (Actor)
											{
												// Set GUID before BeginPlay is called
												if (USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>())
												{
													SaveableComp->SetPersistentId(ActorState.ActorId);
													SaveableComp->OriginalTransform = ActorState.OriginalSpawnTransform;
												}
												
												// If it's an ItemPickup, restore ItemDefinition and ItemEntry
												if (AItemPickup* ItemPickup = Cast<AItemPickup>(Actor))
												{
													if (!ActorState.ItemDefinitionPath.IsEmpty())
													{
														UItemDefinition* ItemDef = LoadObject<UItemDefinition>(nullptr, *ActorState.ItemDefinitionPath);
														if (ItemDef)
														{
															if (!ActorState.SerializedItemEntry.IsEmpty())
															{
																FItemEntry ItemEntry;
																if (SaveSystemHelpers::DeserializeItemEntry(ActorState.SerializedItemEntry, ItemEntry))
																{
																	ItemPickup->SetItemEntry(ItemEntry);
																}
																else
																{
																	ItemPickup->SetItemDef(ItemDef);
																}
															}
															else
															{
																ItemPickup->SetItemDef(ItemDef);
															}
														}
													}
												}
												
												NewObjectSpawnedCount++;
											}
											else
											{
												UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to spawn new object (class: %s)"), *ActorState.SpawnActorClassPath);
												ActorNotFoundCount++;
												continue;
											}
										}
										else
										{
											UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to load actor class for new object: %s"), *ActorState.SpawnActorClassPath);
											ActorNotFoundCount++;
											continue;
										}
									}
									else
									{
										UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] New object with GUID %s cannot be spawned (missing spawn data)"), 
											*ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
										ActorNotFoundCount++;
										continue;
									}
								}
							}
							
							if (!Actor)
							{
								continue;
							}
							
							// Mark this actor as matched
							RestoredActors.Add(Actor);

							// Restore transform (only if bSaveTransform is enabled for this actor)
							USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
							if (!SaveableComp || !SaveableComp->bSaveTransform)
							{
								// Skip transform restoration if disabled
							}
							else
							{
								// Restore transform
								// For physics objects, we need to disable physics temporarily to set location
								UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
								bool bWasSimulatingPhysics = false;
								if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
								{
									bWasSimulatingPhysics = true;
									PrimitiveComp->SetSimulatePhysics(false);
								}
								
								Actor->SetActorLocation(ActorState.Location);
								Actor->SetActorRotation(ActorState.Rotation);
								Actor->SetActorScale3D(ActorState.Scale);
								
								// Re-enable physics if it was enabled
								if (bWasSimulatingPhysics && PrimitiveComp)
								{
									PrimitiveComp->SetSimulatePhysics(true);
								}
							}

							// Restore physics state if applicable
							if (ActorState.bHasPhysics)
							{
								UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
								if (PrimitiveComp)
								{
									if (ActorState.bSimulatePhysics && !PrimitiveComp->IsSimulatingPhysics())
									{
										PrimitiveComp->SetSimulatePhysics(true);
									}

									if (ActorState.LinearVelocity.SizeSquared() > KINDA_SMALL_NUMBER ||
										ActorState.AngularVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
									{
										PrimitiveComp->SetPhysicsLinearVelocity(ActorState.LinearVelocity);
										PrimitiveComp->SetPhysicsAngularVelocityInRadians(ActorState.AngularVelocity);
									}
									PhysicsRestoredCount++;
								}
							}

							// Restore storage state if applicable
							if (ActorState.bHasStorage)
							{
								UStorageComponent* StorageComp = Actor->FindComponentByClass<UStorageComponent>();
								if (StorageComp)
								{
									TArray<FItemEntry> RestoredEntries = StorageSerialization::DeserializeStorageEntries(
										ActorState.SerializedStorageEntries);
									
									TArray<FGuid> ItemIdsToRemove;
									for (const FItemEntry& Entry : StorageComp->GetEntries())
									{
										ItemIdsToRemove.Add(Entry.ItemId);
									}
									
									for (const FGuid& ItemId : ItemIdsToRemove)
									{
										StorageComp->RemoveById(ItemId);
									}

									int32 AddedCount = 0;
									for (const FItemEntry& Entry : RestoredEntries)
									{
										if (StorageComp->TryAdd(Entry))
										{
											AddedCount++;
										}
										else
										{
											UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to add storage entry to actor %s"), *Actor->GetName());
										}
									}

									StorageComp->MaxVolume = ActorState.StorageMaxVolume;
									StorageRestoredCount++;
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Actor %s has bHasStorage=true but no StorageComponent"), *Actor->GetName());
								}
							}

							// Restore ItemEntry data for ItemPickup actors (includes CustomData like UsesRemaining)
							if (AItemPickup* ItemPickup = Cast<AItemPickup>(Actor))
							{
								if (!ActorState.SerializedItemEntry.IsEmpty())
								{
									FItemEntry ItemEntry;
									if (SaveSystemHelpers::DeserializeItemEntry(ActorState.SerializedItemEntry, ItemEntry))
									{
										ItemPickup->SetItemEntry(ItemEntry);
									}
									else
									{
										UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to deserialize ItemEntry for actor %s"), 
											*Actor->GetName());
									}
								}
							}

							ActorRestoredCount++;
						}
						
						// Restore cartridge instance IDs from saved dimension data
						// This ensures cartridges have the correct instance IDs before they trigger dimension loading
						if (UGameInstance* GameInstance = World->GetGameInstance())
						{
							if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
							{
								SaveSystem->RestoreCartridgeInstanceIds(World, TempSave, PlayerCharacter);
								
								// Restore loaded dimension if one was loaded when saving
								// Use a delay to ensure all items and cartridges are fully restored first
								if (TempSave->LoadedDimensionInstanceId.IsValid())
								{
									UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Will restore loaded dimension instance: %s"), 
										*TempSave->LoadedDimensionInstanceId.ToString());
									FTimerHandle RestoreDimensionTimer;
									// Get saved player position for restoration after dimension loads
									FVector SavedPlayerLocation = TempSave->PlayerData.Location.IsNearlyZero() ? 
										TempSave->PlayerLocation : TempSave->PlayerData.Location;
									FRotator SavedPlayerRotation = TempSave->PlayerData.Rotation.IsZero() ? 
										TempSave->PlayerRotation : TempSave->PlayerData.Rotation;
									
									// Fade in callback - only fade in after dimension loads and player position is restored
									TFunction<void()> FadeInCallback = [this, World]()
									{
										if (LoadingFadeWidget)
										{
											// Fade in smoothly (1.5 seconds) to reveal the loaded game
											// Longer duration ensures player position is fully restored before fade completes
											LoadingFadeWidget->FadeIn(1.5f);
										}
									};
									
									World->GetTimerManager().SetTimer(RestoreDimensionTimer, [SaveSystem, World, TempSave, SavedPlayerLocation, SavedPlayerRotation, FadeInCallback]()
									{
										SaveSystem->RestoreLoadedDimension(World, TempSave, SavedPlayerLocation, SavedPlayerRotation, true, FadeInCallback);
									}, 0.2f, false); // Reduced delay: 0.2 seconds should be enough for cartridges to restore
								}
								else
								{
									// No dimension to restore - fade in after restoration completes (for level transition loads)
									// Add a small delay to ensure everything is visually settled before fading in
									if (LoadingFadeWidget)
									{
										FTimerHandle FadeInTimer;
										World->GetTimerManager().SetTimer(FadeInTimer, [this]()
										{
											if (LoadingFadeWidget)
											{
												// Fade in smoothly (1.5 seconds) to reveal the loaded game
												LoadingFadeWidget->FadeIn(1.5f);
											}
										}, 0.2f, false); // 0.2 second delay before fading in
									}
								}
							}
						}
						
						// Clean up temp save
						UGameplayStatics::DeleteGameInSlot(TempSlotName, 0);
					}
				}
			}
		}, 0.5f, false); // 0.5 second delay to ensure player and level are fully loaded
	}
}

void AFirstPersonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Delegate input binding setup to helper
	FPlayerControllerInputSetup::SetupInputBindings(this);
}

void AFirstPersonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Delegate tick subsystems to helpers
	FPlayerControllerTickHandler::TickMovement(this, DeltaTime);
	FPlayerControllerTickHandler::TickHoldToDrop(this, DeltaTime);
	FPlayerControllerTickHandler::TickHoldToUse(this, DeltaTime);
	FPlayerControllerTickHandler::TickPhysicsInteraction(this);
	FPlayerControllerTickHandler::TickInteractHighlight(this);
	FPlayerControllerTickHandler::TickHungerBar(this);
}

void AFirstPersonPlayerController::OnMove(const FInputActionValue& Value)
{
	// Not used when mapping from keys directly; we process movement in PlayerTick for WASD
	const FVector2D Axis = Value.Get<FVector2D>();
	if (APawn* P = GetPawn())
	{
		const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Rt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		P->AddMovementInput(Fwd, Axis.X);
		P->AddMovementInput(Rt, Axis.Y);
	}
}

void AFirstPersonPlayerController::OnLook(const FInputActionValue& Value)
{
	if (bInventoryUIOpen)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();

	// When right mouse is held and we are holding a physics object, forward look deltas to rotate the held object
	if (bRotateHeld)
	{
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				if (PIC->IsHolding())
				{
					PIC->AddRotationInput(Axis);
					return; // do not move camera while rotating the held object
				}
			}
		}
	}

	// Normal camera look
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);

	// Pitch clamping is handled by PlayerCameraManager's ViewPitchMin/Max
}

void AFirstPersonPlayerController::OnJumpPressed(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->Jump();
	}
}

void AFirstPersonPlayerController::OnJumpReleased(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StopJumping();
	}
}

void AFirstPersonPlayerController::OnCrouchToggle(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->ToggleCrouch();
	}
}

void AFirstPersonPlayerController::OnSprintPressed(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StartSprint();
	}
}

void AFirstPersonPlayerController::OnSprintReleased(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StopSprint();
	}
}

void AFirstPersonPlayerController::OnInteractPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractPressed(this, Value);
}

void AFirstPersonPlayerController::OnInteractOngoing(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractOngoing(this, Value);
}

void AFirstPersonPlayerController::OnInteractReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractReleased(this, Value);
}

void AFirstPersonPlayerController::OnInteract(const FInputActionValue& Value)
{
	// Redirect to pressed handler
	OnInteractPressed(Value);
}

void AFirstPersonPlayerController::OnRotateHeldPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnRotateHeldPressed(this, Value);
}

void AFirstPersonPlayerController::OnRotateHeldReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnRotateHeldReleased(this, Value);
}

void AFirstPersonPlayerController::OnThrow(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnThrow(this, Value);
}

void AFirstPersonPlayerController::OnFlashlightToggle(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->ToggleFlashlight();
	}
}

void AFirstPersonPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	// If we already created the hotbar widget, bind it now that we have a pawn
	if (HotbarWidget)
	{
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(InPawn))
		{
			if (UHotbarComponent* HB = C->GetHotbar())
			{
				HotbarWidget->SetHotbar(HB);
			}
		}
	}
}

void AFirstPersonPlayerController::ToggleInventory()
{
	FPlayerControllerUIManager::ToggleInventory(this);
}

void AFirstPersonPlayerController::OpenInventoryWithStorage(UStorageComponent* Storage)
{
	FPlayerControllerUIManager::OpenInventoryWithStorage(this, Storage);
}

UStorageWindowWidget* AFirstPersonPlayerController::GetStorageWindow()
{
	return FPlayerControllerUIManager::GetStorageWindow(this);
}

void AFirstPersonPlayerController::OnPickupPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupPressed(this, Value);
}

void AFirstPersonPlayerController::OnPickupOngoing(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupOngoing(this, Value);
}

void AFirstPersonPlayerController::OnPickupReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupReleased(this, Value);
}

void AFirstPersonPlayerController::OnPickup(const FInputActionValue& Value)
{
	// Redirect to pressed handler
	OnPickupPressed(Value);
}

FString AFirstPersonPlayerController::GetKeyDisplayName(UInputAction* Action) const
{
	if (!Action || !DefaultMappingContext)
	{
		return FString();
	}

	// Query the mapping context for mappings to this action
	const TArray<FEnhancedActionKeyMapping>& Mappings = DefaultMappingContext->GetMappings();
	for (const auto& Mapping : Mappings)
	{
		if (Mapping.Action == Action)
		{
			// Get the first key from the mapping
			if (Mapping.Key.IsValid())
			{
				return Mapping.Key.GetDisplayName().ToString();
			}
		}
	}

	return FString();
}

void AFirstPersonPlayerController::OnSpawnItem(const FInputActionValue& Value)
{
#if !UE_BUILD_SHIPPING
	if (bInventoryUIOpen)
	{
		return;
	}
	// Resolve the item definition from the soft reference (Blueprint-configurable)
	UItemDefinition* Def = DebugSpawnItem.LoadSynchronous();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnItem] DebugSpawnItem is not set or failed to load"));
		return;
	}
	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector BaseLoc = CamLoc + CamRot.Vector() * 200.f;
	const FRotator BaseRot = FRotator(0.f, CamRot.Yaw, 0.f);
	FTransform BaseTransform(BaseRot, BaseLoc, FVector(1.f));
	FTransform FinalTransform = Def ? Def->DefaultDropTransform * BaseTransform : BaseTransform;
	if (UWorld* World = GetWorld())
	{
		AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FinalTransform, Params);
		if (Pickup)
		{
			Pickup->SetItemDef(Def);
			UE_LOG(LogTemp, Display, TEXT("[SpawnItem] Spawned pickup %s at %s"), *GetNameSafe(Def), *FinalTransform.GetLocation().ToString());
		}
	}
#endif
}

float AFirstPersonPlayerController::ClampPitchDegrees(float Degrees)
{
	return FMath::Clamp(Degrees, -89.f, 89.f);
}

void AFirstPersonPlayerController::OnPausePressed(const FInputActionValue& Value)
{
	// Don't pause if inventory is open
	if (bInventoryUIOpen)
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		return;
	}

	// Use actual game pause state as source of truth
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	bool bGameIsPaused = World->IsPaused();

	// Check if we just unpaused to prevent buffered input from immediately re-pausing
	// This check applies to both pausing and unpausing
	if (bIsUnpausing)
	{
		// Ignore this input - it's likely buffered from the unpause
		return;
	}

	if (bGameIsPaused)
	{
		// Game is paused - Enhanced Input might not fire reliably when paused,
		// but if it does, forward to the widget which handles Escape key directly
		// The widget's NativeOnKeyDown will handle unpausing
		// For now, just call Hide() - the widget should handle keyboard focus
		PauseMenuWidget->Hide();
	}
	else
	{
		// Game is not paused - user wants to pause
		
		// Show pause menu
		PauseMenuWidget->Show();
		
		// Set input mode to UI only
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		// Pause the game
		SetPause(true);
		bIsPaused = true;
		
	}
}
