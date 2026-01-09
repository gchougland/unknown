#include "Save/SaveSystemHelpers.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/EquipmentTypes.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/Engine.h"

namespace SaveSystemHelpers
{
	FString SerializeItemEntry(const FItemEntry& Entry)
	{
		if (!Entry.IsValid() || !Entry.Def)
		{
			return FString();
		}

		FString Result;
		
		// Store ItemDefinition path
		FString DefPath = Entry.Def->GetPathName();
		Result += DefPath;
		
		// Store ItemId
		Result += TEXT("|");
		Result += Entry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces);
		
		// Store CustomData
		for (const auto& Pair : Entry.CustomData)
		{
			Result += TEXT("|");
			Result += Pair.Key.ToString();
			Result += TEXT("=");
			Result += Pair.Value;
		}
		
		return Result;
	}

	bool DeserializeItemEntry(const FString& SerializedData, FItemEntry& OutEntry)
	{
		if (SerializedData.IsEmpty())
		{
			return false;
		}

		TArray<FString> Parts;
		SerializedData.ParseIntoArray(Parts, TEXT("|"), true);
		
		if (Parts.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] DeserializeItemEntry: Invalid format, need at least 2 parts"));
			return false;
		}

		// Load ItemDefinition from path
		FString DefPath = Parts[0];
		UItemDefinition* Def = LoadObject<UItemDefinition>(nullptr, *DefPath);
		if (!Def)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] Failed to load ItemDefinition from path: %s"), *DefPath);
			return false;
		}

		// Parse ItemId
		FGuid ItemId;
		if (!FGuid::Parse(Parts[1], ItemId))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] Failed to parse ItemId: %s"), *Parts[1]);
			return false;
		}

		OutEntry.Def = Def;
		OutEntry.ItemId = ItemId;
		OutEntry.CustomData.Empty();

		// Parse CustomData (format: "Key=Value")
		for (int32 i = 2; i < Parts.Num(); ++i)
		{
			FString Part = Parts[i];
			int32 EqualsIndex = Part.Find(TEXT("="));
			if (EqualsIndex != INDEX_NONE)
			{
				FString Key = Part.Left(EqualsIndex);
				FString Value = Part.RightChop(EqualsIndex + 1);
				OutEntry.CustomData.Add(*Key, Value);
			}
		}

		return true;
	}

	FString SerializeEquipmentSlot(EEquipmentSlot Slot, const FItemEntry& Entry)
	{
		FString Result;
		
		// Store slot index as uint8
		uint8 SlotIndex = static_cast<uint8>(Slot);
		Result += FString::FromInt(SlotIndex);
		
		// Store item entry data
		Result += TEXT("|");
		Result += SerializeItemEntry(Entry);
		
		return Result;
	}

	bool DeserializeEquipmentSlot(const FString& SerializedData, EEquipmentSlot& OutSlot, FItemEntry& OutEntry)
	{
		if (SerializedData.IsEmpty())
		{
			return false;
		}

		TArray<FString> Parts;
		SerializedData.ParseIntoArray(Parts, TEXT("|"), true);
		
		if (Parts.Num() < 2)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] DeserializeEquipmentSlot: Invalid format, need at least 2 parts"));
			return false;
		}

		// Parse slot index
		int32 SlotIndex = 0;
		if (!LexTryParseString(SlotIndex, *Parts[0]))
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] Failed to parse slot index: %s"), *Parts[0]);
			return false;
		}

		if (SlotIndex < 0 || SlotIndex > 7)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SaveSystemHelpers] Invalid slot index: %d"), SlotIndex);
			return false;
		}

		OutSlot = static_cast<EEquipmentSlot>(SlotIndex);

		// Reconstruct item entry string (everything after first |)
		FString ItemEntryString = Parts[1];
		for (int32 i = 2; i < Parts.Num(); ++i)
		{
			ItemEntryString += TEXT("|");
			ItemEntryString += Parts[i];
		}

		return DeserializeItemEntry(ItemEntryString, OutEntry);
	}

	bool ShouldSavePhysicsObject(AActor* Actor, const FTransform& OriginalTransform, float PositionThreshold, float RotationThreshold)
	{
		if (!Actor)
		{
			return false;
		}

		// Check if actor has physics simulation
		bool bHasPhysics = false;
		TArray<UPrimitiveComponent*> PrimitiveComponents;
		Actor->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
		
		for (UPrimitiveComponent* Comp : PrimitiveComponents)
		{
			if (Comp && Comp->IsSimulatingPhysics())
			{
				bHasPhysics = true;
				break;
			}
		}

		if (!bHasPhysics)
		{
			return false;
		}

		// Check if transform differs significantly from original
		FTransform CurrentTransform = Actor->GetActorTransform();
		
		FVector PositionDelta = CurrentTransform.GetLocation() - OriginalTransform.GetLocation();
		if (PositionDelta.Size() > PositionThreshold)
		{
			return true;
		}

		// Check rotation difference
		FRotator RotationDelta = (CurrentTransform.GetRotation() * OriginalTransform.GetRotation().Inverse()).Rotator();
		if (FMath::Abs(RotationDelta.Pitch) > RotationThreshold ||
			FMath::Abs(RotationDelta.Yaw) > RotationThreshold ||
			FMath::Abs(RotationDelta.Roll) > RotationThreshold)
		{
			return true;
		}

		// Check scale difference
		FVector ScaleDelta = CurrentTransform.GetScale3D() - OriginalTransform.GetScale3D();
		if (ScaleDelta.Size() > 0.01f)
		{
			return true;
		}

		return false;
	}
}


