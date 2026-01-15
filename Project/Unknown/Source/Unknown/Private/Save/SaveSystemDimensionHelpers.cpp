#include "Save/SaveSystemDimensionHelpers.h"
#include "Save/GameSaveData.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "Components/SaveableActorComponent.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/StorageSerialization.h"
#include "Inventory/ItemDefinition.h"
#include "Save/SaveSystemHelpers.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "EngineUtils.h"

bool SaveSystemDimensionHelpers::SaveDimensionInstance(
	UWorld* World,
	UGameSaveData* SaveData,
	UDimensionManagerSubsystem* DimensionManager,
	FGuid InstanceId,
	const FString& SlotName)
{
	// SlotName is now optional - if empty, we're just updating SaveData in memory
	// If provided, we'll save to disk (legacy behavior, but not used anymore)
	if (!World || !SaveData || !DimensionManager || !InstanceId.IsValid())
	{
		return false;
	}

	ULevel* DimensionLevel = DimensionManager->GetDimensionLevel(InstanceId);
	if (!DimensionLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Cannot save dimension: Dimension level not found for instance %s"), *InstanceId.ToString());
		return false;
	}

	// Find or create dimension instance save data
	FDimensionInstanceSaveData* DimensionSaveData = nullptr;
	for (FDimensionInstanceSaveData& DimData : SaveData->DimensionInstances)
	{
		if (DimData.InstanceId == InstanceId)
		{
			DimensionSaveData = &DimData;
			break;
		}
	}

	if (!DimensionSaveData)
	{
		FDimensionInstanceSaveData NewDimData;
		NewDimData.InstanceId = InstanceId;
		SaveData->DimensionInstances.Add(NewDimData);
		DimensionSaveData = &SaveData->DimensionInstances.Last();
	}

	// Get dimension instance info
	FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
	if (InstanceInfo.InstanceId == InstanceId)
	{
		DimensionSaveData->CartridgeId = InstanceInfo.CartridgeId;
		DimensionSaveData->WorldPosition = InstanceInfo.WorldPosition;
		DimensionSaveData->Stability = 100.0f; // TODO: Get from instance data
		DimensionSaveData->DimensionDefinitionPath = InstanceInfo.DimensionLevel.ToSoftObjectPath().ToString();
		
		// Mark this dimension as loaded in the main save data
		// This ensures we can restore it when loading the save
		if (SaveData)
		{
			SaveData->LoadedDimensionInstanceId = InstanceId;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Marked dimension instance %s as loaded in save data"), 
				*InstanceId.ToString());
		}
	}

	// Save all actors in the dimension level
	DimensionSaveData->ActorStates.Empty();
	TArray<FGuid> CurrentActorIds;

	for (AActor* Actor : DimensionLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}

		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (!SaveableComp)
		{
			continue;
		}

		// Only save actors that belong to this dimension
		if (SaveableComp->GetDimensionInstanceId() != InstanceId)
		{
			continue;
		}

		FGuid ActorId = SaveableComp->GetPersistentId();
		if (!ActorId.IsValid())
		{
			continue;
		}

		CurrentActorIds.Add(ActorId);

		// Create actor state data
		FActorStateSaveData ActorState;
		ActorState.ActorId = ActorId;
		ActorState.bExists = true;
		ActorState.ActorName = Actor->GetName();
		ActorState.ActorClassPath = Actor->GetClass()->GetPathName();
		
		// Get transform in level-local space (for streaming levels)
		FTransform ActorWorldTransform = Actor->GetActorTransform();
		FTransform OriginalTransformToSave = SaveableComp->OriginalTransform;
		FVector DimensionWorldPos = InstanceInfo.WorldPosition;
		
		// CRITICAL: OriginalTransform should ALWAYS be in level-local space for dimension actors
		// It's set to local space during baseline generation and TagActorsInDimension
		// We need to detect if it's already local or if it's in world space
		bool bIsAlreadyLocal = false;
		
		if (!OriginalTransformToSave.GetLocation().IsNearlyZero())
		{
			FVector OriginalLocation = OriginalTransformToSave.GetLocation();
			FVector TestConverted = OriginalLocation - DimensionWorldPos;
			
			// If the original location is small (<10000 units from origin) and the test conversion
			// would result in a very large value (>10000 units), it means OriginalTransform is
			// already in local space (converting again would double the offset)
			// Also check: if original location is negative and large, it's likely already local
			if ((OriginalLocation.Size() < 10000.0f && TestConverted.Size() > 10000.0f) ||
				(OriginalLocation.Size() < DimensionWorldPos.Size() * 0.1f))
			{
				bIsAlreadyLocal = true;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] OriginalTransform for %s is already in local space: %s (test convert would give: %s, world pos: %s)"), 
					*Actor->GetName(), *OriginalLocation.ToString(), *TestConverted.ToString(), *DimensionWorldPos.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] OriginalTransform for %s appears to be in world space: %s (test convert: %s, world pos: %s)"), 
					*Actor->GetName(), *OriginalLocation.ToString(), *TestConverted.ToString(), *DimensionWorldPos.ToString());
			}
		}
		
		// If OriginalTransform is not set, use current transform and convert to local
		if (OriginalTransformToSave.GetLocation().IsNearlyZero())
		{
			OriginalTransformToSave = ActorWorldTransform;
			// Don't set it on the component yet - we'll set it after converting
			bIsAlreadyLocal = false; // World transform needs conversion
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] OriginalTransform was empty for %s, using current world transform: %s"), 
				*Actor->GetName(), *ActorWorldTransform.GetLocation().ToString());
		}
		
		// Convert to level-local space for saving (streaming levels)
		// Only convert if it's not already in local space
		FTransform LocalTransform;
		if (bIsAlreadyLocal)
		{
			// Already in local space, use directly
			LocalTransform = OriginalTransformToSave;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Using OriginalTransform directly (already local) for %s: %s"), 
				*Actor->GetName(), *LocalTransform.GetLocation().ToString());
		}
		else
		{
			// Convert from world space to local space
			LocalTransform = OriginalTransformToSave;
			FVector OriginalLocalLocation = OriginalTransformToSave.GetLocation() - DimensionWorldPos;
			LocalTransform.SetLocation(OriginalLocalLocation);
			
			// CRITICAL: Do NOT update SaveableComp->OriginalTransform here!
			// OriginalTransform should only be set during baseline generation or when restoring from save.
			// If it's in world space here, it means it was set incorrectly earlier, but we shouldn't
			// change it during save operations as that would corrupt the baseline.
			// We'll just use the converted value for saving, but leave the component's OriginalTransform unchanged.
			
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] OriginalTransform for %s was in world space during save - converted to local for saving only. OriginalTransform on component not updated (should be set during baseline generation)."), 
				*Actor->GetName());
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Converted OriginalTransform from world to local for %s - world: %s, local: %s"), 
				*Actor->GetName(), *OriginalTransformToSave.GetLocation().ToString(), *OriginalLocalLocation.ToString());
		}
		
		ActorState.OriginalSpawnTransform = LocalTransform;

		// Check if this is a new object
		bool bIsInBaseline = DimensionSaveData->BaselineActorIds.Num() > 0 && 
			DimensionSaveData->BaselineActorIds.Contains(ActorId);
		bool bIsNewObject = DimensionSaveData->BaselineActorIds.Num() > 0 && !bIsInBaseline;
		
		if (bIsNewObject)
		{
			ActorState.bIsNewObject = true;
			ActorState.SpawnActorClassPath = Actor->GetClass()->GetPathName();
		}

		// Save transform (in level-local space for streaming levels)
		if (SaveableComp->bSaveTransform)
		{
			FVector WorldLocation = Actor->GetActorLocation();
			FVector CurrentLocalLocation = WorldLocation - DimensionWorldPos;
			ActorState.Location = CurrentLocalLocation;
			ActorState.Rotation = Actor->GetActorRotation();
			ActorState.Scale = Actor->GetActorScale3D();
		}

		// Save physics state
		UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
		{
			ActorState.bHasPhysics = true;
			ActorState.bSimulatePhysics = true;
			
			if (SaveableComp->bSavePhysicsState)
			{
				ActorState.LinearVelocity = PrimitiveComp->GetPhysicsLinearVelocity();
				ActorState.AngularVelocity = PrimitiveComp->GetPhysicsAngularVelocityInRadians();
			}
		}

		// Save storage component data
		UStorageComponent* StorageComp = Actor->FindComponentByClass<UStorageComponent>();
		if (StorageComp)
		{
			ActorState.bHasStorage = true;
			TArray<FItemEntry> StorageEntries = StorageComp->GetEntries();
			ActorState.SerializedStorageEntries = StorageSerialization::SerializeStorageEntries(StorageEntries);
			ActorState.StorageMaxVolume = StorageComp->MaxVolume;
		}

		// Save ItemPickup data
		AItemPickup* ItemPickup = Cast<AItemPickup>(Actor);
		if (ItemPickup)
		{
			FItemEntry ItemEntry = ItemPickup->GetItemEntry();
			if (ItemEntry.Def)
			{
				ActorState.ItemDefinitionPath = ItemEntry.Def->GetPathName();
				ActorState.SerializedItemEntry = SaveSystemHelpers::SerializeItemEntry(ItemEntry);
			}
		}

		DimensionSaveData->ActorStates.Add(ActorState);
	}

	// Detect removed actors (in baseline but not in current state)
	// Also check if any saved actors are no longer in the current state
	if (DimensionSaveData->BaselineActorIds.Num() > 0)
	{
		TSet<FGuid> CurrentActorIdsSet;
		for (const FGuid& Id : CurrentActorIds)
		{
			CurrentActorIdsSet.Add(Id);
		}

		// Check all baseline actors
		for (const FGuid& BaselineId : DimensionSaveData->BaselineActorIds)
		{
			if (!CurrentActorIdsSet.Contains(BaselineId))
			{
				// Check if this actor is already in ActorStates (might have been saved before)
				bool bAlreadyInActorStates = false;
				for (const FActorStateSaveData& ExistingState : DimensionSaveData->ActorStates)
				{
					if (ExistingState.ActorId == BaselineId)
					{
						bAlreadyInActorStates = true;
						// Update it to mark as removed
						FActorStateSaveData& MutableState = const_cast<FActorStateSaveData&>(ExistingState);
						MutableState.bExists = false;
						UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Marked existing actor state %s as removed"), *BaselineId.ToString());
						break;
					}
				}

				if (!bAlreadyInActorStates)
				{
					// Actor was in baseline but not found - mark as removed
					FActorStateSaveData RemovedActorState;
					RemovedActorState.ActorId = BaselineId;
					RemovedActorState.bExists = false;
					
					// Try to find the actor in the world to get metadata (might still exist but in wrong level)
					AActor* RemovedActor = USaveableActorComponent::FindActorByGuid(World, BaselineId);
					if (RemovedActor)
					{
						RemovedActorState.ActorName = RemovedActor->GetName();
						RemovedActorState.ActorClassPath = RemovedActor->GetClass()->GetPathName();
						if (USaveableActorComponent* RemovedSaveable = RemovedActor->FindComponentByClass<USaveableActorComponent>())
						{
							RemovedActorState.OriginalSpawnTransform = RemovedSaveable->OriginalTransform;
						}
					}
					else
					{
						// Try to find metadata from previous save
						for (const FActorStateSaveData& PreviousState : DimensionSaveData->ActorStates)
						{
							if (PreviousState.ActorId == BaselineId && PreviousState.bExists)
							{
								RemovedActorState.ActorName = PreviousState.ActorName;
								RemovedActorState.ActorClassPath = PreviousState.ActorClassPath;
								RemovedActorState.OriginalSpawnTransform = PreviousState.OriginalSpawnTransform;
								break;
							}
						}
					}
					
					DimensionSaveData->ActorStates.Add(RemovedActorState);
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Marked actor %s as removed (was in baseline)"), *BaselineId.ToString());
				}
			}
		}
	}

	// CRITICAL: Only set baseline if it's empty (first time saving)
	// The baseline should NEVER change after it's been set - it represents the original
	// state of the level file when first loaded. All changes are tracked separately.
	if (DimensionSaveData->BaselineActorIds.Num() == 0)
	{
		// First time saving - generate baseline from current level state
		// This should only contain actors that were originally in the level file
		// (not newly spawned ones, but we can't distinguish that here, so we'll use
		// all current actors as the baseline - this is fine since it's the first save)
		DimensionSaveData->BaselineActorIds = CurrentActorIds;
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline for dimension instance %s with %d actors (first save)"), 
			*InstanceId.ToString(), DimensionSaveData->BaselineActorIds.Num());
	}
	else
	{
		// Baseline already exists - DO NOT UPDATE IT
		// The baseline represents the original level state and must remain constant
		UE_LOG(LogTemp, Verbose, TEXT("[SaveSystemDimension] Baseline already exists (%d actors) - not updating"), 
			DimensionSaveData->BaselineActorIds.Num());
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Saved dimension instance %s with %d actors (%d existing, %d removed)"), 
		*InstanceId.ToString(), DimensionSaveData->ActorStates.Num(), CurrentActorIds.Num(), 
		DimensionSaveData->ActorStates.Num() - CurrentActorIds.Num());

	// Don't save to disk here - let SaveGame() handle it
	// This ensures dimension instances are included when the full game is saved
	return true;
}

bool SaveSystemDimensionHelpers::LoadDimensionInstance(
	UWorld* World,
	UGameSaveData* SaveData,
	UDimensionManagerSubsystem* DimensionManager,
	FGuid InstanceId)
{
	if (!World || !SaveData || !DimensionManager || !InstanceId.IsValid())
	{
		return false;
	}

	ULevel* DimensionLevel = DimensionManager->GetDimensionLevel(InstanceId);
	if (!DimensionLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Cannot load dimension: Dimension level not found for instance %s"), *InstanceId.ToString());
		return false;
	}

	// Find dimension instance save data
	FDimensionInstanceSaveData* DimensionSaveData = nullptr;
	
	// Log all dimension instances in save data for debugging
	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Looking for dimension instance %s in save data. Total dimension instances in save: %d"), 
		*InstanceId.ToString(), SaveData->DimensionInstances.Num());
	for (const FDimensionInstanceSaveData& DimData : SaveData->DimensionInstances)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Found saved dimension instance: %s (CartridgeId: %s)"), 
			*DimData.InstanceId.ToString(), *DimData.CartridgeId.ToString());
		if (DimData.InstanceId == InstanceId)
		{
			DimensionSaveData = const_cast<FDimensionInstanceSaveData*>(&DimData);
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Matched dimension instance %s in save data"), *InstanceId.ToString());
			break;
		}
	}

	// If no save data exists, this is the first time loading - generate baseline immediately
	if (!DimensionSaveData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] No save data found for dimension instance %s (first time loading) - generating baseline. This may indicate the instance ID doesn't match saved data."), *InstanceId.ToString());
		
		// Create new save data entry
		FDimensionInstanceSaveData NewDimData;
		NewDimData.InstanceId = InstanceId;
		FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
		if (InstanceInfo.InstanceId == InstanceId)
		{
			NewDimData.CartridgeId = InstanceInfo.CartridgeId;
			NewDimData.WorldPosition = InstanceInfo.WorldPosition;
			NewDimData.Stability = 100.0f; // TODO: Get from instance data
			NewDimData.DimensionDefinitionPath = InstanceInfo.DimensionLevel.ToSoftObjectPath().ToString();
		}
		
		// Generate baseline from current level state (original actors in the level file)
		NewDimData.BaselineActorIds.Empty();
		for (AActor* Actor : DimensionLevel->Actors)
		{
			if (!Actor)
			{
				continue;
			}

			USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
			if (!SaveableComp)
			{
				continue;
			}

			// Only include actors that belong to this dimension
			if (SaveableComp->GetDimensionInstanceId() != InstanceId)
			{
				continue;
			}

			FGuid ActorId = SaveableComp->GetPersistentId();
			if (!ActorId.IsValid())
			{
				// Generate GUID if not set
				ActorId = FGuid::NewGuid();
				SaveableComp->SetPersistentId(ActorId);
			}

			// Set OriginalTransform if not set
			// Convert to level-local space immediately for consistency
			if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero())
			{
				FTransform WorldTransform = Actor->GetActorTransform();
				FVector DimensionWorldPos = InstanceInfo.WorldPosition;
				FTransform LocalTransform = WorldTransform;
				FVector LocalLocation = WorldTransform.GetLocation() - DimensionWorldPos;
				LocalTransform.SetLocation(LocalLocation);
				SaveableComp->OriginalTransform = LocalTransform;
				
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline: Set OriginalTransform for %s - world: %s, local: %s"), 
					*Actor->GetName(), *WorldTransform.GetLocation().ToString(), *LocalLocation.ToString());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline: %s already has OriginalTransform: %s"), 
					*Actor->GetName(), *SaveableComp->OriginalTransform.GetLocation().ToString());
			}

			NewDimData.BaselineActorIds.Add(ActorId);
		}

		// Add to save data
		SaveData->DimensionInstances.Add(NewDimData);
		DimensionSaveData = &SaveData->DimensionInstances.Last();
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline for dimension instance %s with %d actors"), 
			*InstanceId.ToString(), NewDimData.BaselineActorIds.Num());
		
		// No saved state to restore - baseline is set, return success
		return true;
	}

	// Check if baseline is empty (shouldn't happen, but handle it)
	if (DimensionSaveData->BaselineActorIds.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Baseline is empty for dimension instance %s - generating baseline now"), *InstanceId.ToString());
		
		// Generate baseline from current level state
		DimensionSaveData->BaselineActorIds.Empty();
		for (AActor* Actor : DimensionLevel->Actors)
		{
			if (!Actor)
			{
				continue;
			}

			USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
			if (!SaveableComp)
			{
				continue;
			}

			// Only include actors that belong to this dimension
			if (SaveableComp->GetDimensionInstanceId() != InstanceId)
			{
				continue;
			}

			FGuid ActorId = SaveableComp->GetPersistentId();
			if (!ActorId.IsValid())
			{
				// Generate GUID if not set
				ActorId = FGuid::NewGuid();
				SaveableComp->SetPersistentId(ActorId);
			}

			// Set OriginalTransform if not set
			// Convert to level-local space immediately for consistency
			if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero())
			{
				FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
				FTransform WorldTransform = Actor->GetActorTransform();
				FVector DimensionWorldPos = InstanceInfo.WorldPosition;
				FTransform LocalTransform = WorldTransform;
				FVector LocalLocation = WorldTransform.GetLocation() - DimensionWorldPos;
				LocalTransform.SetLocation(LocalLocation);
				SaveableComp->OriginalTransform = LocalTransform;
				
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline (fallback): Set OriginalTransform for %s - world: %s, local: %s"), 
					*Actor->GetName(), *WorldTransform.GetLocation().ToString(), *LocalLocation.ToString());
			}

			DimensionSaveData->BaselineActorIds.Add(ActorId);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Generated baseline for dimension instance %s with %d actors"), 
			*InstanceId.ToString(), DimensionSaveData->BaselineActorIds.Num());
	}

	// Create baseline set for quick lookup
	TSet<FGuid> BaselineActorIds;
	for (const FGuid& BaselineId : DimensionSaveData->BaselineActorIds)
	{
		BaselineActorIds.Add(BaselineId);
	}

	TSet<AActor*> RestoredActors;
	int32 RemovedCount = 0;
	int32 RestoredCount = 0;
	int32 NewObjectSpawnedCount = 0;

	// === STEP 1: Destroy removed actors ===
	// Destroy actors that are marked as not existing in the save
	// (GUIDs should have been restored in STEP 1.5)
	for (const FActorStateSaveData& ActorState : DimensionSaveData->ActorStates)
	{
		if (ActorState.bExists)
		{
			continue; // Skip existing actors
		}

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
		
		if (ActorToRemove && ActorToRemove->GetLevel() == DimensionLevel)
		{
			// Verify it belongs to this dimension
			USaveableActorComponent* SaveableComp = ActorToRemove->FindComponentByClass<USaveableActorComponent>();
			if (SaveableComp && SaveableComp->GetDimensionInstanceId() == InstanceId)
			{
				ActorToRemove->Destroy();
				RemovedCount++;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Destroyed removed actor %s"), *ActorState.ActorId.ToString());
			}
		}
	}

	// === STEP 1.5: Early GUID Restoration (Before we try to find/spawn actors) ===
	// Restore GUIDs to actors using metadata matching before they generate new GUIDs
	// This must happen as early as possible to prevent GUID regeneration and duplicates
	// Track which actors have already been matched to prevent duplicate matches
	TSet<AActor*> MatchedActors;
	int32 EarlyGUIDRestoredCount = 0;
	
	// First pass: Restore GUIDs for existing actors (bExists = true) that are in baseline
	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Starting GUID restoration. DimensionLevel has %d actors, baseline has %d actors"), 
		DimensionLevel ? DimensionLevel->Actors.Num() : 0, BaselineActorIds.Num());
	
	for (const FActorStateSaveData& ActorState : DimensionSaveData->ActorStates)
	{
		// Skip removed actors in first pass
		if (!ActorState.bExists || ActorState.ActorClassPath.IsEmpty())
		{
			continue;
		}
		
		// Only match actors that are in baseline (existing level actors)
		if (!BaselineActorIds.Contains(ActorState.ActorId))
		{
			continue; // Skip new objects - they'll be spawned later
		}
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Looking for baseline actor %s (class: %s)"), 
			*ActorState.ActorId.ToString(), *ActorState.ActorClassPath);
		
		// Try to find actor by GUID first
		AActor* Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
		
		if (Actor)
		{
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Found baseline actor %s by GUID"), *ActorState.ActorId.ToString());
			MatchedActors.Add(Actor);
			EarlyGUIDRestoredCount++;
			continue;
		}
		
		// If not found, try metadata fallback matching
		FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
		FVector DimensionWorldPos = InstanceInfo.WorldPosition;
		FVector SavedLocation = ActorState.OriginalSpawnTransform.GetLocation();
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: GUID not found, trying metadata matching. Saved local pos: %s, DimensionWorldPos: %s, DimensionLevel has %d actors"), 
			*SavedLocation.ToString(), *DimensionWorldPos.ToString(), DimensionLevel->Actors.Num());
		
		int32 CandidatesChecked = 0;
		int32 CandidatesWithClassMatch = 0;
		int32 CandidatesWithComponent = 0;
		int32 CandidatesWithDimensionID = 0;
		int32 CandidatesWithTransformCheck = 0;
		
		for (AActor* Candidate : DimensionLevel->Actors)
		{
			if (!Candidate)
			{
				continue;
			}
			
			CandidatesChecked++;
			
			if (MatchedActors.Contains(Candidate))
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s already matched, skipping"), *Candidate->GetName());
				continue;
			}
			
			// Check if class path matches
			FString CandidateClassPath = Candidate->GetClass()->GetPathName();
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Checking candidate %s (class: %s, expected: %s)"), 
				*Candidate->GetName(), *CandidateClassPath, *ActorState.ActorClassPath);
			
			if (CandidateClassPath != ActorState.ActorClassPath)
			{
				continue;
			}
			
			CandidatesWithClassMatch++;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s has matching class"), *Candidate->GetName());
			
			// Check if this actor has a SaveableActorComponent
			USaveableActorComponent* SaveableComp = Candidate->FindComponentByClass<USaveableActorComponent>();
			if (!SaveableComp)
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s has no SaveableActorComponent"), *Candidate->GetName());
				continue;
			}
			
			CandidatesWithComponent++;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s has SaveableActorComponent"), *Candidate->GetName());
			
			FGuid CandidateDimensionID = SaveableComp->GetDimensionInstanceId();
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s dimension ID: %s (expected: %s)"), 
				*Candidate->GetName(), *CandidateDimensionID.ToString(), *InstanceId.ToString());
			
			// If dimension ID doesn't match and it's set, skip this candidate
			// If dimension ID is invalid (not set yet), we'll set it after matching
			if (CandidateDimensionID.IsValid() && CandidateDimensionID != InstanceId)
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s has wrong dimension ID"), *Candidate->GetName());
				continue;
			}
			
			// If dimension ID is not set yet, set it now (TagActorsInDimension might not have set it yet)
			if (!CandidateDimensionID.IsValid())
			{
				SaveableComp->SetDimensionInstanceId(InstanceId);
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Set dimension ID for candidate %s"), *Candidate->GetName());
			}
			
			CandidatesWithDimensionID++;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s has correct dimension ID"), *Candidate->GetName());
			
			// If actor already has a valid GUID that matches, we're done
			FGuid CandidateGUID = SaveableComp->GetPersistentId();
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s GUID: %s (expected: %s)"), 
				*Candidate->GetName(), *CandidateGUID.ToString(), *ActorState.ActorId.ToString());
			
			if (CandidateGUID.IsValid() && CandidateGUID == ActorState.ActorId)
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s already has matching GUID"), *Candidate->GetName());
				Actor = Candidate;
				MatchedActors.Add(Candidate);
				EarlyGUIDRestoredCount++;
				break;
			}
			
			// Get transform to compare (use OriginalTransform if set, otherwise use current transform)
			FTransform TransformToCompare = SaveableComp->OriginalTransform;
			bool bTransformIsEmpty = TransformToCompare.GetLocation().IsNearlyZero() && 
				TransformToCompare.GetRotation().IsIdentity() && 
				TransformToCompare.GetScale3D().IsNearlyZero();
			
			if (bTransformIsEmpty)
			{
				// OriginalTransform not set yet - use current actor transform
				TransformToCompare = Candidate->GetActorTransform();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s using current transform (OriginalTransform was empty)"), *Candidate->GetName());
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s using OriginalTransform"), *Candidate->GetName());
			}
			
			FVector CandidateLocation = TransformToCompare.GetLocation();
			
			// Always try converting from world space first (most common case after level load)
			FVector CandidateLocalFromWorld = CandidateLocation - DimensionWorldPos;
			FVector DeltaAsIs = CandidateLocation - SavedLocation;
			FVector DeltaConverted = CandidateLocalFromWorld - SavedLocation;
			
			FVector CandidateLocalLocation;
			if (DeltaConverted.Size() < DeltaAsIs.Size())
			{
				CandidateLocalLocation = CandidateLocalFromWorld;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s - using world-to-local conversion (world: %s, local: %s, delta converted: %f)"), 
					*Candidate->GetName(), *CandidateLocation.ToString(), *CandidateLocalLocation.ToString(), DeltaConverted.Size());
			}
			else
			{
				CandidateLocalLocation = CandidateLocation;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s - using as-is (already local: %s, delta as-is: %f)"), 
					*Candidate->GetName(), *CandidateLocalLocation.ToString(), DeltaAsIs.Size());
			}
			
			FVector LocationDelta = CandidateLocalLocation - SavedLocation;
			float LocationDeltaSize = LocationDelta.Size();
			
			CandidatesWithTransformCheck++;
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s transform check - saved local: %s, candidate local: %s, delta: %f (tolerance: 100.0)"), 
				*Candidate->GetName(), *SavedLocation.ToString(), *CandidateLocalLocation.ToString(), LocationDeltaSize);
			
			if (LocationDeltaSize <= 100.0f)  // Increased tolerance to 100cm
			{
				// Check rotation (within tolerance)
				FRotator RotationDelta = (TransformToCompare.GetRotation() * ActorState.OriginalSpawnTransform.GetRotation().Inverse()).Rotator();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s rotation check - pitch: %f, yaw: %f, roll: %f (tolerance: 10.0)"), 
					*Candidate->GetName(), RotationDelta.Pitch, RotationDelta.Yaw, RotationDelta.Roll);
				
				if (FMath::Abs(RotationDelta.Pitch) <= 10.0f &&
					FMath::Abs(RotationDelta.Yaw) <= 10.0f &&
					FMath::Abs(RotationDelta.Roll) <= 10.0f)
				{
					// Match found - restore the GUID and ensure OriginalTransform and dimension ID are set
					SaveableComp->SetPersistentId(ActorState.ActorId);
					// Ensure dimension instance ID is set (it should be, but make sure)
					SaveableComp->SetDimensionInstanceId(InstanceId);
					// Set OriginalTransform in level-local space for consistency
					FTransform LocalTransform = ActorState.OriginalSpawnTransform;
					SaveableComp->OriginalTransform = LocalTransform;
					Actor = Candidate;
					MatchedActors.Add(Candidate);
					EarlyGUIDRestoredCount++;
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: *** MATCH FOUND! *** Restored GUID %s to baseline actor %s (location delta: %f)"), 
						*ActorState.ActorId.ToString(), *Candidate->GetName(), LocationDeltaSize);
					break;
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s location match but rotation mismatch (pitch: %f, yaw: %f, roll: %f)"), 
						*Candidate->GetName(), RotationDelta.Pitch, RotationDelta.Yaw, RotationDelta.Roll);
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Candidate %s location delta too large: %f (max: 100.0)"), 
					*Candidate->GetName(), LocationDeltaSize);
			}
		}
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 1.5: Summary for actor %s - Checked %d candidates, %d class matches, %d with component, %d with dimension ID, %d transform checks"), 
			*ActorState.ActorId.ToString(), CandidatesChecked, CandidatesWithClassMatch, CandidatesWithComponent, CandidatesWithDimensionID, CandidatesWithTransformCheck);
		
		if (!Actor)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] STEP 1.5: Could not find baseline actor %s (class: %s) in level"), 
				*ActorState.ActorId.ToString(), *ActorState.ActorClassPath);
		}
	}
	
	// Second pass: Restore GUIDs for removed actors (bExists = false) so we can find them to destroy
	int32 RemovedActorGUIDRestoredCount = 0;
	for (const FActorStateSaveData& ActorState : DimensionSaveData->ActorStates)
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
		for (AActor* Candidate : DimensionLevel->Actors)
		{
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
			if (!SaveableComp || SaveableComp->GetDimensionInstanceId() != InstanceId)
			{
				continue;
			}
			
			// Get transform to compare
			FTransform TransformToCompare = SaveableComp->OriginalTransform;
			bool bTransformIsEmpty = TransformToCompare.GetLocation().IsNearlyZero();
			
			if (bTransformIsEmpty)
			{
				TransformToCompare = Candidate->GetActorTransform();
			}
			
			// Check if transform matches (within tolerance)
			// Convert to level-local space for comparison
			FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
			FVector DimensionWorldPos = InstanceInfo.WorldPosition;
			FVector SavedLocation = ActorState.OriginalSpawnTransform.GetLocation();
			FVector CandidateLocation = TransformToCompare.GetLocation();
			
			// Convert candidate transform to level-local space
			FVector CandidateLocalLocation;
			if (CandidateLocation.Size() > 10000.0f)  // Likely world space
			{
				CandidateLocalLocation = CandidateLocation - DimensionWorldPos;
			}
			else
			{
				// Might be level-local already, check both
				FVector CandidateLocalFromWorld = CandidateLocation - DimensionWorldPos;
				FVector DeltaAsIs = CandidateLocation - SavedLocation;
				FVector DeltaConverted = CandidateLocalFromWorld - SavedLocation;
				
				if (DeltaConverted.Size() < DeltaAsIs.Size())
				{
					CandidateLocalLocation = CandidateLocalFromWorld;
				}
				else
				{
					CandidateLocalLocation = CandidateLocation;
				}
			}
			
			FVector LocationDelta = CandidateLocalLocation - SavedLocation;
			if (LocationDelta.Size() <= 50.0f)  // 50cm tolerance
			{
				// Check rotation
				FRotator RotationDelta = (TransformToCompare.GetRotation() * ActorState.OriginalSpawnTransform.GetRotation().Inverse()).Rotator();
				if (FMath::Abs(RotationDelta.Pitch) <= 10.0f &&
					FMath::Abs(RotationDelta.Yaw) <= 10.0f &&
					FMath::Abs(RotationDelta.Roll) <= 10.0f)
				{
					// Match found - restore the GUID for removal
					SaveableComp->SetPersistentId(ActorState.ActorId);
					// Set OriginalTransform in level-local space
					FTransform LocalTransform = ActorState.OriginalSpawnTransform;
					SaveableComp->OriginalTransform = LocalTransform;
					MatchedActors.Add(Candidate);
					RemovedActorGUIDRestoredCount++;
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Restored GUID %s to removed actor %s (location delta: %f)"), 
						*ActorState.ActorId.ToString(), *Candidate->GetName(), LocationDelta.Size());
					break;
				}
			}
		}
	}
	
	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Restored %d GUIDs for existing actors, %d for removed actors"), 
		EarlyGUIDRestoredCount, RemovedActorGUIDRestoredCount);

	// === STEP 2: Restore existing actors and spawn new ones ===
	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Loading %d actor states, baseline has %d actors"), 
		DimensionSaveData->ActorStates.Num(), BaselineActorIds.Num());
	
	// Create a map of ActorId -> Actor for actors already matched in STEP 1.5
	// This allows STEP 2 to quickly find actors that were already matched
	TMap<FGuid, AActor*> AlreadyMatchedActorsMap;
	for (AActor* MatchedActor : MatchedActors)
	{
		if (MatchedActor)
		{
			USaveableActorComponent* SaveableComp = MatchedActor->FindComponentByClass<USaveableActorComponent>();
			if (SaveableComp && SaveableComp->GetPersistentId().IsValid())
			{
				AlreadyMatchedActorsMap.Add(SaveableComp->GetPersistentId(), MatchedActor);
			}
		}
	}
	
	for (const FActorStateSaveData& ActorState : DimensionSaveData->ActorStates)
	{
		if (!ActorState.bExists)
		{
			continue; // Already handled in STEP 1
		}

		AActor* Actor = nullptr;
		bool bIsInBaseline = BaselineActorIds.Contains(ActorState.ActorId);
		bool bIsNewObject = ActorState.bIsNewObject || !bIsInBaseline;
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Processing actor %s: bIsNewObject=%d, bIsNewObjectFlag=%d, inBaseline=%d, SpawnClassPath=%s, ActorClassPath=%s"), 
			*ActorState.ActorId.ToString(), 
			bIsNewObject ? 1 : 0,
			ActorState.bIsNewObject ? 1 : 0,
			bIsInBaseline ? 1 : 0,
			*ActorState.SpawnActorClassPath,
			*ActorState.ActorClassPath);

		// First, check if this actor was already matched in STEP 1.5
		AActor** AlreadyMatchedActorPtr = AlreadyMatchedActorsMap.Find(ActorState.ActorId);
		if (AlreadyMatchedActorPtr && *AlreadyMatchedActorPtr)
		{
			Actor = *AlreadyMatchedActorPtr;
			UE_LOG(LogTemp, Verbose, TEXT("[SaveSystemDimension] STEP 2: Found actor %s from STEP 1.5 match"), *ActorState.ActorId.ToString());
		}
		
		// If not found from STEP 1.5, try to find by GUID
		if (!Actor)
		{
			Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
			if (Actor)
			{
				UE_LOG(LogTemp, Verbose, TEXT("[SaveSystemDimension] STEP 2: Found actor %s by GUID lookup"), *ActorState.ActorId.ToString());
			}
		}
		
		// If not found by GUID, try metadata lookup (but only for baseline actors to avoid false matches)
		// This is a fallback for when STEP 1.5 didn't match the actor (e.g., after save/load when GUIDs are lost)
		if (!Actor && bIsInBaseline && !ActorState.ActorClassPath.IsEmpty())
		{
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Trying metadata matching for baseline actor %s"), *ActorState.ActorId.ToString());
			
			// For baseline actors, try to find by metadata in the dimension level
			// This handles cases where the level was reloaded and actors don't have GUIDs yet
			FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
			FVector DimensionWorldPos = InstanceInfo.WorldPosition;
			FVector SavedLocation = ActorState.OriginalSpawnTransform.GetLocation();
			
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Metadata matching - saved local pos: %s, DimensionWorldPos: %s, DimensionLevel has %d actors"), 
				*SavedLocation.ToString(), *DimensionWorldPos.ToString(), DimensionLevel->Actors.Num());
			
			int32 CandidatesChecked = 0;
			int32 CandidatesWithClassMatch = 0;
			int32 CandidatesWithComponent = 0;
			int32 CandidatesWithDimensionID = 0;
			int32 CandidatesWithTransformCheck = 0;
			
			for (AActor* Candidate : DimensionLevel->Actors)
			{
				if (!Candidate)
				{
					continue;
				}
				
				CandidatesChecked++;
				
				if (MatchedActors.Contains(Candidate))
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s already matched, skipping"), *Candidate->GetName());
					continue;
				}
				
				// Check if class path matches
				FString CandidateClassPath = Candidate->GetClass()->GetPathName();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Checking candidate %s (class: %s, expected: %s)"), 
					*Candidate->GetName(), *CandidateClassPath, *ActorState.ActorClassPath);
				
				if (CandidateClassPath != ActorState.ActorClassPath)
				{
					continue;
				}
				
				CandidatesWithClassMatch++;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s has matching class"), *Candidate->GetName());
				
				// Check if this actor has a SaveableActorComponent
				USaveableActorComponent* SaveableComp = Candidate->FindComponentByClass<USaveableActorComponent>();
				if (!SaveableComp)
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s has no SaveableActorComponent"), *Candidate->GetName());
					continue;
				}
				
				CandidatesWithComponent++;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s has SaveableActorComponent"), *Candidate->GetName());
				
				FGuid CandidateDimensionID = SaveableComp->GetDimensionInstanceId();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s dimension ID: %s (expected: %s)"), 
					*Candidate->GetName(), *CandidateDimensionID.ToString(), *InstanceId.ToString());
				
				// If dimension ID doesn't match and it's set, skip this candidate
				// If dimension ID is invalid (not set yet), we'll set it after matching
				if (CandidateDimensionID.IsValid() && CandidateDimensionID != InstanceId)
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s has wrong dimension ID"), *Candidate->GetName());
					continue;
				}
				
				// If dimension ID is not set yet, set it now (TagActorsInDimension might not have set it yet)
				if (!CandidateDimensionID.IsValid())
				{
					SaveableComp->SetDimensionInstanceId(InstanceId);
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Set dimension ID for candidate %s"), *Candidate->GetName());
				}
				
				CandidatesWithDimensionID++;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s has correct dimension ID"), *Candidate->GetName());
				
				// If actor already has a valid GUID that matches, we're done
				FGuid CandidateGUID = SaveableComp->GetPersistentId();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s GUID: %s (expected: %s)"), 
					*Candidate->GetName(), *CandidateGUID.ToString(), *ActorState.ActorId.ToString());
				
				if (CandidateGUID.IsValid() && CandidateGUID == ActorState.ActorId)
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s already has matching GUID"), *Candidate->GetName());
					Actor = Candidate;
					MatchedActors.Add(Candidate);
					break;
				}
				
				// If actor has a different GUID or no GUID, check by transform
				// Get transform to compare (use OriginalTransform if set, otherwise use current transform)
				FTransform TransformToCompare = SaveableComp->OriginalTransform;
				bool bTransformIsEmpty = TransformToCompare.GetLocation().IsNearlyZero();
				
				if (bTransformIsEmpty)
				{
					TransformToCompare = Candidate->GetActorTransform();
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s using current transform (OriginalTransform was empty)"), *Candidate->GetName());
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s using OriginalTransform"), *Candidate->GetName());
				}
				
				FVector CandidateLocation = TransformToCompare.GetLocation();
				
				// Always try converting from world space first (most common case)
				FVector CandidateLocalFromWorld = CandidateLocation - DimensionWorldPos;
				FVector DeltaAsIs = CandidateLocation - SavedLocation;
				FVector DeltaConverted = CandidateLocalFromWorld - SavedLocation;
				
				FVector CandidateLocalLocation;
				if (DeltaConverted.Size() < DeltaAsIs.Size())
				{
					CandidateLocalLocation = CandidateLocalFromWorld;
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s - using world-to-local conversion (world: %s, local: %s, delta converted: %f)"), 
						*Candidate->GetName(), *CandidateLocation.ToString(), *CandidateLocalLocation.ToString(), DeltaConverted.Size());
				}
				else
				{
					CandidateLocalLocation = CandidateLocation;
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s - using as-is (already local: %s, delta as-is: %f)"), 
						*Candidate->GetName(), *CandidateLocalLocation.ToString(), DeltaAsIs.Size());
				}
				
				FVector LocationDelta = CandidateLocalLocation - SavedLocation;
				float LocationDeltaSize = LocationDelta.Size();
				
				CandidatesWithTransformCheck++;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s transform check - saved local: %s, candidate local: %s, delta: %f (tolerance: 100.0)"), 
					*Candidate->GetName(), *SavedLocation.ToString(), *CandidateLocalLocation.ToString(), LocationDeltaSize);
				
				if (LocationDeltaSize <= 100.0f)  // 100cm tolerance
				{
					// Check rotation (within tolerance)
					FRotator RotationDelta = (TransformToCompare.GetRotation() * ActorState.OriginalSpawnTransform.GetRotation().Inverse()).Rotator();
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s rotation check - pitch: %f, yaw: %f, roll: %f (tolerance: 10.0)"), 
						*Candidate->GetName(), RotationDelta.Pitch, RotationDelta.Yaw, RotationDelta.Roll);
					
					if (FMath::Abs(RotationDelta.Pitch) <= 10.0f &&
						FMath::Abs(RotationDelta.Yaw) <= 10.0f &&
						FMath::Abs(RotationDelta.Roll) <= 10.0f)
					{
						// Match found - restore the GUID and ensure OriginalTransform and dimension ID are set
						SaveableComp->SetPersistentId(ActorState.ActorId);
						// Ensure dimension instance ID is set (it should be, but make sure)
						SaveableComp->SetDimensionInstanceId(InstanceId);
						// Set OriginalTransform in level-local space for consistency
						FTransform LocalTransform = ActorState.OriginalSpawnTransform;
						SaveableComp->OriginalTransform = LocalTransform;
						Actor = Candidate;
						MatchedActors.Add(Candidate);
						UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: *** MATCH FOUND! *** Matched and restored GUID %s to baseline actor %s (location delta: %f)"), 
							*ActorState.ActorId.ToString(), *Candidate->GetName(), LocationDeltaSize);
						break;
					}
					else
					{
						UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s location match but rotation mismatch (pitch: %f, yaw: %f, roll: %f)"), 
							*Candidate->GetName(), RotationDelta.Pitch, RotationDelta.Yaw, RotationDelta.Roll);
					}
				}
				else
				{
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Candidate %s location delta too large: %f (max: 100.0)"), 
						*Candidate->GetName(), LocationDeltaSize);
				}
			}
			
			UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] STEP 2: Summary for actor %s - Checked %d candidates, %d class matches, %d with component, %d with dimension ID, %d transform checks"), 
				*ActorState.ActorId.ToString(), CandidatesChecked, CandidatesWithClassMatch, CandidatesWithComponent, CandidatesWithDimensionID, CandidatesWithTransformCheck);
			
			if (!Actor)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] STEP 2: Could not find baseline actor %s by metadata matching"), *ActorState.ActorId.ToString());
			}
		}

		// Verify actor is in dimension level and belongs to this dimension
		if (Actor)
		{
			if (Actor->GetLevel() != DimensionLevel)
			{
				Actor = nullptr; // Actor is in wrong level
			}
			else
			{
				USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
				if (!SaveableComp || SaveableComp->GetDimensionInstanceId() != InstanceId)
				{
					Actor = nullptr; // Actor doesn't belong to this dimension
				}
				// Note: We don't check GUID here anymore because we may have just restored it
				// If the actor was found, it's the right one (either by GUID or by metadata match)
			}
		}
		
		// CRITICAL: Baseline actors should NEVER be spawned - they always exist in the level file
		// If a baseline actor is not found, it means something went wrong (maybe it was destroyed or not loaded yet)
		// We should log a warning but NOT spawn it, as it should exist in the level
		// Only spawn if it's a new object (not in baseline) AND we haven't found it yet
		
		// If it's a baseline actor and we couldn't find it, log a warning but don't spawn
		if (!Actor && bIsInBaseline)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Baseline actor %s not found in level - it should exist in the level file. Skipping restoration."), 
				*ActorState.ActorId.ToString());
			continue; // Skip this actor - it should exist in the level but we can't find it
		}
		
		// For new objects, check if they already exist before spawning (to prevent duplicates)
		// This can happen if the dimension was loaded multiple times
		bool bShouldSpawn = !Actor && bIsNewObject && (!ActorState.SpawnActorClassPath.IsEmpty() || !ActorState.ActorClassPath.IsEmpty());
		
		// Double-check: if it's a new object, make sure it doesn't already exist with a different GUID
		if (bShouldSpawn)
		{
			// Check if an actor with this class and transform already exists (might have been spawned before)
			for (AActor* ExistingActor : DimensionLevel->Actors)
			{
				if (!ExistingActor || MatchedActors.Contains(ExistingActor))
				{
					continue;
				}
				
				USaveableActorComponent* ExistingSaveable = ExistingActor->FindComponentByClass<USaveableActorComponent>();
				if (!ExistingSaveable || ExistingSaveable->GetDimensionInstanceId() != InstanceId)
				{
					continue;
				}
				
				// Check if class matches
				if (ExistingActor->GetClass()->GetPathName() != ActorState.SpawnActorClassPath && 
					ExistingActor->GetClass()->GetPathName() != ActorState.ActorClassPath)
				{
					continue;
				}
				
				// Check if transform is close (within 100cm)
				FVector ExistingLocation = ExistingActor->GetActorLocation();
				FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
				FVector DimensionWorldPos = InstanceInfo.WorldPosition;
				FVector SavedWorldLocation = ActorState.OriginalSpawnTransform.GetLocation() + DimensionWorldPos;
				FVector LocationDelta = ExistingLocation - SavedWorldLocation;
				
				if (LocationDelta.Size() <= 100.0f)
				{
					// Found existing actor - restore its GUID instead of spawning
					ExistingSaveable->SetPersistentId(ActorState.ActorId);
					ExistingSaveable->SetDimensionInstanceId(InstanceId);
					FTransform LocalTransform = ActorState.OriginalSpawnTransform;
					ExistingSaveable->OriginalTransform = LocalTransform;
					Actor = ExistingActor;
					MatchedActors.Add(ExistingActor);
					bShouldSpawn = false;
					UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Found existing new object %s, restored GUID instead of spawning"), 
						*ActorState.ActorId.ToString());
					break;
				}
			}
		}

		// Spawn new object if needed (or if baseline actor not found but we have class path)
		if (bShouldSpawn)
		{
			if (ActorState.SpawnActorClassPath.IsEmpty())
			{
				// Fallback: use ActorClassPath if SpawnActorClassPath is not set
				// This can happen if the actor was saved before the baseline was established
				if (!ActorState.ActorClassPath.IsEmpty())
				{
					UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorState.ActorClassPath);
					if (ActorClass)
					{
						FActorSpawnParameters SpawnParams;
						SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
						SpawnParams.OverrideLevel = DimensionLevel;

						Actor = World->SpawnActor<AActor>(ActorClass, ActorState.OriginalSpawnTransform, SpawnParams);
						if (Actor)
						{
							// Tag with dimension instance ID
							USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
							if (SaveableComp)
							{
								SaveableComp->SetPersistentId(ActorState.ActorId);
								SaveableComp->SetDimensionInstanceId(InstanceId);
								SaveableComp->OriginalTransform = ActorState.OriginalSpawnTransform;
							}
							NewObjectSpawnedCount++;
							UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Spawned actor %s (using ActorClassPath fallback)"), *ActorState.ActorId.ToString());
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Cannot spawn new object: No class path for actor %s"), *ActorState.ActorId.ToString());
				}
			}
			else
			{
				UClass* ActorClass = LoadClass<AActor>(nullptr, *ActorState.SpawnActorClassPath);
				if (ActorClass)
				{
					FActorSpawnParameters SpawnParams;
					SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
					SpawnParams.OverrideLevel = DimensionLevel;

					Actor = World->SpawnActor<AActor>(ActorClass, ActorState.OriginalSpawnTransform, SpawnParams);
					if (Actor)
					{
						// Tag with dimension instance ID
						USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
						if (SaveableComp)
						{
							SaveableComp->SetPersistentId(ActorState.ActorId);
							SaveableComp->SetDimensionInstanceId(InstanceId);
							SaveableComp->OriginalTransform = ActorState.OriginalSpawnTransform;
						}
						NewObjectSpawnedCount++;
						UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Spawned actor %s (using SpawnActorClassPath)"), *ActorState.ActorId.ToString());
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[SaveSystemDimension] Failed to load class: %s"), *ActorState.SpawnActorClassPath);
				}
			}
		}

		if (!Actor)
		{
			continue; // Could not find or spawn actor
		}

		// Restore actor state
		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (!SaveableComp)
		{
			continue;
		}

		// Restore GUID and dimension ID
		SaveableComp->SetPersistentId(ActorState.ActorId);
		SaveableComp->SetDimensionInstanceId(InstanceId);
		if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero())
		{
			SaveableComp->OriginalTransform = ActorState.OriginalSpawnTransform;
		}

		// Restore transform (convert from level-local to world space for streaming levels)
		if (SaveableComp->bSaveTransform)
		{
			FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
			FVector DimensionWorldPos = InstanceInfo.WorldPosition;
			FVector WorldLocation = ActorState.Location + DimensionWorldPos;
			Actor->SetActorLocation(WorldLocation);
			Actor->SetActorRotation(ActorState.Rotation);
			Actor->SetActorScale3D(ActorState.Scale);
		}

		// Restore physics state
		if (ActorState.bHasPhysics)
		{
			UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
			if (PrimitiveComp)
			{
				PrimitiveComp->SetSimulatePhysics(ActorState.bSimulatePhysics);
				if (ActorState.bSimulatePhysics && SaveableComp->bSavePhysicsState)
				{
					PrimitiveComp->SetPhysicsLinearVelocity(ActorState.LinearVelocity);
					PrimitiveComp->SetPhysicsAngularVelocityInRadians(ActorState.AngularVelocity);
				}
			}
		}

		// Restore storage component
		if (ActorState.bHasStorage)
		{
			UStorageComponent* StorageComp = Actor->FindComponentByClass<UStorageComponent>();
			if (StorageComp)
			{
				TArray<FItemEntry> StorageEntries = StorageSerialization::DeserializeStorageEntries(ActorState.SerializedStorageEntries);
				StorageComp->Entries = StorageEntries;
				StorageComp->MaxVolume = ActorState.StorageMaxVolume;
			}
		}

		// Restore ItemPickup data
		AItemPickup* ItemPickup = Cast<AItemPickup>(Actor);
		if (ItemPickup && !ActorState.SerializedItemEntry.IsEmpty())
		{
			FItemEntry ItemEntry;
			SaveSystemHelpers::DeserializeItemEntry(ActorState.SerializedItemEntry, ItemEntry);
			ItemPickup->SetItemEntry(ItemEntry);
		}

		RestoredActors.Add(Actor);
		RestoredCount++;
	}

	UE_LOG(LogTemp, Log, TEXT("[SaveSystemDimension] Loaded dimension instance %s: %d actors restored, %d new spawned, %d removed"), 
		*InstanceId.ToString(), RestoredCount, NewObjectSpawnedCount, RemovedCount);

	return true;
}

TArray<AActor*> SaveSystemDimensionHelpers::GetActorsForDimension(UWorld* World, FGuid InstanceId)
{
	TArray<AActor*> DimensionActors;

	if (!World || !InstanceId.IsValid())
	{
		return DimensionActors;
	}

	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (SaveableComp && SaveableComp->GetDimensionInstanceId() == InstanceId)
		{
			DimensionActors.Add(Actor);
		}
	}

	return DimensionActors;
}
