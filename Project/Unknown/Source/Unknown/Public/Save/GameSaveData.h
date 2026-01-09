// Game save data class for storing game state
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSaveData.generated.h"

// Player data section
USTRUCT()
struct FPlayerSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FVector Location;
	
	UPROPERTY(SaveGame)
	FRotator Rotation;
	
	UPROPERTY(SaveGame)
	float CurrentHunger = 100.0f;
	
	UPROPERTY(SaveGame)
	float MaxHunger = 100.0f;
};

// Hotbar save data
USTRUCT()
struct FHotbarSaveData
{
	GENERATED_BODY()

	// ItemDefinition paths for each slot (index 0-8)
	UPROPERTY(SaveGame)
	TArray<FString> AssignedItemPaths;
	
	UPROPERTY(SaveGame)
	int32 ActiveIndex = INDEX_NONE;
};

// Inventory save data (reuse existing StorageSerialization pattern)
USTRUCT()
struct FInventorySaveData
{
	GENERATED_BODY()

	// Serialized entries using StorageSerialization format
	UPROPERTY(SaveGame)
	FString SerializedEntries;
};

// Equipment save data
USTRUCT()
struct FEquipmentSaveData
{
	GENERATED_BODY()

	// Map of slot enum value (as uint8) to serialized item entry
	// Format: "SlotIndex|ItemDefPath|ItemId|CustomData..."
	UPROPERTY(SaveGame)
	TArray<FString> SerializedEquippedItems;
};

// Actor state save data (for any actor with SaveableActorComponent)
USTRUCT()
struct FActorStateSaveData
{
	GENERATED_BODY()

	UPROPERTY(SaveGame)
	FGuid ActorId;
	
	// Actor identification metadata (for fallback matching when GUID fails)
	UPROPERTY(SaveGame)
	FString ActorName;  // Actor's GetName() result
	
	UPROPERTY(SaveGame)
	FString ActorClassPath;  // Actor's class path (e.g., "/Game/Blueprints/BP_Crate.BP_Crate_C")
	
	UPROPERTY(SaveGame)
	FTransform OriginalSpawnTransform;  // Original transform when first saved (for matching)
	
	UPROPERTY(SaveGame)
	FVector Location;
	
	UPROPERTY(SaveGame)
	FRotator Rotation;
	
	UPROPERTY(SaveGame)
	FVector Scale;
	
	UPROPERTY(SaveGame)
	bool bExists = true; // Whether the actor currently exists in the world
	
	// New object spawning data (only for objects not in baseline)
	UPROPERTY(SaveGame)
	bool bIsNewObject = false;
	
	UPROPERTY(SaveGame)
	FString SpawnActorClassPath;  // Class to spawn (e.g., "/Game/Blueprints/BP_ItemPickup.BP_ItemPickup_C")
	
	// For ItemPickup actors
	UPROPERTY(SaveGame)
	FString ItemDefinitionPath;  // ItemDefinition asset path
	
	UPROPERTY(SaveGame)
	FString SerializedItemEntry;  // Serialized FItemEntry for ItemPickup
	
	// Physics-specific data (only valid if actor has physics)
	UPROPERTY(SaveGame)
	bool bHasPhysics = false;
	
	UPROPERTY(SaveGame)
	FVector LinearVelocity;
	
	UPROPERTY(SaveGame)
	FVector AngularVelocity;
	
	UPROPERTY(SaveGame)
	bool bSimulatePhysics = false;
	
	// Container-specific data (only valid if actor has StorageComponent)
	UPROPERTY(SaveGame)
	bool bHasStorage = false;
	
	UPROPERTY(SaveGame)
	FString SerializedStorageEntries;
	
	UPROPERTY(SaveGame)
	float StorageMaxVolume = 60.0f;
};


UCLASS()
class UNKNOWN_API UGameSaveData : public USaveGame
{
	GENERATED_BODY()

public:
	UGameSaveData();

	// Save name (user-defined)
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FString SaveName;

	// Timestamp when the save was created
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FString Timestamp;

	// Player location (legacy - kept for backward compatibility)
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FVector PlayerLocation;

	// Player rotation (legacy - kept for backward compatibility)
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FRotator PlayerRotation;

	// Level package path (e.g., "/Game/Levels/MainMap")
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FString LevelPackagePath;

	// Extended save data
	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	FPlayerSaveData PlayerData;

	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	FHotbarSaveData HotbarData;

	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	FInventorySaveData InventoryData;

	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	FEquipmentSaveData EquipmentData;

	// Unified actor state tracking - tracks all actors with SaveableActorComponent and their current state
	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	TArray<FActorStateSaveData> ActorStates;

	// Baseline of actor GUIDs that existed in the level when this save was first created
	// Used to detect which actors have been removed/destroyed
	UPROPERTY(SaveGame, VisibleAnywhere, Category="SaveData")
	TArray<FGuid> BaselineActorIds;

	// Get formatted timestamp string
	UFUNCTION(BlueprintPure, Category="SaveData")
	FString GetFormattedTimestamp() const { return Timestamp; }

	// Get save name
	UFUNCTION(BlueprintPure, Category="SaveData")
	FString GetSaveName() const { return SaveName; }
};

