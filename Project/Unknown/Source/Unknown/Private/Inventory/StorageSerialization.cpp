#include "Inventory/StorageSerialization.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Engine/AssetManager.h"
#include "UObject/UObjectGlobals.h"

namespace StorageSerialization
{
	FString SerializeStorageEntries(const TArray<FItemEntry>& Entries)
	{
		FString Result;
		Result += FString::FromInt(Entries.Num());
		
		for (const FItemEntry& Entry : Entries)
		{
			if (!Entry.Def)
			{
				continue;
			}
			
			// Store the asset path of the ItemDefinition
			FString DefPath = Entry.Def->GetPathName();
			FString ItemIdStr = Entry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces);
			
			Result += TEXT("|");
			Result += DefPath;
			Result += TEXT("|");
			Result += ItemIdStr;
		}
		
		return Result;
	}
	
	TArray<FItemEntry> DeserializeStorageEntries(const FString& SerializedData)
	{
		TArray<FItemEntry> Result;
		
		if (SerializedData.IsEmpty())
		{
			return Result;
		}
		
		TArray<FString> Parts;
		SerializedData.ParseIntoArray(Parts, TEXT("|"), true);
		
		if (Parts.Num() < 1)
		{
			return Result;
		}
		
		int32 EntryCount = 0;
		if (!LexTryParseString(EntryCount, *Parts[0]))
		{
			return Result;
		}
		
		// Each entry takes 2 parts (DefPath, ItemId)
		const int32 ExpectedParts = 1 + (EntryCount * 2);
		if (Parts.Num() < ExpectedParts)
		{
			UE_LOG(LogTemp, Warning, TEXT("[StorageSerialization] DeserializeStorageEntries: Invalid format, expected %d parts, got %d"), ExpectedParts, Parts.Num());
			return Result;
		}
		
		int32 PartIndex = 1;
		for (int32 i = 0; i < EntryCount; ++i)
		{
			if (PartIndex + 1 >= Parts.Num())
			{
				break;
			}
			
			FString DefPath = Parts[PartIndex++];
			FString ItemIdStr = Parts[PartIndex++];
			
			// Load the ItemDefinition from path
			UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *DefPath);
			if (!Def)
			{
				UE_LOG(LogTemp, Warning, TEXT("[StorageSerialization] Failed to load ItemDefinition from path: %s"), *DefPath);
				continue;
			}
			
			FGuid ItemId;
			if (!FGuid::Parse(ItemIdStr, ItemId))
			{
				UE_LOG(LogTemp, Warning, TEXT("[StorageSerialization] Failed to parse ItemId: %s"), *ItemIdStr);
				continue;
			}
			
			FItemEntry Entry;
			Entry.Def = Def;
			Entry.ItemId = ItemId;
			Result.Add(Entry);
		}
		
		return Result;
	}
	
	void SaveStorageToItemEntry(UStorageComponent* Storage, FItemEntry& ItemEntry)
	{
		if (!Storage)
		{
			return;
		}
		
		FString Serialized = SerializeStorageEntries(Storage->GetEntries());
		ItemEntry.CustomData.Add(TEXT("StorageData"), Serialized);
		
		// Also store MaxVolume
		ItemEntry.CustomData.Add(TEXT("StorageMaxVolume"), FString::SanitizeFloat(Storage->MaxVolume));
	}
	
	void RestoreStorageFromItemEntry(const FItemEntry& ItemEntry, UStorageComponent* Storage)
	{
		if (!Storage)
		{
			return;
		}
		
		const FString* SerializedData = ItemEntry.CustomData.Find(TEXT("StorageData"));
		if (!SerializedData || SerializedData->IsEmpty())
		{
			// No storage data to restore
			return;
		}
		
		// Restore MaxVolume if stored
		const FString* MaxVolumeStr = ItemEntry.CustomData.Find(TEXT("StorageMaxVolume"));
		if (MaxVolumeStr)
		{
			float MaxVolume = 0.f;
			if (LexTryParseString(MaxVolume, **MaxVolumeStr))
			{
				Storage->MaxVolume = MaxVolume;
			}
		}
		
		// Deserialize and restore entries
		TArray<FItemEntry> RestoredEntries = DeserializeStorageEntries(*SerializedData);
		Storage->Entries = RestoredEntries;
		
		UE_LOG(LogTemp, Display, TEXT("[StorageSerialization] Restored %d entries to storage component"), RestoredEntries.Num());
	}
}

