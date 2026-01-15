#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "Save/SaveSystemHelpers.h"
#include "Save/SaveSystemDimensionHelpers.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "Player/FirstPersonCharacter.h"
#include "Player/FirstPersonPlayerController.h"
#include "UI/LoadingFadeWidget.h"
#include "Player/HungerComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/StorageSerialization.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Dimensions/DimensionCartridgeHelpers.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "Dimensions/PortalDevice.h"
#include "Components/SaveableActorComponent.h"
#include "Components/PhysicsObjectSocketComponent.h"
#include "EngineUtils.h"
#include "Components/PrimitiveComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFile.h"
#include "UObject/Object.h"
#include "GameFramework/Actor.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"
#include "TimerManager.h"

void USaveSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentSaveData = nullptr;
	CurrentSlotId.Empty();
	CurrentSaveName.Empty();
}

FString USaveSystemSubsystem::SanitizeSaveNameForFilename(const FString& SaveName) const
{
	FString Sanitized = SaveName;
	
	// Replace invalid filename characters with underscores
	// Windows/Unreal invalid chars: < > : " / \ | ? * and control characters
	Sanitized.ReplaceInline(TEXT("<"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT(">"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT(":"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("\""), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("/"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("\\"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("|"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("?"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("*"), TEXT("_"));
	
	// Remove leading/trailing spaces and dots (Windows doesn't allow these)
	Sanitized.TrimStartAndEndInline();
	
	// Remove leading/trailing dots (Windows doesn't allow these as filenames)
	while (Sanitized.StartsWith(TEXT(".")))
	{
		Sanitized = Sanitized.RightChop(1);
	}
	while (Sanitized.EndsWith(TEXT(".")))
	{
		Sanitized = Sanitized.LeftChop(1);
	}
	
	// Limit length to avoid filesystem issues (keep reasonable filename length)
	if (Sanitized.Len() > 50)
	{
		Sanitized = Sanitized.Left(50);
	}
	
	// If empty after sanitization, use a default
	if (Sanitized.IsEmpty())
	{
		Sanitized = TEXT("Save");
	}
	
	return Sanitized;
}

FString USaveSystemSubsystem::GetSlotName(const FString& SlotId, const FString& SaveName) const
{
	FString SlotName = FString::Printf(TEXT("SaveSlot_%s"), *SlotId);
	
	// Append save name if provided (sanitized)
	if (!SaveName.IsEmpty())
	{
		FString SanitizedName = SanitizeSaveNameForFilename(SaveName);
		SlotName += FString::Printf(TEXT("_%s"), *SanitizedName);
	}
	
	return SlotName;
}

FString USaveSystemSubsystem::GetSlotIdFromSlotName(const FString& SlotName) const
{
	// Format: SaveSlot_{SlotId}_{SaveName}
	// SlotId format is always "slot_{timestamp}", so we extract the first two underscore-separated parts
	
	// Remove "SaveSlot_" prefix
	if (SlotName.StartsWith(TEXT("SaveSlot_")))
	{
		FString WithoutPrefix = SlotName.RightChop(9); // Length of "SaveSlot_"
		
		// Split by underscore - SlotId is always "slot_{timestamp}" (first two parts)
		TArray<FString> Parts;
		WithoutPrefix.ParseIntoArray(Parts, TEXT("_"), true);
		
		if (Parts.Num() >= 2)
		{
			// SlotId is "slot_{timestamp}", so combine first two parts
			return FString::Printf(TEXT("%s_%s"), *Parts[0], *Parts[1]);
		}
		
		return WithoutPrefix;
	}
	
	return SlotName;
}

bool USaveSystemSubsystem::SaveGame(const FString& SlotId, const FString& SaveName)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: SlotId is empty"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: World is null"));
		return false;
	}

	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: Player character not found"));
		return false;
	}

	// Get save slot name
	FString SlotName = GetSlotName(SlotId, SaveName);
	
	// Check if we're saving to a different slot than the current one
	bool bSavingToDifferentSlot = (SlotId != CurrentSlotId);
	
	UGameSaveData* SaveGameInstance = nullptr;
	TMap<FGuid, FActorStateSaveData> PreviousActorStates;
	
	if (CurrentSaveData && !bSavingToDifferentSlot)
	{
		// Using current save data - preserve baseline and previous actor states
		SaveGameInstance = CurrentSaveData;
		
		// Cache previous actor states for looking up metadata of removed actors
		for (const FActorStateSaveData& PreviousState : SaveGameInstance->ActorStates)
		{
			PreviousActorStates.Add(PreviousState.ActorId, PreviousState);
		}
	}
	else if (CurrentSaveData && bSavingToDifferentSlot)
	{
		// Saving to a different slot - copy current save data (including baseline) to preserve it
		SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
		if (!SaveGameInstance)
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create save game object for new slot"));
			return false;
		}
		
		// Deep copy the current save data (including baseline) - this preserves the original baseline
		SaveGameInstance->LevelPackagePath = CurrentSaveData->LevelPackagePath;
		SaveGameInstance->BaselineActorIds = CurrentSaveData->BaselineActorIds;
		
		// Copy dimension instance data - this preserves all dimension states across save slots
		SaveGameInstance->DimensionInstances = CurrentSaveData->DimensionInstances;
		SaveGameInstance->LoadedDimensionInstanceId = CurrentSaveData->LoadedDimensionInstanceId;
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Copied dimension instance data to new save slot: %d dimension instances, loaded dimension: %s"), 
			SaveGameInstance->DimensionInstances.Num(), 
			SaveGameInstance->LoadedDimensionInstanceId.IsValid() ? *SaveGameInstance->LoadedDimensionInstanceId.ToString() : TEXT("None"));
		
		// Cache previous actor states from current save
		for (const FActorStateSaveData& PreviousState : CurrentSaveData->ActorStates)
		{
			PreviousActorStates.Add(PreviousState.ActorId, PreviousState);
		}
		
	}
	else
	{
		// No current save data - try to load existing save from disk
		SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(SlotName, 0));
		
		if (SaveGameInstance)
		{
			// Loaded existing save - cache previous actor states
			for (const FActorStateSaveData& PreviousState : SaveGameInstance->ActorStates)
			{
				PreviousActorStates.Add(PreviousState.ActorId, PreviousState);
			}
		}
		else
		{
			// No existing save - create new save game object
			// Note: This should NOT happen during normal gameplay - only if loading failed or starting fresh
			// The baseline will be established below when we see BaselineActorIds is empty
			SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
			if (!SaveGameInstance)
			{
				UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create save game object"));
				return false;
			}
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Creating new save (no current save data and no existing save found on disk) - baseline will be established"));
		}
	}
	
	// Update current save data reference if we're saving to the same slot
	if (!bSavingToDifferentSlot)
	{
		CurrentSaveData = SaveGameInstance;
		CurrentSlotId = SlotId;
		CurrentSaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %s"), *SlotId) : SaveName;
	}

	// Save player location and rotation (legacy fields for backward compatibility)
	SaveGameInstance->PlayerLocation = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerRotation = PlayerCharacter->GetActorRotation();
	SaveGameInstance->SaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %s"), *SlotId) : SaveName;

	// === Save Player Data ===
	SaveGameInstance->PlayerData.Location = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerData.Rotation = PlayerCharacter->GetActorRotation();
	
	// Save hunger stats
	if (UHungerComponent* HungerComp = PlayerCharacter->GetHunger())
	{
		SaveGameInstance->PlayerData.CurrentHunger = HungerComp->GetCurrentHunger();
		SaveGameInstance->PlayerData.MaxHunger = HungerComp->GetMaxHunger();
	}
	
	// Save inventory
	if (UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory())
	{
		SaveGameInstance->InventoryData.SerializedEntries = 
			StorageSerialization::SerializeStorageEntries(InventoryComp->GetEntries());
	}

	// Save hotbar
	if (UHotbarComponent* HotbarComp = PlayerCharacter->GetHotbar())
	{
		SaveGameInstance->HotbarData.ActiveIndex = HotbarComp->GetActiveIndex();
		SaveGameInstance->HotbarData.AssignedItemPaths.Empty();
		
		for (int32 i = 0; i < HotbarComp->GetNumSlots(); ++i)
		{
			const FHotbarSlot& Slot = HotbarComp->GetSlot(i);
			if (Slot.AssignedType)
			{
				SaveGameInstance->HotbarData.AssignedItemPaths.Add(Slot.AssignedType->GetPathName());
			}
			else
			{
				SaveGameInstance->HotbarData.AssignedItemPaths.Add(FString()); // Empty string for unassigned slots
			}
		}
	}

	// Save equipment
	if (UEquipmentComponent* EquipmentComp = PlayerCharacter->GetEquipment())
	{
		SaveGameInstance->EquipmentData.SerializedEquippedItems.Empty();
		TMap<EEquipmentSlot, FItemEntry> EquippedItems = EquipmentComp->GetAllEquippedItems();
		
		for (const auto& Pair : EquippedItems)
		{
			FString Serialized = SaveSystemHelpers::SerializeEquipmentSlot(Pair.Key, Pair.Value);
			SaveGameInstance->EquipmentData.SerializedEquippedItems.Add(Serialized);
		}
	}

	// Save current level package path (for standalone builds, use package path)
	// Get the level's package - this works in both editor and standalone
	FString LevelPackagePath;
	
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			// Get the package from the level's outer
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				LevelPackagePath = LevelPackage->GetName();
			}
		}
	}
	
	// Fallback: try to get from world's outer
	if (LevelPackagePath.IsEmpty())
	{
		if (UObject* WorldOuter = World->GetOuter())
		{
			if (UPackage* WorldPackage = WorldOuter->GetPackage())
			{
				LevelPackagePath = WorldPackage->GetName();
			}
		}
	}
	
	// Last resort: use map name and construct package path
	if (LevelPackagePath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			// Construct package path (e.g., "/Game/Levels/MainMap")
			LevelPackagePath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			// Absolute fallback: use world path
			FString WorldPath = World->GetPathName();
			int32 DotIndex = WorldPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (DotIndex != INDEX_NONE)
			{
				LevelPackagePath = WorldPath.Left(DotIndex);
			}
			else
			{
				LevelPackagePath = WorldPath;
			}
		}
	}
	
	// Normalize the level path by removing PIE prefixes (e.g., "UEDPIE_0_MainMap" -> "MainMap")
	// This ensures saves work in both PIE and standalone builds
	FString NormalizedPath = LevelPackagePath;
	
	// Remove PIE prefix if present (e.g., "/Game/Levels/UEDPIE_0_MainMap" -> "/Game/Levels/MainMap")
	// PIE prefixes can be like "UEDPIE_0_", "UEDPIE_1_", etc.
	if (NormalizedPath.Contains(TEXT("UEDPIE_")))
	{
		// Find the last slash before the level name
		int32 LastSlash = NormalizedPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (LastSlash != INDEX_NONE)
		{
			FString PathPrefix = NormalizedPath.Left(LastSlash + 1); // Include the slash
			FString LevelName = NormalizedPath.RightChop(LastSlash + 1);
			
			// Remove PIE prefix from level name (e.g., "UEDPIE_0_MainMap" -> "MainMap")
			// PIE prefix format is typically "UEDPIE_X_" where X is a number
			if (LevelName.StartsWith(TEXT("UEDPIE_")))
			{
				// Find the first underscore after "UEDPIE"
				int32 FirstUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
				if (FirstUnderscore != INDEX_NONE)
				{
					// Find the second underscore (after the number)
					int32 SecondUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstUnderscore + 1);
					if (SecondUnderscore != INDEX_NONE)
					{
						// Extract everything after the PIE prefix
						LevelName = LevelName.RightChop(SecondUnderscore + 1);
						NormalizedPath = PathPrefix + LevelName;
					}
				}
			}
		}
	}
	
	SaveGameInstance->LevelPackagePath = NormalizedPath;
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Saving level package path: %s (original: %s, MapName: %s)"), 
		*NormalizedPath, *LevelPackagePath, *World->GetMapName());

	// Save which dimension instance is currently loaded (if any)
	// Note: If a dimension was loaded but unloaded before this save, LoadedDimensionInstanceId
	// may have already been set when the dimension was saved. We should preserve that value.
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
		if (DimensionManager)
		{
			// Save player's dimension instance ID (which dimension the player is currently in)
			SaveGameInstance->PlayerData.PlayerDimensionInstanceId = DimensionManager->GetPlayerDimensionInstanceId();
			if (SaveGameInstance->PlayerData.PlayerDimensionInstanceId.IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Saved player dimension instance ID: %s"), 
					*SaveGameInstance->PlayerData.PlayerDimensionInstanceId.ToString());
			}
			
			// Save loaded dimension instance ID and dimension state
			if (DimensionManager->IsDimensionLoaded())
			{
				// Dimension is currently loaded - save its instance ID
				FGuid CurrentInstanceId = DimensionManager->GetCurrentInstanceId();
				SaveGameInstance->LoadedDimensionInstanceId = CurrentInstanceId;
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Saved loaded dimension instance: %s"), 
					*SaveGameInstance->LoadedDimensionInstanceId.ToString());
				
				// CRITICAL: Save the dimension instance state (actor states, baseline, etc.)
				// This ensures dimension state is saved when saving through pause menu
				SaveDimensionInstance(CurrentInstanceId);
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Saved dimension instance state for %s"), 
					*CurrentInstanceId.ToString());
			}
			else if (!SaveGameInstance->LoadedDimensionInstanceId.IsValid())
			{
				// Dimension is not loaded AND we don't have a saved value - clear it
				SaveGameInstance->LoadedDimensionInstanceId = FGuid();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] No dimension loaded when saving"));
			}
			else
			{
				// Dimension is not currently loaded, but we have a saved value from when it was saved
				// Preserve it - this means a dimension was loaded when saving but unloaded before main save
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Dimension not currently loaded, but preserving saved loaded dimension instance: %s"), 
					*SaveGameInstance->LoadedDimensionInstanceId.ToString());
			}
		}
	}

	// === Unified Actor State Tracking ===
	// Note: PreviousActorStates was already cached above when loading the save
	// Save all actors with SaveableActorComponent and their current state
	SaveGameInstance->ActorStates.Empty();
	TArray<FGuid> CurrentActorIds;
	
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		// Check if actor has SaveableActorComponent
		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (!SaveableComp)
		{
			continue;
		}

		// CRITICAL: Skip actors that belong to a dimension - they are saved separately in DimensionInstances
		// Dimension actors should NEVER be saved to the main world's ActorStates
		FGuid ActorDimensionInstanceId = SaveableComp->GetDimensionInstanceId();
		if (ActorDimensionInstanceId.IsValid())
		{
			// This actor belongs to a dimension - skip it
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
		
		// Save actor identification metadata (for fallback matching)
		ActorState.ActorName = Actor->GetName();
		ActorState.ActorClassPath = Actor->GetClass()->GetPathName();
		
		// Get OriginalTransform - if not set yet, use current transform
		FTransform OriginalTransformToSave = SaveableComp->OriginalTransform;
		if (OriginalTransformToSave.GetLocation().IsNearlyZero())
		{
			// OriginalTransform not set yet - use current transform and set it
			OriginalTransformToSave = Actor->GetActorTransform();
			SaveableComp->OriginalTransform = OriginalTransformToSave;
		}
		ActorState.OriginalSpawnTransform = OriginalTransformToSave;
		
		// Check if this is a new object (not in baseline)
		// A new object is one that was added after the baseline was established
		// If baseline is empty, this is the first save, so all actors are part of the baseline (not new objects)
		bool bIsInBaseline = SaveGameInstance->BaselineActorIds.Num() > 0 && 
			SaveGameInstance->BaselineActorIds.Contains(ActorId);
		bool bIsNewObject = SaveGameInstance->BaselineActorIds.Num() > 0 && !bIsInBaseline;
		
		if (bIsNewObject)
		{
			// This is a new object added after baseline was established (e.g., dropped item)
			ActorState.bIsNewObject = true;
			ActorState.SpawnActorClassPath = Actor->GetClass()->GetPathName();
			
		}
		
		// Save ItemEntry data for ALL ItemPickup actors (both new and existing)
		// This ensures that changes to world item pickups (like partially eaten food) are persisted
		if (AItemPickup* ItemPickup = Cast<AItemPickup>(Actor))
		{
			if (UItemDefinition* ItemDef = ItemPickup->GetItemDef())
			{
				ActorState.ItemDefinitionPath = ItemDef->GetPathName();
				
				// Serialize ItemEntry (includes CustomData like UsesRemaining)
				FItemEntry ItemEntry = ItemPickup->GetItemEntry();
				ActorState.SerializedItemEntry = SaveSystemHelpers::SerializeItemEntry(ItemEntry);
			}
		}
		
		// Save transform if enabled
		if (SaveableComp->bSaveTransform)
		{
			ActorState.Location = Actor->GetActorLocation();
			ActorState.Rotation = Actor->GetActorRotation();
			ActorState.Scale = Actor->GetActorScale3D();
		}

		// Check for physics component
		UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
		{
			// Only save physics objects that have moved from their original position
			if (SaveSystemHelpers::ShouldSavePhysicsObject(Actor, SaveableComp->OriginalTransform))
			{
				ActorState.bHasPhysics = true;
				ActorState.bSimulatePhysics = true;
				
				if (SaveableComp->bSavePhysicsState)
				{
					ActorState.LinearVelocity = PrimitiveComp->GetPhysicsLinearVelocity();
					ActorState.AngularVelocity = PrimitiveComp->GetPhysicsAngularVelocityInRadians();
				}
			}
		}

		// Check for storage component
		UStorageComponent* StorageComp = Actor->FindComponentByClass<UStorageComponent>();
		if (StorageComp)
		{
			ActorState.bHasStorage = true;
			TArray<FItemEntry> StorageEntries = StorageComp->GetEntries();
			ActorState.SerializedStorageEntries = StorageSerialization::SerializeStorageEntries(StorageEntries);
			ActorState.StorageMaxVolume = StorageComp->MaxVolume;
		}

		SaveGameInstance->ActorStates.Add(ActorState);
	}

	// === Detect Removed Actors ===
	// Compare current actors to baseline to detect which ones were removed
	if (SaveGameInstance->BaselineActorIds.Num() > 0)
	{
		// This is a subsequent save - compare to baseline
		int32 RemovedCount = 0;
		for (const FGuid& BaselineId : SaveGameInstance->BaselineActorIds)
		{
			if (!CurrentActorIds.Contains(BaselineId))
			{
				// Actor was in baseline but not in current state - it was removed
				// Try to find it in the world to get its last known location and metadata (for matching on load)
				AActor* RemovedActor = USaveableActorComponent::FindActorByGuid(World, BaselineId);
				FActorStateSaveData RemovedActorState;
				RemovedActorState.ActorId = BaselineId;
				RemovedActorState.bExists = false;
				
				if (RemovedActor)
				{
					// Actor still exists in world - save current metadata
					RemovedActorState.ActorName = RemovedActor->GetName();
					RemovedActorState.ActorClassPath = RemovedActor->GetClass()->GetPathName();
					RemovedActorState.Location = RemovedActor->GetActorLocation();
					RemovedActorState.Rotation = RemovedActor->GetActorRotation();
					RemovedActorState.Scale = RemovedActor->GetActorScale3D();
					
					// Try to get original transform from SaveableComponent if it exists
					if (USaveableActorComponent* RemovedSaveable = RemovedActor->FindComponentByClass<USaveableActorComponent>())
					{
						RemovedActorState.OriginalSpawnTransform = RemovedSaveable->OriginalTransform;
					}
				}
				else
				{
					// Actor was destroyed - look up metadata from previous save
					if (const FActorStateSaveData* PreviousState = PreviousActorStates.Find(BaselineId))
					{
						// Use metadata from previous save
						RemovedActorState.ActorName = PreviousState->ActorName;
						RemovedActorState.ActorClassPath = PreviousState->ActorClassPath;
						RemovedActorState.OriginalSpawnTransform = PreviousState->OriginalSpawnTransform;
						// Use last known location from previous save
						RemovedActorState.Location = PreviousState->Location;
						RemovedActorState.Rotation = PreviousState->Rotation;
						RemovedActorState.Scale = PreviousState->Scale;
					}
					else
					{
						// No previous state found - this shouldn't happen if baseline was established correctly
						UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Marked actor %s as removed but could not find previous metadata (baseline issue?)"), 
							*BaselineId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
					}
				}
				
				SaveGameInstance->ActorStates.Add(RemovedActorState);
				RemovedCount++;
			}
		}
	}
	else
	{
		// No baseline exists - this should not happen during normal gameplay
		// Baseline should only be created in CreateNewGameSave() when starting a new game
		// If we're here, either:
		// 1. We're loading a very old save without a baseline (shouldn't happen)
		// 2. Something went wrong with save/load
		// For safety, we'll establish a baseline, but log a warning
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] No baseline found in save - establishing new baseline (this should not happen during normal gameplay!)"));
		SaveGameInstance->BaselineActorIds = CurrentActorIds;
	}


	// Set timestamp
	FDateTime Now = FDateTime::Now();
	SaveGameInstance->Timestamp = Now.ToString(TEXT("%Y.%m.%d %H:%M:%S"));

	// Check if we need to save a temp save for level transition
	if (!PendingLevelLoad.IsEmpty())
	{
		FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Saving temp save with inventory entries: %d, hotbar slots: %d, equipment: %d"), 
			SaveGameInstance->InventoryData.SerializedEntries.Len() > 0 ? 1 : 0,
			SaveGameInstance->HotbarData.AssignedItemPaths.Num(),
			SaveGameInstance->EquipmentData.SerializedEquippedItems.Num());
		bool bTempSaveSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, TempSlotName, 0);
		if (bTempSaveSuccess)
		{
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Temp save created successfully"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create temp save!"));
		}
		
			// Open the level
			FString LevelToLoad = PendingLevelLoad;
			PendingLevelLoad.Empty();
			UGameplayStatics::OpenLevel(World, *LevelToLoad);
			
		return true;
	}

	// Save to slot (SlotName was already computed above when we tried to load the save)
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, 0);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully saved game to slot: %s"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to save game to slot: %s"), *SlotName);
	}

	return bSuccess;
}

bool USaveSystemSubsystem::LoadGame(const FString& SlotId)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: SlotId is empty"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: World is null"));
		return false;
	}

	// Fade to black before loading (hide items spawning and position changes)
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
	if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
	{
		// Fade out quickly (0.3 seconds) to hide the loading process
		FirstPersonPC->LoadingFadeWidget->FadeOut(0.3f);
		// Wait for fade to complete before proceeding with load
		// We'll use a timer to delay the actual loading
		const float FadeOutDuration = 0.3f;
		FTimerHandle FadeOutTimer;
		World->GetTimerManager().SetTimer(FadeOutTimer, [this, World, SlotId]()
		{
			LoadGameAfterFade(SlotId);
		}, FadeOutDuration, false);
		
		return true; // Return early, actual load happens after fade
	}
	else
	{
		// No fade widget available, proceed with normal load
		return LoadGameAfterFade(SlotId);
	}
}

bool USaveSystemSubsystem::LoadGameAfterFade(const FString& SlotId)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: World is null"));
		return false;
	}

	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	UGameSaveData* SaveGameInstance = nullptr;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - try to load it
			SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0));
			if (SaveGameInstance)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to load save game from slot: %s"), *SlotId);
		return false;
	}
	
	// Set as current save data (the "running" save game)
	CurrentSaveData = SaveGameInstance;
	CurrentSlotId = SlotId;
	CurrentSaveName = SaveGameInstance->SaveName;
	

	// Check if we need to load a different level
	FString CurrentLevelPath;
	
	// Get current level package path (same logic as saving)
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				CurrentLevelPath = LevelPackage->GetName();
			}
		}
	}
	
	if (CurrentLevelPath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			CurrentLevelPath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			FString WorldPath = World->GetPathName();
			int32 DotIndex = WorldPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (DotIndex != INDEX_NONE)
			{
				CurrentLevelPath = WorldPath.Left(DotIndex);
			}
			else
			{
				CurrentLevelPath = WorldPath;
			}
		}
	}

	// Always load the save's level if we have a level path in the save
	// This ensures we load the correct level even if the current level detection fails
	bool bShouldLoadLevel = !SaveGameInstance->LevelPackagePath.IsEmpty();
	
	// If the save is from a different level, or we're loading from main menu (which might not detect correctly),
	// always load the save's level
	if (bShouldLoadLevel)
	{
		// Check if we're in MainMenu - if so, always load the save's level
		FString MapName = World->GetMapName();
		bool bIsMainMenu = MapName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase);
		
		// Load level if we're in a different level OR if we're in MainMenu (always load from main menu)
		if (bIsMainMenu || CurrentLevelPath != SaveGameInstance->LevelPackagePath)
		{
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Loading level: %s (current: %s)"), 
				*SaveGameInstance->LevelPackagePath, *CurrentLevelPath);
			
			// Convert package path to level name for OpenLevel
			// OpenLevel expects the level name without the package path prefix
			FString LevelName = SaveGameInstance->LevelPackagePath;
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Original saved level path: %s"), *LevelName);
			
			// Remove "/Game/" prefix if present for OpenLevel
			if (LevelName.StartsWith(TEXT("/Game/")))
			{
				LevelName = LevelName.RightChop(6); // Remove "/Game/"
			}
			// Remove leading slash if present
			if (LevelName.StartsWith(TEXT("/")))
			{
				LevelName = LevelName.RightChop(1);
			}
			
			// If LevelName still contains "Levels/", extract just the level name
			// For example: "Levels/MainMap" -> "MainMap"
			if (LevelName.Contains(TEXT("/")))
			{
				int32 LastSlash = LevelName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (LastSlash != INDEX_NONE)
				{
					LevelName = LevelName.RightChop(LastSlash + 1); // Get everything after the last slash
				}
			}
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Converted level name for OpenLevel: %s"), *LevelName);
			
			// Store save data for restoration after level loads
			// (We handle the level transition directly here, not through SaveGame())
			FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
		bool bTempSaveSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, TempSlotName, 0);
		if (!bTempSaveSuccess)
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create temp save!"));
		}
			
			// Ensure fade widget is black before opening level to prevent flash
			// The widget should already be black from the fade-out, but ensure it stays black
			APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
			AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
			if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
			{
				// Force opacity to 1.0 (fully black) before level transition
				FirstPersonPC->LoadingFadeWidget->SetOpacity(1.0f);
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Set fade widget to black before level transition"));
			}
			
			// Open the level - player position will be restored after level loads
			UGameplayStatics::OpenLevel(World, *LevelName);
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Opened level: %s - position will be restored after load"), *LevelName);
			
			// Clear PendingLevelLoad since we've already opened the level
			// (PendingLevelLoad was only set for potential use, but we handle it directly here)
			PendingLevelLoad.Empty();
			
			// Position restoration will be handled in FirstPersonPlayerController::BeginPlay
			// by checking for the temp save file
			
			return true;
		}
	}

	// Same level - restore all game state
	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: Player character not found"));
		return false;
	}

	// Restore player location and rotation
	// Check if new save data exists (check if inventory data was saved as a proxy)
	bool bHasNewSaveData = !SaveGameInstance->InventoryData.SerializedEntries.IsEmpty() || 
		SaveGameInstance->HotbarData.AssignedItemPaths.Num() > 0 ||
		SaveGameInstance->EquipmentData.SerializedEquippedItems.Num() > 0;
	
	FVector RestoreLocation;
	FRotator RestoreRotation;
	
	if (bHasNewSaveData)
	{
		// Use new save data
		RestoreLocation = SaveGameInstance->PlayerData.Location;
		RestoreRotation = SaveGameInstance->PlayerData.Rotation;
	}
	else
	{
		// Fallback to legacy data
		RestoreLocation = SaveGameInstance->PlayerLocation;
		RestoreRotation = SaveGameInstance->PlayerRotation;
	}
	
	// If player was in a dimension when saving, delay position restoration until dimension loads
	// This prevents the player from spawning in main world before dimension appears
	bool bPlayerWasInDimension = SaveGameInstance->LoadedDimensionInstanceId.IsValid();
	if (bPlayerWasInDimension)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Player was in dimension when saving - delaying position restoration until dimension loads"));
		// Position will be restored after dimension loads (via RestoreLoadedDimension callback)
	}
	else
	{
		// Player was in main world - restore position immediately
		PlayerCharacter->SetActorLocation(RestoreLocation);
		PlayerCharacter->SetActorRotation(RestoreRotation);
	}

	// Only restore new save data if it exists
	if (!bHasNewSaveData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] No new save data found, only restoring position"));
		return true;
	}

	// Restore player's dimension instance ID (which dimension the player is currently in)
	// This must be done BEFORE dimension loading so items dropped get the correct dimension ID
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
		if (DimensionManager && SaveGameInstance->PlayerData.PlayerDimensionInstanceId.IsValid())
		{
			DimensionManager->SetPlayerDimensionInstanceId(SaveGameInstance->PlayerData.PlayerDimensionInstanceId);
			UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored player dimension instance ID: %s"), 
				*SaveGameInstance->PlayerData.PlayerDimensionInstanceId.ToString());
		}
	}
	
	// Restore hunger stats
	if (UHungerComponent* HungerComp = PlayerCharacter->GetHunger())
	{
		float SavedHunger = SaveGameInstance->PlayerData.CurrentHunger;
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
				HungerComp->OnHungerChanged.Broadcast(SavedHunger, SaveGameInstance->PlayerData.MaxHunger);
			}
		}
		else
		{
			// Fallback: use RestoreHunger if reflection fails
			HungerComp->RestoreHunger(HungerDelta);
		}
		
	}

	// Restore inventory
	if (UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory())
	{
		TArray<FItemEntry> RestoredEntries = StorageSerialization::DeserializeStorageEntries(
			SaveGameInstance->InventoryData.SerializedEntries);
		
		// Clear existing inventory and add restored entries
		// Note: We need to remove items one by one since there's no Clear() method
		TArray<FGuid> ItemIdsToRemove;
		for (const FItemEntry& Entry : InventoryComp->GetEntries())
		{
			ItemIdsToRemove.Add(Entry.ItemId);
		}
		for (const FGuid& ItemId : ItemIdsToRemove)
		{
			InventoryComp->RemoveById(ItemId);
		}

		// Add restored entries
		for (const FItemEntry& Entry : RestoredEntries)
		{
			InventoryComp->TryAdd(Entry);
		}
	}

	// Restore hotbar
	if (UHotbarComponent* HotbarComp = PlayerCharacter->GetHotbar())
	{
		// Clear all slots first
		for (int32 i = 0; i < HotbarComp->GetNumSlots(); ++i)
		{
			HotbarComp->ClearSlot(i);
		}

		// Restore assigned item types
		for (int32 i = 0; i < SaveGameInstance->HotbarData.AssignedItemPaths.Num() && 
			i < HotbarComp->GetNumSlots(); ++i)
		{
			const FString& ItemPath = SaveGameInstance->HotbarData.AssignedItemPaths[i];
			if (!ItemPath.IsEmpty())
			{
				UItemDefinition* ItemDef = LoadObject<UItemDefinition>(nullptr, *ItemPath);
				if (ItemDef)
				{
					HotbarComp->AssignSlot(i, ItemDef);
				}
			}
		}

		// Restore active index (this will also select the item if available)
		if (SaveGameInstance->HotbarData.ActiveIndex != INDEX_NONE && 
			SaveGameInstance->HotbarData.ActiveIndex >= 0 && 
			SaveGameInstance->HotbarData.ActiveIndex < HotbarComp->GetNumSlots())
		{
			HotbarComp->SelectSlot(SaveGameInstance->HotbarData.ActiveIndex, PlayerCharacter->GetInventory());
		}
	}

	// Restore equipment (must be after inventory is restored)
	if (UEquipmentComponent* EquipmentComp = PlayerCharacter->GetEquipment())
	{
		for (const FString& SerializedEquipment : SaveGameInstance->EquipmentData.SerializedEquippedItems)
		{
			EEquipmentSlot Slot;
			FItemEntry Entry;
			if (SaveSystemHelpers::DeserializeEquipmentSlot(SerializedEquipment, Slot, Entry))
			{
				// First, ensure the item is in inventory (it should be, but check)
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

					// If not in inventory, add it
					if (!bItemInInventory)
					{
						InventoryComp->TryAdd(Entry);
					}

					// Now equip it
					FText ErrorText;
					if (EquipmentComp->EquipFromInventory(Entry.ItemId, ErrorText))
					{
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to restore equipment in slot %d: %s"), 
							static_cast<int32>(Slot), *ErrorText.ToString());
					}
				}
			}
		}
	}

	// Restore all actor states
	int32 ActorRestoredCount = 0;
	int32 ActorNotFoundCount = 0;
	int32 PhysicsRestoredCount = 0;
	int32 StorageRestoredCount = 0;
	
	for (const FActorStateSaveData& ActorState : SaveGameInstance->ActorStates)
	{
		AActor* Actor = USaveableActorComponent::FindActorByGuid(World, ActorState.ActorId);
		if (!Actor)
		{
			ActorNotFoundCount++;
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Actor with ID %s not found in world"), 
				*ActorState.ActorId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			continue;
		}

		// Restore transform
		Actor->SetActorLocation(ActorState.Location);
		Actor->SetActorRotation(ActorState.Rotation);
		Actor->SetActorScale3D(ActorState.Scale);

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
				
				// Clear existing storage
				TArray<FGuid> ItemIdsToRemove;
				for (const FItemEntry& Entry : StorageComp->GetEntries())
				{
					ItemIdsToRemove.Add(Entry.ItemId);
				}
				for (const FGuid& ItemId : ItemIdsToRemove)
				{
					StorageComp->RemoveById(ItemId);
				}

				// Restore entries
				int32 AddedCount = 0;
				for (const FItemEntry& Entry : RestoredEntries)
				{
					if (StorageComp->TryAdd(Entry))
					{
						AddedCount++;
					}
				}

				StorageComp->MaxVolume = ActorState.StorageMaxVolume;
				StorageRestoredCount++;
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
	if (SaveGameInstance && PlayerCharacter)
	{
		RestoreCartridgeInstanceIds(World, SaveGameInstance, PlayerCharacter);
	}

	// NOTE: Dimension restoration and fade-in are handled by RestorePlayerPositionAfterLevelLoad
	// when a level transition occurs. For same-level loads, we handle it here.
	// But if we're doing a level transition, we skip this to avoid double fade-in.
	// The level transition path will handle dimension restoration via RestorePlayerPositionAfterLevelLoad.
	
	// Check if we're doing a level transition (temp save exists means level transition is happening)
	FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
	bool bLevelTransition = UGameplayStatics::DoesSaveGameExist(TempSlotName, 0);
	
	if (!bLevelTransition)
	{
		// Same-level load - handle dimension restoration and fade-in here
		if (SaveGameInstance && SaveGameInstance->LoadedDimensionInstanceId.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Will restore loaded dimension instance: %s (same-level load)"), 
				*SaveGameInstance->LoadedDimensionInstanceId.ToString());
			FTimerHandle RestoreDimensionTimer;
			
			// Store player position/rotation for restoration after dimension loads
			FVector SavedPlayerLocation = RestoreLocation;
			FRotator SavedPlayerRotation = RestoreRotation;
			
			// Fade in after dimension loads and player position is restored
			TFunction<void()> FadeInCallback = [this]()
			{
				FadeInAfterLoad();
			};
			
			World->GetTimerManager().SetTimer(RestoreDimensionTimer, [this, World, SaveGameInstance, SavedPlayerLocation, SavedPlayerRotation, FadeInCallback]()
			{
				RestoreLoadedDimension(World, SaveGameInstance, SavedPlayerLocation, SavedPlayerRotation, true, FadeInCallback);
			}, 0.2f, false); // Reduced delay: 0.2 seconds should be enough for cartridges to restore
		}
		else
		{
			// No dimension to load - fade in after loading completes (for same-level loads)
			// Add a small delay to ensure everything is visually settled before fading in
			if (World)
			{
				FTimerHandle FadeInTimer;
				World->GetTimerManager().SetTimer(FadeInTimer, [this]()
				{
					FadeInAfterLoad();
				}, 0.2f, false); // 0.2 second delay before fading in
			}
		}
	}
	else
	{
		// Level transition - RestorePlayerPositionAfterLevelLoad will handle dimension restoration and fade-in
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Level transition detected - dimension restoration will be handled by RestorePlayerPositionAfterLevelLoad"));
	}
	
	return true;
}

bool USaveSystemSubsystem::DeleteSave(const FString& SlotId)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot delete: SlotId is empty"));
		return false;
	}

	// Find and delete the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	bool bSuccess = false;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - delete it
			bSuccess = UGameplayStatics::DeleteGameInSlot(FileName, 0);
			if (bSuccess)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to delete save slot: %s"), *SlotName);
	}

	return bSuccess;
}

FSaveSlotInfo USaveSystemSubsystem::GetSaveSlotInfo(const FString& SlotId) const
{
	FSaveSlotInfo Info;
	Info.SlotId = SlotId;

	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	UGameSaveData* SaveGameInstance = nullptr;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - try to load it
			SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0));
			if (SaveGameInstance)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (SaveGameInstance)
	{
		Info.bExists = true;
		Info.SaveName = SaveGameInstance->GetSaveName();
		Info.Timestamp = SaveGameInstance->GetFormattedTimestamp();
	}
	else
	{
		Info.bExists = false;
	}

	return Info;
}

bool USaveSystemSubsystem::DoesSaveSlotExist(const FString& SlotId) const
{
	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file
			return true;
		}
	}
	
	return false;
}

TArray<FSaveSlotInfo> USaveSystemSubsystem::GetAllSaveSlots() const
{
	TArray<FSaveSlotInfo> SaveSlots;

	// Enumerate save files from the save directory
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	
	if (!FileManager.DirectoryExists(*SaveDir))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Save directory does not exist: %s"), *SaveDir);
		return SaveSlots;
	}

	// Use IFileManager::FindFiles - this requires full file paths with wildcards
	// FindFiles signature: FindFiles(OutFileNames, StartDirectory, FileExtension)
	// But IFileManager::FindFiles works differently - it takes a search pattern
	TArray<FString> SaveFiles;
	
	// Try using the correct IFileManager::FindFiles signature
	// Parameters: OutFileNames, StartDirectory, FileExtension, bFiles, bDirectories, bRecursive
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));

	// If FindFiles still returns 0, try using IterateDirectory with a visitor
	if (SaveFiles.Num() == 0)
	{
		
		// Manually iterate through directory to find all .sav files
		struct FSaveFileVisitor : public IPlatformFile::FDirectoryVisitor
		{
			TArray<FString>& FoundFiles;
			FString SaveDirPath;
			
			FSaveFileVisitor(TArray<FString>& InFoundFiles, const FString& InSaveDir)
				: FoundFiles(InFoundFiles), SaveDirPath(InSaveDir)
			{}
			
			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
			{
				if (!bIsDirectory)
				{
					FString FilePath(FilenameOrDirectory);
					if (FilePath.EndsWith(TEXT(".sav")))
					{
						FString RelativePath = FilePath;
						FPaths::MakePathRelativeTo(RelativePath, *(SaveDirPath / TEXT("")));
						FoundFiles.Add(RelativePath);
					}
				}
				return true;
			}
		};
		
		FSaveFileVisitor Visitor(SaveFiles, SaveDir);
		FileManager.IterateDirectory(*SaveDir, Visitor);
		
	}

	// Process each save file
	for (const FString& SaveFile : SaveFiles)
	{
		// Get the base filename without extension and path
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		
		
		// Check if it's a save slot (starts with "SaveSlot_")
		if (FileName.StartsWith(TEXT("SaveSlot_")))
		{
			FString SlotId = GetSlotIdFromSlotName(FileName);
			
			FSaveSlotInfo Info = GetSaveSlotInfo(SlotId);
			if (Info.bExists)
			{
				SaveSlots.Add(Info);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Save slot file exists but failed to load: %s"), *SlotId);
			}
		}
		else
		{
		}
	}


	// Sort by timestamp (most recent first)
	SaveSlots.Sort([](const FSaveSlotInfo& A, const FSaveSlotInfo& B) {
		return A.Timestamp > B.Timestamp;
	});

	return SaveSlots;
}

FSaveSlotInfo USaveSystemSubsystem::GetMostRecentSaveSlot() const
{
	TArray<FSaveSlotInfo> AllSlots = GetAllSaveSlots();
	
	if (AllSlots.Num() > 0)
	{
		return AllSlots[0]; // Already sorted by timestamp
	}

	return FSaveSlotInfo();
}

bool USaveSystemSubsystem::CreateNewGameSave(const FString& SlotId, const FString& SaveName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot create new game save: World is null"));
		return false;
	}

	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot create new game save: Player character not found"));
		return false;
	}

	// Get save slot name
	FString SlotName = GetSlotName(SlotId, SaveName);
	
	// Create new save game object - this is for a new game, so we'll create a new baseline
	UGameSaveData* SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create save game object"));
		return false;
	}

	// Set as current save data (the "running" save game)
	CurrentSaveData = SaveGameInstance;
	CurrentSlotId = SlotId;
	CurrentSaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %s"), *SlotId) : SaveName;

	// Save player location and rotation (current spawn location)
	SaveGameInstance->PlayerLocation = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerRotation = PlayerCharacter->GetActorRotation();
	SaveGameInstance->SaveName = CurrentSaveName;

	// Save player data (same as SaveGame)
	SaveGameInstance->PlayerData.Location = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerData.Rotation = PlayerCharacter->GetActorRotation();
	
	// Save hunger stats
	if (UHungerComponent* HungerComp = PlayerCharacter->GetHunger())
	{
		SaveGameInstance->PlayerData.CurrentHunger = HungerComp->GetCurrentHunger();
		SaveGameInstance->PlayerData.MaxHunger = HungerComp->GetMaxHunger();
	}
	
	// Save player's dimension instance ID (which dimension the player is currently in)
	UGameInstance* GameInstance = World->GetGameInstance();
	if (GameInstance)
	{
		UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
		if (DimensionManager)
		{
			SaveGameInstance->PlayerData.PlayerDimensionInstanceId = DimensionManager->GetPlayerDimensionInstanceId();
			if (SaveGameInstance->PlayerData.PlayerDimensionInstanceId.IsValid())
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Saved player dimension instance ID: %s"), 
					*SaveGameInstance->PlayerData.PlayerDimensionInstanceId.ToString());
			}
		}
	}

	// Save inventory
	if (UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory())
	{
		SaveGameInstance->InventoryData.SerializedEntries = 
			StorageSerialization::SerializeStorageEntries(InventoryComp->GetEntries());
	}

	// Save hotbar
	if (UHotbarComponent* HotbarComp = PlayerCharacter->GetHotbar())
	{
		SaveGameInstance->HotbarData.ActiveIndex = HotbarComp->GetActiveIndex();
		SaveGameInstance->HotbarData.AssignedItemPaths.Empty();
		
		for (int32 i = 0; i < HotbarComp->GetNumSlots(); ++i)
		{
			const FHotbarSlot& Slot = HotbarComp->GetSlot(i);
			if (Slot.AssignedType)
			{
				SaveGameInstance->HotbarData.AssignedItemPaths.Add(Slot.AssignedType->GetPathName());
			}
			else
			{
				SaveGameInstance->HotbarData.AssignedItemPaths.Add(FString()); // Empty string for unassigned slots
			}
		}
	}

	// Save equipment
	if (UEquipmentComponent* EquipmentComp = PlayerCharacter->GetEquipment())
	{
		SaveGameInstance->EquipmentData.SerializedEquippedItems.Empty();
		TMap<EEquipmentSlot, FItemEntry> EquippedItems = EquipmentComp->GetAllEquippedItems();
		
		for (const auto& Pair : EquippedItems)
		{
			FString Serialized = SaveSystemHelpers::SerializeEquipmentSlot(Pair.Key, Pair.Value);
			SaveGameInstance->EquipmentData.SerializedEquippedItems.Add(Serialized);
		}
	}
	
	// Save current level package path (same logic as SaveGame)
	FString LevelPackagePath;
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				LevelPackagePath = LevelPackage->GetName();
			}
		}
	}
	
	if (LevelPackagePath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			LevelPackagePath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			LevelPackagePath = TEXT("/Game/Levels/MainMap");
		}
	}
	
	// Normalize the level path by removing PIE prefixes (same as SaveGame)
	FString NormalizedPath = LevelPackagePath;
	if (NormalizedPath.Contains(TEXT("UEDPIE_")))
	{
		int32 LastSlash = NormalizedPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (LastSlash != INDEX_NONE)
		{
			FString PathPrefix = NormalizedPath.Left(LastSlash + 1);
			FString LevelName = NormalizedPath.RightChop(LastSlash + 1);
			
			if (LevelName.StartsWith(TEXT("UEDPIE_")))
			{
				int32 FirstUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
				if (FirstUnderscore != INDEX_NONE)
				{
					int32 SecondUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstUnderscore + 1);
					if (SecondUnderscore != INDEX_NONE)
					{
						LevelName = LevelName.RightChop(SecondUnderscore + 1);
						NormalizedPath = PathPrefix + LevelName;
					}
				}
			}
		}
	}
	
	SaveGameInstance->LevelPackagePath = NormalizedPath;
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Creating new game save - LevelPath: %s"), *NormalizedPath);

	// Establish baseline from current world (all actors are part of the baseline for a new game)
	// Collect all current actors to establish baseline
	TArray<FGuid> CurrentActorIds;
	SaveGameInstance->ActorStates.Empty();
	
	// Iterate through all actors and save their states (same logic as SaveGame)
	for (TActorIterator<AActor> ActorIterator(World); ActorIterator; ++ActorIterator)
	{
		AActor* Actor = *ActorIterator;
		if (!Actor)
		{
			continue;
		}

		// Check if actor has SaveableActorComponent
		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (!SaveableComp)
		{
			continue;
		}

		FGuid ActorId = SaveableComp->GetPersistentId();
		if (!ActorId.IsValid())
		{
			continue;
		}

		// Add to baseline (all actors are part of the baseline for a new game)
		CurrentActorIds.Add(ActorId);

		// Create actor state data
		FActorStateSaveData ActorState;
		ActorState.ActorId = ActorId;
		ActorState.bExists = true;
		
		// Save actor identification metadata (for fallback matching)
		ActorState.ActorName = Actor->GetName();
		ActorState.ActorClassPath = Actor->GetClass()->GetPathName();
		
		// Get OriginalTransform - if not set yet, use current transform
		FTransform OriginalTransformToSave = SaveableComp->OriginalTransform;
		if (OriginalTransformToSave.GetLocation().IsNearlyZero())
		{
			OriginalTransformToSave = Actor->GetActorTransform();
			SaveableComp->OriginalTransform = OriginalTransformToSave;
		}
		ActorState.OriginalSpawnTransform = OriginalTransformToSave;
		
		// For a new game, all actors are part of the baseline (not new objects)
		ActorState.bIsNewObject = false;
		
		// Save transform if enabled
		if (SaveableComp->bSaveTransform)
		{
			ActorState.Location = Actor->GetActorLocation();
			ActorState.Rotation = Actor->GetActorRotation();
			ActorState.Scale = Actor->GetActorScale3D();
		}

		// Check for physics component
		UPrimitiveComponent* PrimitiveComp = Actor->FindComponentByClass<UPrimitiveComponent>();
		if (PrimitiveComp && PrimitiveComp->IsSimulatingPhysics())
		{
			if (SaveSystemHelpers::ShouldSavePhysicsObject(Actor, SaveableComp->OriginalTransform))
			{
				ActorState.bHasPhysics = true;
				ActorState.bSimulatePhysics = true;
				
				if (SaveableComp->bSavePhysicsState)
				{
					ActorState.LinearVelocity = PrimitiveComp->GetPhysicsLinearVelocity();
					ActorState.AngularVelocity = PrimitiveComp->GetPhysicsAngularVelocityInRadians();
				}
			}
		}

		// Check for storage component
		UStorageComponent* StorageComp = Actor->FindComponentByClass<UStorageComponent>();
		if (StorageComp)
		{
			ActorState.bHasStorage = true;
			TArray<FItemEntry> StorageEntries = StorageComp->GetEntries();
			ActorState.SerializedStorageEntries = StorageSerialization::SerializeStorageEntries(StorageEntries);
			ActorState.StorageMaxVolume = StorageComp->MaxVolume;
		}

		SaveGameInstance->ActorStates.Add(ActorState);
	}
	
	// Establish baseline (all current actors are part of the baseline for a new game)
	SaveGameInstance->BaselineActorIds = CurrentActorIds;
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Established new baseline with %d actors"), SaveGameInstance->BaselineActorIds.Num());
	
	// Set timestamp
	FDateTime Now = FDateTime::Now();
	SaveGameInstance->Timestamp = Now.ToString(TEXT("%Y.%m.%d %H:%M:%S"));

	// Save to slot (SlotName was already computed above)
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, 0);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully created new game save to slot: %s (Level: %s)"), *SlotName, *NormalizedPath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create new game save to slot: %s"), *SlotName);
	}

	return bSuccess;
}

void USaveSystemSubsystem::FadeInAfterLoad()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Fade in after loading completes (for same-level loads)
	APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
	AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
	if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] FadeInAfterLoad called - starting fade-in"));
		// Fade in smoothly (2.0 seconds) to reveal the loaded game
		// Longer duration ensures player position is fully restored before fade completes
		FirstPersonPC->LoadingFadeWidget->FadeIn(3.0f);
	}
	else
	{
		ULoadingFadeWidget* WidgetPtr = FirstPersonPC ? FirstPersonPC->LoadingFadeWidget.Get() : nullptr;
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] FadeInAfterLoad called but widget not available (PC=%p, Widget=%p)"), 
			FirstPersonPC, WidgetPtr);
	}
}

void USaveSystemSubsystem::CheckAndCreatePendingNewGameSave()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check for any pending new game saves (stored with prefix "_NEWGAME_")
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	
	if (!FileManager.DirectoryExists(*SaveDir))
	{
		return;
	}

	// Find all pending new game saves
	TArray<FString> SaveFiles;
	FString SearchPattern = SaveDir / TEXT("*.sav");
	FileManager.FindFiles(SaveFiles, *SearchPattern, true, false);
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		
		// Check if it's a pending new game save
		if (FileName.StartsWith(TEXT("_NEWGAME_")))
		{
			// Load the pending save info
			if (UGameSaveData* PendingSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0)))
			{
				// Extract slot ID from filename (remove "_NEWGAME_" prefix)
				FString SlotId = FileName.RightChop(9); // Length of "_NEWGAME_"
				
				// Wait for player to spawn, then create the actual save
				// Use a timer to ensure player is fully loaded
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, [this, World, SlotId, PendingSave]()
				{
					// Get player character
					APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
					AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
					
					if (PlayerCharacter)
					{
						// Create the actual save with current player position
						CreateNewGameSave(SlotId, PendingSave->SaveName);
						
						// Clean up pending save
						FString PendingSlotName = FString::Printf(TEXT("_NEWGAME_%s"), *SlotId);
						UGameplayStatics::DeleteGameInSlot(PendingSlotName, 0);
						
					}
				}, 0.5f, false); // 0.5 second delay to ensure player is spawned
				
				break; // Only handle one pending save at a time
			}
		}
	}
}

bool USaveSystemSubsystem::SaveDimensionInstance(FGuid InstanceId)
{
	if (!InstanceId.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save dimension: Invalid instance ID"));
		return false;
	}
	
	// If CurrentSaveData doesn't exist, create a temporary one in memory
	// This allows dimension saves to work even if no save slot has been loaded/created yet
	if (!CurrentSaveData)
	{
		UWorld* World = GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save dimension: World is null"));
			return false;
		}
		
		// Create a temporary save data in memory (won't be saved to disk until SaveGame() is called)
		CurrentSaveData = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
		if (!CurrentSaveData)
		{
			UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Cannot save dimension: Failed to create save game object"));
			return false;
		}
		
		// Set level path
		FString LevelPackagePath = World->GetMapName();
		CurrentSaveData->LevelPackagePath = LevelPackagePath;
		
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Created temporary save data for dimension save (no save slot loaded yet)"));
	}
       
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save dimension: World is null"));
		return false;
	}
       
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save dimension: GameInstance is null"));
		return false;
	}
       
	UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
	if (!DimensionManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save dimension: DimensionManager not found"));
		return false;
	}
       
	UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Saving dimension instance %s"), *InstanceId.ToString());
	
	// Save dimension instance to CurrentSaveData (don't save to disk yet - SaveGame() will handle it)
	// Pass empty SlotName since we're not saving to disk here
	bool bSuccess = SaveSystemDimensionHelpers::SaveDimensionInstance(World, CurrentSaveData, DimensionManager, InstanceId, FString());
	
	if (!bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to save dimension instance %s"), *InstanceId.ToString());
	}
	
	return bSuccess;
}

bool USaveSystemSubsystem::LoadDimensionInstance(FGuid InstanceId)
{
	if (!InstanceId.IsValid() || !CurrentSaveData)
	{
		return false;
	}
       
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
       
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return false;
	}
       
	UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
	if (!DimensionManager)
	{
		return false;
	}
       
	return SaveSystemDimensionHelpers::LoadDimensionInstance(World, CurrentSaveData, DimensionManager, InstanceId);
}

TArray<AActor*> USaveSystemSubsystem::GetActorsForDimension(UWorld* World, FGuid InstanceId) const
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

void USaveSystemSubsystem::RestoreCartridgeInstanceIds(UWorld* World, UGameSaveData* SaveData, AFirstPersonCharacter* PlayerCharacter)
{
	if (!World || !SaveData || !PlayerCharacter)
	{
		return;
	}

	UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory();
	if (!InventoryComp)
	{
		return;
	}

	// Loop through all saved dimension instances and restore their instance IDs to cartridges
	for (const FDimensionInstanceSaveData& DimSaveData : SaveData->DimensionInstances)
	{
		if (!DimSaveData.InstanceId.IsValid() || !DimSaveData.CartridgeId.IsValid())
		{
			continue;
		}

		// Find cartridges in inventory with matching CartridgeId
		const TArray<FItemEntry>& Entries = InventoryComp->GetEntries();
		for (int32 i = 0; i < Entries.Num(); ++i)
		{
			FItemEntry Entry = Entries[i];
			UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItemEntry(Entry);
			if (CartridgeData && CartridgeData->GetCartridgeId() == DimSaveData.CartridgeId)
			{
				// Update the cartridge's instance ID from saved dimension data
				if (!CartridgeData->InstanceData)
				{
					CartridgeData->InstanceData = NewObject<UDimensionInstanceData>(CartridgeData);
				}
				
				CartridgeData->InstanceData->InstanceId = DimSaveData.InstanceId;
				CartridgeData->InstanceData->CartridgeId = DimSaveData.CartridgeId;
				CartridgeData->InstanceData->WorldPosition = DimSaveData.WorldPosition;
				CartridgeData->InstanceData->Stability = DimSaveData.Stability;
				CartridgeData->InstanceData->bIsLoaded = false;

				// Update the ItemEntry with the new cartridge data
				UDimensionCartridgeHelpers::SetCartridgeDataInItemEntry(Entry, CartridgeData);

				// Remove the old entry and add the updated one
				FGuid ItemId = Entry.ItemId;
				if (InventoryComp->RemoveById(ItemId))
				{
					InventoryComp->TryAdd(Entry);
				}

				// Also update the ItemPickup actor if it exists in the world
				for (TActorIterator<AItemPickup> ActorItr(World); ActorItr; ++ActorItr)
				{
					AItemPickup* ItemPickup = *ActorItr;
					if (ItemPickup && ItemPickup->GetItemEntry().ItemId == ItemId)
					{
						ItemPickup->SetItemEntry(Entry);
						UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored instance ID %s to cartridge %s"), 
							*DimSaveData.InstanceId.ToString(), *DimSaveData.CartridgeId.ToString());
						break;
					}
				}

				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored instance ID %s to cartridge %s in inventory"), 
					*DimSaveData.InstanceId.ToString(), *DimSaveData.CartridgeId.ToString());
				break; // Found the cartridge, move to next dimension instance
			}
		}
	}
}

void USaveSystemSubsystem::RestoreLoadedDimension(UWorld* World, UGameSaveData* SaveData, FVector PlayerLocation, FRotator PlayerRotation, bool bRestorePlayerPosition, TFunction<void()> OnComplete)
{
	if (!World || !SaveData || !SaveData->LoadedDimensionInstanceId.IsValid())
	{
		return;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
	if (!DimensionManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot restore dimension: DimensionManager not found"));
		return;
	}

	// Find the saved dimension instance data
	const FDimensionInstanceSaveData* DimSaveData = nullptr;
	for (const FDimensionInstanceSaveData& DimData : SaveData->DimensionInstances)
	{
		if (DimData.InstanceId == SaveData->LoadedDimensionInstanceId)
		{
			DimSaveData = &DimData;
			break;
		}
	}

	if (!DimSaveData)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot restore dimension: Saved dimension instance %s not found in save data"), 
			*SaveData->LoadedDimensionInstanceId.ToString());
		return;
	}

	// Find the cartridge with matching CartridgeId
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot restore dimension: Player character not found"));
		return;
	}

	UInventoryComponent* InventoryComp = PlayerCharacter->GetInventory();
	if (!InventoryComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot restore dimension: Inventory component not found"));
		return;
	}

	// Find cartridge in inventory OR in portal device sockets
	// First check portal device sockets (cartridge might be socketed)
	bool bFoundCartridge = false;
	APortalDevice* FoundPortalDevice = nullptr;
	FGuid FoundCartridgeId;
	
	// Check portal devices first
	for (TActorIterator<APortalDevice> ActorItr(World); ActorItr; ++ActorItr)
	{
		APortalDevice* PortalDevice = *ActorItr;
		if (!PortalDevice || !PortalDevice->CartridgeSocket)
		{
			continue;
		}

		AItemPickup* SocketedItem = PortalDevice->CartridgeSocket->GetSocketedItem();
		if (SocketedItem)
		{
			UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(SocketedItem);
			if (CartridgeData && CartridgeData->GetCartridgeId() == DimSaveData->CartridgeId)
			{
				bFoundCartridge = true;
				FoundPortalDevice = PortalDevice;
				FoundCartridgeId = CartridgeData->GetCartridgeId();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Found cartridge in portal device socket"));
				break;
			}
		}
	}

	// If not found in socket, check inventory
	if (!bFoundCartridge)
	{
		const TArray<FItemEntry>& Entries = InventoryComp->GetEntries();
		for (const FItemEntry& Entry : Entries)
		{
			UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItemEntry(Entry);
			if (CartridgeData && CartridgeData->GetCartridgeId() == DimSaveData->CartridgeId)
			{
				bFoundCartridge = true;
				FoundCartridgeId = CartridgeData->GetCartridgeId();
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Found cartridge in inventory"));
				break;
			}
		}
	}

	if (!bFoundCartridge)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot restore dimension: Cartridge with ID %s not found in inventory or sockets"), 
			*DimSaveData->CartridgeId.ToString());
		return;
	}

	// If cartridge is in a socket, load the dimension
	if (FoundPortalDevice)
	{
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Cartridge found in portal device socket - ensuring dimension is loaded"));
		
		// Check if dimension is already loaded
		if (DimensionManager->IsDimensionLoaded())
		{
			FGuid CurrentInstanceId = DimensionManager->GetCurrentInstanceId();
			if (CurrentInstanceId == SaveData->LoadedDimensionInstanceId)
			{
				UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Dimension instance %s is already loaded"), 
					*CurrentInstanceId.ToString());
				
				// Dimension already loaded - ensure screen stays black before restoring player position
				APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
				AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
				if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
				{
					// Force opacity to 1.0 (fully black) to prevent flash during teleport
					FirstPersonPC->LoadingFadeWidget->SetOpacity(1.0f);
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Ensured fade widget is black before restoring player position (already-loaded)"));
				}
				
				// Dimension already loaded - restore player position immediately
				if (bRestorePlayerPosition && !PlayerLocation.IsNearlyZero())
				{
					PlayerCharacter->SetActorLocation(PlayerLocation);
					PlayerCharacter->SetActorRotation(PlayerRotation);
					if (APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
					{
						PlayerController->SetControlRotation(PlayerRotation);
					}
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored player position in already-loaded dimension: %s"), *PlayerLocation.ToString());
					
					// Ensure fade widget stays black during delay
					if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
					{
						FirstPersonPC->LoadingFadeWidget->SetOpacity(1.0f);
					}
					
					// Wait a bit before fading in to ensure position is settled
					// Use shared pointer to keep timer handle alive
					TSharedPtr<FTimerHandle> FadeInDelayTimer = MakeShared<FTimerHandle>();
					World->GetTimerManager().SetTimer(*FadeInDelayTimer, [OnComplete, FadeInDelayTimer, World]()
					{
						UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Fade-in delay timer fired (already-loaded) - calling fade-in callback"));
						// Call completion callback (fade in) after delay
						if (OnComplete)
						{
							OnComplete();
						}
						// Clear the timer handle
						if (World && FadeInDelayTimer.IsValid())
						{
							World->GetTimerManager().ClearTimer(*FadeInDelayTimer);
						}
					}, 0.9f, false); // Wait 0.9 seconds before fade-in (0.7s base + 0.2s extra to ensure player position is fully settled)
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Set fade-in delay timer for 0.9 seconds (already-loaded dimension)"));
				}
				else if (OnComplete)
				{
					// No player position to restore, but we still need to fade in
					OnComplete();
				}
				return;
			}
		}
		
		// Trigger dimension loading
		FoundPortalDevice->OpenPortal(FoundCartridgeId);
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Triggered dimension loading for instance %s"), 
			*SaveData->LoadedDimensionInstanceId.ToString());
		
		// If player was in dimension when saving, wait for dimension to load before restoring position
		// Poll for dimension to be loaded since we can't easily bind to the delegate from here
		if (bRestorePlayerPosition && !PlayerLocation.IsNearlyZero())
		{
			FGuid TargetInstanceId = SaveData->LoadedDimensionInstanceId;
			
			// Use a shared pointer to track if we've already restored position and store the timer handle
			TSharedPtr<bool> bPositionRestored = MakeShareable(new bool(false));
			TSharedPtr<FTimerHandle> PollTimerHandle = MakeShareable(new FTimerHandle());
			
			// Poll every 0.1 seconds until dimension is loaded
			auto PollForDimensionLoaded = [World, PlayerLocation, PlayerRotation, OnComplete, TargetInstanceId, DimensionManager, bPositionRestored, PollTimerHandle]()
			{
				if (*bPositionRestored)
				{
					// Already restored, stop polling
					World->GetTimerManager().ClearTimer(*PollTimerHandle);
					return;
				}
				
				if (!DimensionManager || !DimensionManager->IsDimensionLoaded())
				{
					return; // Keep polling
				}
				
				FGuid CurrentInstanceId = DimensionManager->GetCurrentInstanceId();
				if (CurrentInstanceId != TargetInstanceId)
				{
					return; // Wrong dimension, keep polling
				}
				
				// Dimension is loaded! Ensure screen stays black before restoring player position
				APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
				AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
				if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
				{
					// Force opacity to 1.0 (fully black) to prevent flash during teleport
					FirstPersonPC->LoadingFadeWidget->SetOpacity(1.0f);
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Ensured fade widget is black before restoring player position"));
				}
				
				// Restore player position
				*bPositionRestored = true;
				World->GetTimerManager().ClearTimer(*PollTimerHandle); // Stop polling
				
				APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
				if (AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn))
				{
					PlayerCharacter->SetActorLocation(PlayerLocation);
					PlayerCharacter->SetActorRotation(PlayerRotation);
					if (APlayerController* PlayerController = Cast<APlayerController>(PlayerCharacter->GetController()))
					{
						PlayerController->SetControlRotation(PlayerRotation);
					}
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Restored player position in dimension: %s"), *PlayerLocation.ToString());
					
					// Ensure fade widget stays black during delay
					if (FirstPersonPC && FirstPersonPC->LoadingFadeWidget)
					{
						FirstPersonPC->LoadingFadeWidget->SetOpacity(1.0f);
					}
					
					// Wait a bit before fading in to ensure everything is settled
					// Use shared pointer to keep timer handle alive
					TSharedPtr<FTimerHandle> FadeInDelayTimer = MakeShared<FTimerHandle>();
					World->GetTimerManager().SetTimer(*FadeInDelayTimer, [OnComplete, FadeInDelayTimer, World]()
					{
						UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Fade-in delay timer fired - calling fade-in callback"));
						// Call completion callback (fade in) after additional delay
						if (OnComplete)
						{
							OnComplete();
						}
						// Clear the timer handle
						if (World && FadeInDelayTimer.IsValid())
						{
							World->GetTimerManager().ClearTimer(*FadeInDelayTimer);
						}
					}, 1.4f, false); // Additional 1.4 second delay before fade-in starts (1.2s base + 0.2s extra to ensure player position is fully settled)
					UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Set fade-in delay timer for 1.4 seconds"));
				}
			};
			
			// Start polling
			World->GetTimerManager().SetTimer(*PollTimerHandle, PollForDimensionLoaded, 0.1f, true); // Poll every 0.1 seconds
			
			// Also set a one-time timer to stop polling after a reasonable timeout (5 seconds)
			FTimerHandle TimeoutTimer;
			World->GetTimerManager().SetTimer(TimeoutTimer, [World, PollTimerHandle, OnComplete, bPositionRestored]()
			{
				if (!*bPositionRestored)
				{
					World->GetTimerManager().ClearTimer(*PollTimerHandle);
					UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Timeout waiting for dimension to load - proceeding with fade-in"));
					if (OnComplete)
					{
						OnComplete();
					}
				}
			}, 5.0f, false);
			
			UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Waiting for dimension instance %s to load before restoring player position"), 
				*TargetInstanceId.ToString());
		}
		else if (OnComplete)
		{
			// No player position to restore, fade in immediately (dimension will load in background)
			OnComplete();
		}
	}
	else
	{
		// Cartridge is in inventory but not in socket - don't auto-load
		UE_LOG(LogTemp, Log, TEXT("[SaveSystem] Cartridge found in inventory but not in socket - dimension will not be auto-loaded"));
	}
}

