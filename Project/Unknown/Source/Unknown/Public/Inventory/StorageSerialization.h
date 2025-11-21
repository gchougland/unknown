#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemTypes.h"

class UStorageComponent;
class UItemDefinition;

/**
 * Helper functions for serializing storage component data to/from ItemEntry CustomData
 * POSSIBLY NOT CURRENTLY WORKING
 */
namespace StorageSerialization
{
	// Serialize storage component entries to a string format stored in CustomData
	// Format: "EntryCount|Def1Path|ItemId1|Def2Path|ItemId2|..."
	UNKNOWN_API FString SerializeStorageEntries(const TArray<FItemEntry>& Entries);
	
	// Deserialize storage entries from CustomData string
	// Returns empty array if serialization fails
	UNKNOWN_API TArray<FItemEntry> DeserializeStorageEntries(const FString& SerializedData);
	
	// Save storage component data to ItemEntry CustomData
	UNKNOWN_API void SaveStorageToItemEntry(UStorageComponent* Storage, FItemEntry& ItemEntry);
	
	// Restore storage component data from ItemEntry CustomData
	UNKNOWN_API void RestoreStorageFromItemEntry(const FItemEntry& ItemEntry, UStorageComponent* Storage);
}

