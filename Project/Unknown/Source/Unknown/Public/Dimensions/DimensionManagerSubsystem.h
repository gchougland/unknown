#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dimensions/DimensionDefinition.h"
#include "Dimensions/DimensionInstanceData.h"
#include "Engine/LevelStreamingDynamic.h"
#include "DimensionManagerSubsystem.generated.h"

class ULevel;
class ULevelStreaming;
class UDimensionDefinition;
class UDimensionInstanceData;
class ADimensionSpawnMarker;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDimensionLoaded, FGuid, InstanceId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDimensionUnloaded, FGuid, InstanceId);

/**
 * Information about a loaded dimension instance
 */
USTRUCT(BlueprintType)
struct FDimensionInstanceInfo
{
	GENERATED_BODY()

	// Unique instance identifier
	UPROPERTY(BlueprintReadOnly)
	FGuid InstanceId;

	// Associated cartridge ID
	UPROPERTY(BlueprintReadOnly)
	FGuid CartridgeId;

	// Position in the open world
	UPROPERTY(BlueprintReadOnly)
	FVector WorldPosition;

	// Reference to the dimension level
	UPROPERTY(BlueprintReadOnly)
	TSoftObjectPtr<UWorld> DimensionLevel;

	// Streaming level handle
	UPROPERTY()
	TObjectPtr<ULevelStreaming> StreamingLevel;

	// Whether dimension is currently loaded
	UPROPERTY(BlueprintReadOnly)
	bool bIsLoaded = false;
};

/**
 * Subsystem for managing dimension instances and level loading.
 * Handles loading/unloading dimensions at fixed positions in the open world.
 */
UCLASS()
class UNKNOWN_API UDimensionManagerSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Load a dimension instance at the specified position
	// If InstanceId is valid, uses that instance ID (for restoring existing dimension)
	// If InstanceId is invalid, generates a new instance ID (for new dimension)
	UFUNCTION(BlueprintCallable, Category="Dimension Manager")
	bool LoadDimensionInstance(FGuid CartridgeId, UDimensionDefinition* DimensionDef, const FVector& SpawnPosition, FGuid InstanceId = FGuid());

	// Unload the current dimension instance
	UFUNCTION(BlueprintCallable, Category="Dimension Manager")
	void UnloadDimensionInstance();

	// Get the current streaming level
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	ULevelStreaming* GetStreamingLevel() const;

	// Check if a dimension is currently loaded
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	bool IsDimensionLoaded() const;

	// Get current dimension instance ID
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	FGuid GetCurrentInstanceId() const;

	// Get the level for a dimension instance
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	ULevel* GetDimensionLevel(FGuid InstanceId) const;

	// Tag all actors in a dimension level with the instance ID
	UFUNCTION(BlueprintCallable, Category="Dimension Manager")
	void TagActorsInDimension(ULevel* DimensionLevel, FGuid InstanceId);

	// Get current instance info
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	FDimensionInstanceInfo GetCurrentInstanceInfo() const;

	// Get the dimension instance ID the player is currently in (empty if in main world)
	UFUNCTION(BlueprintPure, Category="Dimension Manager")
	FGuid GetPlayerDimensionInstanceId() const { return PlayerDimensionInstanceId; }

	// Set the dimension instance ID the player is currently in
	UFUNCTION(BlueprintCallable, Category="Dimension Manager")
	void SetPlayerDimensionInstanceId(FGuid InstanceId) { PlayerDimensionInstanceId = InstanceId; }

	// Clear the player's dimension instance ID (player is back in main world)
	UFUNCTION(BlueprintCallable, Category="Dimension Manager")
	void ClearPlayerDimensionInstanceId() { PlayerDimensionInstanceId = FGuid(); }

	// Delegates
	UPROPERTY(BlueprintAssignable, Category="Dimension Manager")
	FOnDimensionLoaded OnDimensionLoaded;

	UPROPERTY(BlueprintAssignable, Category="Dimension Manager")
	FOnDimensionUnloaded OnDimensionUnloaded;

private:
	// Currently loaded dimension instance (only one at a time)
	TOptional<FDimensionInstanceInfo> CurrentInstance;

	// Dimension instance ID the player is currently in (empty if in main world)
	UPROPERTY()
	FGuid PlayerDimensionInstanceId;

	// Callback when level finishes loading
	UFUNCTION()
	void OnLevelLoaded();

	// Delayed callback after actors are positioned
	void OnLevelLoadedDelayed(UWorld* World, ULevel* LoadedLevel);

	// Helper to get the save system subsystem
	class USaveSystemSubsystem* GetSaveSystemSubsystem() const;
};
