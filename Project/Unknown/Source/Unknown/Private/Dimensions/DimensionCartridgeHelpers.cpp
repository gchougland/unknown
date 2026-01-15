#include "Dimensions/DimensionCartridgeHelpers.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemTypes.h"
#include "Dimensions/DimensionDefinition.h"
#include "Dimensions/DimensionInstanceData.h"
#include "Engine/Engine.h"
#include "UObject/UObjectGlobals.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Base64.h"
#include "Misc/Compression.h"

const FName UDimensionCartridgeHelpers::CartridgeDataKey = TEXT("DimensionCartridgeData");

UDimensionCartridgeData* UDimensionCartridgeHelpers::GetCartridgeDataFromItemEntry(const FItemEntry& ItemEntry)
{
	// Check if item has cartridge data in CustomData
	const FString* CartridgeDataStr = ItemEntry.CustomData.Find(CartridgeDataKey);
	if (!CartridgeDataStr || CartridgeDataStr->IsEmpty())
	{
		return nullptr;
	}

	// Deserialize cartridge data from string
	// The data is Base64 encoded to avoid pipe character conflicts with ItemEntry serialization
	// Format (after decoding): "DimensionDefPath|InstanceId|CartridgeId|Stability|WorldPosX,WorldPosY,WorldPosZ"
	FString DecodedData;
	if (!FBase64::Decode(*CartridgeDataStr, DecodedData))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Failed to decode Base64 cartridge data"));
		return nullptr;
	}

	TArray<FString> Parts;
	DecodedData.ParseIntoArray(Parts, TEXT("|"), true);

	if (Parts.Num() < 5)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Invalid cartridge data format: expected 5 parts, got %d. Decoded data: %s"), Parts.Num(), *DecodedData);
		return nullptr;
	}

	// Create cartridge data object
	UDimensionCartridgeData* CartridgeData = NewObject<UDimensionCartridgeData>();
	if (!CartridgeData)
	{
		return nullptr;
	}

	// Load dimension definition
	FString DimensionDefPath = Parts[0];
	UDimensionDefinition* DimensionDef = LoadObject<UDimensionDefinition>(nullptr, *DimensionDefPath);
	if (!DimensionDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Failed to load dimension definition: %s"), *DimensionDefPath);
		return nullptr;
	}

	CartridgeData->DimensionDef = DimensionDef;

	// Parse instance data
	FGuid InstanceId;
	if (!FGuid::Parse(Parts[1], InstanceId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Failed to parse InstanceId: %s"), *Parts[1]);
		return nullptr;
	}
	
	FGuid CartridgeId;
	if (!FGuid::Parse(Parts[2], CartridgeId))
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Failed to parse CartridgeId: %s"), *Parts[2]);
		return nullptr;
	}
	CartridgeData->CartridgeId = CartridgeId;

	float Stability = FCString::Atof(*Parts[3]);
	
	// Parse WorldPosition as comma-separated values (X,Y,Z) to avoid equals signs
	FVector WorldPosition = FVector::ZeroVector;
	TArray<FString> PositionParts;
	Parts[4].ParseIntoArray(PositionParts, TEXT(","), true);
	if (PositionParts.Num() == 3)
	{
		WorldPosition.X = FCString::Atof(*PositionParts[0]);
		WorldPosition.Y = FCString::Atof(*PositionParts[1]);
		WorldPosition.Z = FCString::Atof(*PositionParts[2]);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Failed to parse WorldPosition: %s (expected X,Y,Z format)"), *Parts[4]);
		WorldPosition = FVector::ZeroVector;
	}

	// Create instance data
	UDimensionInstanceData* InstanceData = NewObject<UDimensionInstanceData>(CartridgeData);
	InstanceData->InstanceId = InstanceId;
	InstanceData->CartridgeId = CartridgeId;
	InstanceData->WorldPosition = WorldPosition;
	InstanceData->Stability = Stability;
	InstanceData->bIsLoaded = false;

	CartridgeData->InstanceData = InstanceData;

	return CartridgeData;
}

UDimensionCartridgeData* UDimensionCartridgeHelpers::GetCartridgeDataFromItem(AItemPickup* Item)
{
	if (!Item)
	{
		return nullptr;
	}

	FItemEntry ItemEntry = Item->GetItemEntry();
	return GetCartridgeDataFromItemEntry(ItemEntry);
}

void UDimensionCartridgeHelpers::SetCartridgeDataInItemEntry(FItemEntry& ItemEntry, UDimensionCartridgeData* CartridgeData)
{
	if (!CartridgeData || !CartridgeData->IsValid())
	{
		// Remove cartridge data if invalid
		ItemEntry.CustomData.Remove(CartridgeDataKey);
		return;
	}

	// Serialize cartridge data to string
	// Format (before encoding): "DimensionDefPath|InstanceId|CartridgeId|Stability|WorldPosX,WorldPosY,WorldPosZ"
	// Note: Using comma-separated format for WorldPosition to avoid equals signs that conflict with CustomData serialization
	// The entire string is Base64 encoded to avoid pipe character conflicts with ItemEntry serialization
	FString SerializedData;

	// Dimension definition path
	if (CartridgeData->DimensionDef)
	{
		SerializedData = CartridgeData->DimensionDef->GetPathName();
	}

	// Instance data
	if (CartridgeData->InstanceData)
	{
		FVector WorldPos = CartridgeData->InstanceData->WorldPosition;
		SerializedData += FString::Printf(TEXT("|%s|%s|%f|%f,%f,%f"),
			*CartridgeData->InstanceData->InstanceId.ToString(),
			*CartridgeData->CartridgeId.ToString(),
			CartridgeData->InstanceData->Stability,
			WorldPos.X, WorldPos.Y, WorldPos.Z
		);
	}
	else
	{
		// No instance data yet - use defaults
		SerializedData += FString::Printf(TEXT("|%s|%s|%f|0.0,0.0,0.0"),
			*FGuid().ToString(),
			*CartridgeData->CartridgeId.ToString(),
			100.0f
		);
	}

	// Base64 encode to avoid pipe character conflicts with ItemEntry serialization
	FString EncodedData = FBase64::Encode(SerializedData);
	ItemEntry.CustomData.Add(CartridgeDataKey, EncodedData);
}

bool UDimensionCartridgeHelpers::HasCartridgeData(const FItemEntry& ItemEntry)
{
	return ItemEntry.CustomData.Contains(CartridgeDataKey);
}

UDimensionCartridgeData* UDimensionCartridgeHelpers::CreateCartridgeFromDimension(UDimensionDefinition* DimensionDef, const FVector& SpawnPosition)
{
	if (!DimensionDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Cannot create cartridge: Invalid dimension definition"));
		return nullptr;
	}

	// Create new cartridge data
	UDimensionCartridgeData* CartridgeData = NewObject<UDimensionCartridgeData>();
	if (!CartridgeData)
	{
		return nullptr;
	}

	// Set dimension definition
	CartridgeData->DimensionDef = DimensionDef;

	// Create instance data
	UDimensionInstanceData* InstanceData = NewObject<UDimensionInstanceData>(CartridgeData);
	if (InstanceData)
	{
		InstanceData->InstanceId = FGuid::NewGuid();
		InstanceData->CartridgeId = CartridgeData->CartridgeId;
		InstanceData->WorldPosition = SpawnPosition;
		InstanceData->Stability = DimensionDef->DefaultStability;
		InstanceData->bIsLoaded = false;
		CartridgeData->InstanceData = InstanceData;
	}

	return CartridgeData;
}

FItemEntry UDimensionCartridgeHelpers::CreateItemEntryWithCartridge(UItemDefinition* CartridgeItemDef, UDimensionDefinition* DimensionDef, const FVector& SpawnPosition)
{
	FItemEntry ItemEntry;
	
	if (!CartridgeItemDef)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionCartridge] Cannot create item entry: Invalid item definition"));
		return ItemEntry;
	}

	// Set up basic item entry
	ItemEntry.Def = CartridgeItemDef;
	ItemEntry.ItemId = FGuid::NewGuid();

	// Create cartridge data and set it in the item entry
	UDimensionCartridgeData* CartridgeData = CreateCartridgeFromDimension(DimensionDef, SpawnPosition);
	if (CartridgeData)
	{
		SetCartridgeDataInItemEntry(ItemEntry, CartridgeData);
	}

	return ItemEntry;
}
