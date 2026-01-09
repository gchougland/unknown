#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/EquipmentTypes.h"

class UItemDefinition;
class AActor;

/**
 * Helper functions for serializing game data to/from strings for save system
 */
namespace SaveSystemHelpers
{
	// Serialize a single ItemEntry to string format: "ItemDefPath|ItemId|CustomDataKey1=Value1|CustomDataKey2=Value2|..."
	UNKNOWN_API FString SerializeItemEntry(const FItemEntry& Entry);
	
	// Deserialize a single ItemEntry from string format
	UNKNOWN_API bool DeserializeItemEntry(const FString& SerializedData, FItemEntry& OutEntry);
	
	// Serialize equipment slot data: "SlotIndex|ItemDefPath|ItemId|CustomData..."
	UNKNOWN_API FString SerializeEquipmentSlot(EEquipmentSlot Slot, const FItemEntry& Entry);
	
	// Deserialize equipment slot data, returns false if invalid
	UNKNOWN_API bool DeserializeEquipmentSlot(const FString& SerializedData, EEquipmentSlot& OutSlot, FItemEntry& OutEntry);
	
	// Check if a physics object should be saved (has moved significantly from original)
	UNKNOWN_API bool ShouldSavePhysicsObject(AActor* Actor, const FTransform& OriginalTransform, float PositionThreshold = 1.0f, float RotationThreshold = 1.0f);
}

