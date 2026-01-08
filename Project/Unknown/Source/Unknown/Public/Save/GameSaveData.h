// Game save data class for storing game state
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "GameSaveData.generated.h"

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

	// Player location
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FVector PlayerLocation;

	// Player rotation
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FRotator PlayerRotation;

	// Level package path (e.g., "/Game/Levels/MainMap")
	UPROPERTY(VisibleAnywhere, Category="SaveData")
	FString LevelPackagePath;

	// Get formatted timestamp string
	UFUNCTION(BlueprintPure, Category="SaveData")
	FString GetFormattedTimestamp() const { return Timestamp; }

	// Get save name
	UFUNCTION(BlueprintPure, Category="SaveData")
	FString GetSaveName() const { return SaveName; }
};

