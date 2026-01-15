#include "Dimensions/DimensionManagerSubsystem.h"
#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "Components/SaveableActorComponent.h"
#include "Engine/World.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void UDimensionManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	CurrentInstance.Reset();
	PlayerDimensionInstanceId = FGuid();
}

bool UDimensionManagerSubsystem::LoadDimensionInstance(FGuid CartridgeId, UDimensionDefinition* DimensionDef, const FVector& SpawnPosition, FGuid InstanceId)
{
	if (!DimensionDef)
	{
		UE_LOG(LogTemp, Error, TEXT("[DimensionManager] Cannot load dimension: DimensionDef is null"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[DimensionManager] Cannot load dimension: World is null"));
		return false;
	}

	// Unload previous dimension if one is loaded
	if (CurrentInstance.IsSet() && CurrentInstance->bIsLoaded)
	{
		UnloadDimensionInstance();
	}

	// Get the level path
	FString LevelPath = DimensionDef->DimensionLevel.ToSoftObjectPath().ToString();
	if (LevelPath.IsEmpty())
	{
		UE_LOG(LogTemp, Error, TEXT("[DimensionManager] Cannot load dimension: Level path is empty"));
		return false;
	}

	// Create instance info
	FDimensionInstanceInfo InstanceInfo;
	// Use provided InstanceId if valid, otherwise generate a new one
	if (InstanceId.IsValid())
	{
		InstanceInfo.InstanceId = InstanceId;
		UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Restoring dimension instance %s from cartridge"), *InstanceId.ToString());
	}
	else
	{
		InstanceInfo.InstanceId = FGuid::NewGuid();
		UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Creating new dimension instance %s"), *InstanceInfo.InstanceId.ToString());
	}
	InstanceInfo.CartridgeId = CartridgeId;
	InstanceInfo.WorldPosition = SpawnPosition;
	InstanceInfo.DimensionLevel = DimensionDef->DimensionLevel;
	InstanceInfo.bIsLoaded = false;

	// Get the level package name from the soft object path
	FSoftObjectPath SoftPath = DimensionDef->DimensionLevel.ToSoftObjectPath();
	FString LevelPackageName = SoftPath.GetLongPackageName();
	
	// Create a unique level name for this instance
	FString UniqueLevelName = FString::Printf(TEXT("Dimension_%s"), *InstanceInfo.InstanceId.ToString(EGuidFormats::Short));

	// Load the level at the specified position using ULevelStreamingDynamic
	bool bLoadSuccess = false;
	ULevelStreamingDynamic* StreamingLevel = ULevelStreamingDynamic::LoadLevelInstance(
		World,
		LevelPackageName,
		SpawnPosition,
		FRotator::ZeroRotator,
		bLoadSuccess,
		UniqueLevelName
	);

	if (!StreamingLevel || !bLoadSuccess)
	{
		UE_LOG(LogTemp, Error, TEXT("[DimensionManager] Failed to load dimension level: %s"), *LevelPackageName);
		return false;
	}

	InstanceInfo.StreamingLevel = StreamingLevel;
	CurrentInstance = InstanceInfo;

	// Set up callback for when level finishes loading
	StreamingLevel->OnLevelLoaded.AddDynamic(this, &UDimensionManagerSubsystem::OnLevelLoaded);

	UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Loading dimension instance %s at position %s"), 
		*InstanceInfo.InstanceId.ToString(), *SpawnPosition.ToString());

	return true;
}

void UDimensionManagerSubsystem::OnLevelLoaded()
{
	if (!CurrentInstance.IsSet())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ULevelStreaming* StreamingLevel = CurrentInstance->StreamingLevel.Get();
	if (!StreamingLevel || !StreamingLevel->GetLoadedLevel())
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] Level loaded but streaming level or loaded level is null"));
		return;
	}

	ULevel* LoadedLevel = StreamingLevel->GetLoadedLevel();
	CurrentInstance->bIsLoaded = true;

	// Wait for actors to be fully positioned before restoring state
	// Streaming levels may position actors after OnLevelLoaded is called
	FTimerHandle WaitForActorsTimer;
	World->GetTimerManager().SetTimer(WaitForActorsTimer, [this, World, LoadedLevel]()
	{
		if (!CurrentInstance.IsSet())
		{
			return;
		}

		// Check if actors are positioned (not all at 0,0,0)
		bool bActorsPositioned = false;
		int32 ActorsWithSaveableComponent = 0;
		int32 ActorsAtOrigin = 0;
		
		for (AActor* Actor : LoadedLevel->Actors)
		{
			if (!Actor)
			{
				continue;
			}

			USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
			if (SaveableComp)
			{
				ActorsWithSaveableComponent++;
				FVector ActorLocation = Actor->GetActorLocation();
				if (ActorLocation.IsNearlyZero())
				{
					ActorsAtOrigin++;
				}
				else
				{
					bActorsPositioned = true;
					UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Actor %s is positioned at %s"), 
						*Actor->GetName(), *ActorLocation.ToString());
				}
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Checking actor positions: %d actors with SaveableComponent, %d at origin, bActorsPositioned=%d"), 
			ActorsWithSaveableComponent, ActorsAtOrigin, bActorsPositioned ? 1 : 0);

		// If all actors are still at origin, wait another frame
		if (!bActorsPositioned && ActorsWithSaveableComponent > 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] Actors not positioned yet, waiting another frame..."));
			FTimerHandle RetryTimer;
			World->GetTimerManager().SetTimer(RetryTimer, [this, World, LoadedLevel]()
			{
				OnLevelLoadedDelayed(World, LoadedLevel);
			}, 0.1f, false);
			return;
		}

		// Actors are positioned, proceed with tagging and loading
		OnLevelLoadedDelayed(World, LoadedLevel);
	}, 0.1f, false); // Wait 0.1 seconds for actors to be positioned
}

void UDimensionManagerSubsystem::OnLevelLoadedDelayed(UWorld* World, ULevel* LoadedLevel)
{
	if (!CurrentInstance.IsSet())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Proceeding with dimension setup for instance %s"), 
		*CurrentInstance->InstanceId.ToString());

	// Tag all actors in the dimension level with dimension ID
	TagActorsInDimension(LoadedLevel, CurrentInstance->InstanceId);

	UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Dimension instance %s loaded successfully"), 
		*CurrentInstance->InstanceId.ToString());

	// Load saved state if it exists (this will restore GUIDs and spawn missing actors)
	USaveSystemSubsystem* SaveSystem = GetSaveSystemSubsystem();
	if (SaveSystem)
	{
		SaveSystem->LoadDimensionInstance(CurrentInstance->InstanceId);
	}

	// Broadcast delegate
	OnDimensionLoaded.Broadcast(CurrentInstance->InstanceId);
}

void UDimensionManagerSubsystem::UnloadDimensionInstance()
{
	if (!CurrentInstance.IsSet() || !CurrentInstance->bIsLoaded)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Get level reference BEFORE we start unloading (otherwise GetDimensionLevel will return null)
	ULevel* DimensionLevel = GetDimensionLevel(CurrentInstance->InstanceId);

	// Save dimension state before unloading
	USaveSystemSubsystem* SaveSystem = GetSaveSystemSubsystem();
	if (SaveSystem && CurrentInstance->bIsLoaded && DimensionLevel)
	{
		UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Saving dimension instance %s before unloading"), 
			*CurrentInstance->InstanceId.ToString());
		bool bSaved = SaveSystem->SaveDimensionInstance(CurrentInstance->InstanceId);
		if (!bSaved)
		{
			UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] Failed to save dimension instance %s before unloading"), 
				*CurrentInstance->InstanceId.ToString());
		}
	}
	else if (!SaveSystem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] Cannot save dimension: SaveSystem not found"));
	}
	else if (!DimensionLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] Cannot save dimension: Dimension level not found (may already be unloading)"));
	}

	// Unload the level
	ULevelStreaming* StreamingLevel = CurrentInstance->StreamingLevel.Get();
	if (StreamingLevel)
	{
		StreamingLevel->SetShouldBeLoaded(false);
		StreamingLevel->SetShouldBeVisible(false);
		StreamingLevel->SetIsRequestingUnloadAndRemoval(true);
	}

	FGuid InstanceId = CurrentInstance->InstanceId;
	CurrentInstance.Reset();

	UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Unloaded dimension instance %s"), *InstanceId.ToString());

	// Clear LoadedDimensionInstanceId in save data when dimension is unloaded
	// This ensures normal cartridge insertion works correctly after unloading
	// (SaveSystem was already retrieved above, reuse it)
	if (SaveSystem && SaveSystem->GetCurrentSaveData())
	{
		UGameSaveData* SaveData = SaveSystem->GetCurrentSaveData();
		if (SaveData && SaveData->LoadedDimensionInstanceId == InstanceId)
		{
			SaveData->LoadedDimensionInstanceId = FGuid();
			UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Cleared LoadedDimensionInstanceId in save data after unloading dimension %s"), *InstanceId.ToString());
		}
	}

	// Broadcast delegate
	OnDimensionUnloaded.Broadcast(InstanceId);
}

ULevelStreaming* UDimensionManagerSubsystem::GetStreamingLevel() const
{
	if (CurrentInstance.IsSet() && CurrentInstance->bIsLoaded)
	{
		return CurrentInstance->StreamingLevel.Get();
	}
	return nullptr;
}

bool UDimensionManagerSubsystem::IsDimensionLoaded() const
{
	return CurrentInstance.IsSet() && CurrentInstance->bIsLoaded;
}

FGuid UDimensionManagerSubsystem::GetCurrentInstanceId() const
{
	if (CurrentInstance.IsSet())
	{
		return CurrentInstance->InstanceId;
	}
	return FGuid();
}

ULevel* UDimensionManagerSubsystem::GetDimensionLevel(FGuid InstanceId) const
{
	if (CurrentInstance.IsSet() && CurrentInstance->InstanceId == InstanceId)
	{
		ULevelStreaming* StreamingLevel = CurrentInstance->StreamingLevel.Get();
		if (StreamingLevel)
		{
			return StreamingLevel->GetLoadedLevel();
		}
	}
	return nullptr;
}

void UDimensionManagerSubsystem::TagActorsInDimension(ULevel* DimensionLevel, FGuid InstanceId)
{
	if (!DimensionLevel)
	{
		return;
	}

	UWorld* World = DimensionLevel->GetWorld();
	if (!World)
	{
		return;
	}

	// Check if there's save data for this dimension instance
	// If there is, we should NOT assign new GUIDs - let the save system restore them
	bool bHasSaveData = false;
	USaveSystemSubsystem* SaveSystem = GetSaveSystemSubsystem();
	if (SaveSystem && SaveSystem->CurrentSaveData)
	{
		for (const FDimensionInstanceSaveData& DimData : SaveSystem->CurrentSaveData->DimensionInstances)
		{
			if (DimData.InstanceId == InstanceId)
			{
				bHasSaveData = true;
				break;
			}
		}
	}

	int32 TaggedCount = 0;
	int32 GUIDAssignedCount = 0;
	for (AActor* Actor : DimensionLevel->Actors)
	{
		if (!Actor)
		{
			continue;
		}

		USaveableActorComponent* SaveableComp = Actor->FindComponentByClass<USaveableActorComponent>();
		if (SaveableComp)
		{
			// Only assign a new GUID if:
			// 1. The actor doesn't have one, AND
			// 2. There's no save data for this dimension (first time loading)
			// If there's save data, the save system will restore the GUIDs
			if (!SaveableComp->GetPersistentId().IsValid() && !bHasSaveData)
			{
				SaveableComp->SetPersistentId(FGuid::NewGuid());
				GUIDAssignedCount++;
			}
			
			// Set OriginalTransform if not set (for default level actors)
			// Convert to level-local space for consistency with saved transforms
			if (SaveableComp->OriginalTransform.GetLocation().IsNearlyZero() && 
				SaveableComp->OriginalTransform.GetRotation().IsIdentity() && 
				SaveableComp->OriginalTransform.GetScale3D().IsNearlyZero())
			{
				// Get dimension world position to convert to level-local space
				FTransform WorldTransform = Actor->GetActorTransform();
				FVector WorldLocation = WorldTransform.GetLocation();
				
				UE_LOG(LogTemp, Log, TEXT("[DimensionManager] TagActorsInDimension: Setting OriginalTransform for %s - world pos: %s"), 
					*Actor->GetName(), *WorldLocation.ToString());
				
				if (CurrentInstance.IsSet())
				{
					FVector DimensionWorldPos = CurrentInstance->WorldPosition;
					FTransform LocalTransform = WorldTransform;
					FVector LocalLocation = WorldLocation - DimensionWorldPos;
					LocalTransform.SetLocation(LocalLocation);
					SaveableComp->OriginalTransform = LocalTransform;
					UE_LOG(LogTemp, Log, TEXT("[DimensionManager] TagActorsInDimension: Converted to local space - DimensionWorldPos: %s, local pos: %s"), 
						*DimensionWorldPos.ToString(), *LocalLocation.ToString());
				}
				else
				{
					// Fallback to world transform if we don't have instance info yet
					SaveableComp->OriginalTransform = WorldTransform;
					UE_LOG(LogTemp, Warning, TEXT("[DimensionManager] TagActorsInDimension: CurrentInstance not set, using world transform for %s"), 
						*Actor->GetName());
				}
			}
			else
			{
				UE_LOG(LogTemp, Log, TEXT("[DimensionManager] TagActorsInDimension: %s already has OriginalTransform: %s"), 
					*Actor->GetName(), *SaveableComp->OriginalTransform.GetLocation().ToString());
			}
			
			SaveableComp->SetDimensionInstanceId(InstanceId);
			TaggedCount++;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[DimensionManager] Tagged %d actors in dimension instance %s (%d new GUIDs assigned, hasSaveData=%d)"), 
		TaggedCount, *InstanceId.ToString(), GUIDAssignedCount, bHasSaveData ? 1 : 0);
}

FDimensionInstanceInfo UDimensionManagerSubsystem::GetCurrentInstanceInfo() const
{
	if (CurrentInstance.IsSet())
	{
		return CurrentInstance.GetValue();
	}
	return FDimensionInstanceInfo();
}

USaveSystemSubsystem* UDimensionManagerSubsystem::GetSaveSystemSubsystem() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<USaveSystemSubsystem>();
}
