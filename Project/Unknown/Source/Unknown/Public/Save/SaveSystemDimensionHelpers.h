#pragma once

#include "CoreMinimal.h"
#include "Save/GameSaveData.h"

class UWorld;
class ULevel;
class UGameSaveData;
class UDimensionManagerSubsystem;

// Forward declaration for dimension instance save data
struct FDimensionInstanceSaveData;

/**
 * Helper functions for dimension instance save/load operations.
 * Extracted from SaveSystemSubsystem to keep it manageable.
 */
namespace SaveSystemDimensionHelpers
{
	// Save dimension instance actor states
	UNKNOWN_API bool SaveDimensionInstance(
		UWorld* World,
		UGameSaveData* SaveData,
		UDimensionManagerSubsystem* DimensionManager,
		FGuid InstanceId,
		const FString& SlotName
	);

	// Load dimension instance actor states
	UNKNOWN_API bool LoadDimensionInstance(
		UWorld* World,
		UGameSaveData* SaveData,
		UDimensionManagerSubsystem* DimensionManager,
		FGuid InstanceId
	);

	// Get all actors belonging to a dimension
	UNKNOWN_API TArray<AActor*> GetActorsForDimension(UWorld* World, FGuid InstanceId);
}
