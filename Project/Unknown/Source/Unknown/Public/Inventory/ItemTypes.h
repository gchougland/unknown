#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "ItemTypes.generated.h"

class UItemDefinition;

/** Per-item entry stored in containers (no stacking in v1) */
USTRUCT(BlueprintType)
struct FItemEntry
{
	GENERATED_BODY()

	// Shared definition (immutable metadata)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TObjectPtr<UItemDefinition> Def = nullptr;

	// Unique runtime id for this unit (assigned on add if invalid)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	FGuid ItemId;

	// Optional custom data (lightweight key/value strings for now)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Item")
	TMap<FName, FString> CustomData;

	// Blueprint-friendly helper functions for CustomData
	// Note: These are regular functions (UFUNCTION not allowed in structs), but they're still callable from Blueprints
	void SetCustomDataValue(FName Key, const FString& Value)
	{
		CustomData.Add(Key, Value);
	}

	FString GetCustomDataValue(FName Key, const FString& DefaultValue = TEXT("")) const
	{
		const FString* FoundValue = CustomData.Find(Key);
		return FoundValue ? *FoundValue : DefaultValue;
	}

	bool HasCustomDataKey(FName Key) const
	{
		return CustomData.Contains(Key);
	}

	void RemoveCustomDataKey(FName Key)
	{
		CustomData.Remove(Key);
	}

	void ClearCustomData()
	{
		CustomData.Empty();
	}

	bool IsValid() const { return Def != nullptr; }
};
