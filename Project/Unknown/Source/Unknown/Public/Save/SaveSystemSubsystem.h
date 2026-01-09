// Save system subsystem for managing save/load operations
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveSystemSubsystem.generated.h"

// Structure containing save slot information
USTRUCT(BlueprintType)
struct FSaveSlotInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString SlotId;

	UPROPERTY(BlueprintReadOnly)
	FString SaveName;

	UPROPERTY(BlueprintReadOnly)
	FString Timestamp;

	UPROPERTY(BlueprintReadOnly)
	bool bExists;

	FSaveSlotInfo()
		: SlotId(TEXT(""))
		, SaveName(TEXT(""))
		, Timestamp(TEXT(""))
		, bExists(false)
	{
	}
};

UCLASS()
class UNKNOWN_API USaveSystemSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// USubsystem interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Save the game to a slot
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	bool SaveGame(const FString& SlotId, const FString& SaveName);

	// Load the game from a slot
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	bool LoadGame(const FString& SlotId);

	// Delete a save slot
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	bool DeleteSave(const FString& SlotId);

	// Get all save slots (existing and empty)
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	TArray<FSaveSlotInfo> GetAllSaveSlots() const;

	// Get save slot info for a specific slot
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	FSaveSlotInfo GetSaveSlotInfo(const FString& SlotId) const;

	// Check if a save slot exists
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	bool DoesSaveSlotExist(const FString& SlotId) const;

	// Get the most recent save slot
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	FSaveSlotInfo GetMostRecentSaveSlot() const;

	// Create a new save at the player's current spawn location (for new games)
	UFUNCTION(BlueprintCallable, Category="SaveSystem")
	bool CreateNewGameSave(const FString& SlotId, const FString& SaveName);

	// Check for and create pending new game save (called after level loads)
	void CheckAndCreatePendingNewGameSave();

private:
	// Internal function to perform the actual loading after fade completes
	bool LoadGameAfterFade(const FString& SlotId);

	// Internal function to fade in after loading completes (for same-level loads)
	void FadeInAfterLoad();

public:
	// Get the save slot name from slot ID and save name (for use with UGameplayStatics)
	// Format: SaveSlot_{SlotId}_{SanitizedSaveName}
	FString GetSlotName(const FString& SlotId, const FString& SaveName = TEXT("")) const;
	
	// Get slot ID from slot name (extracts SlotId from filename with save name)
	FString GetSlotIdFromSlotName(const FString& SlotName) const;
	
	// Sanitize a save name for use in filename (removes invalid characters)
	FString SanitizeSaveNameForFilename(const FString& SaveName) const;
	
	// Pending new game save info (stored in temp save slot)
	static constexpr const TCHAR* PendingNewGameSlotPrefix = TEXT("_NEWGAME_");
	
	// Pending level to load (set when level transition is needed)
	FString PendingLevelLoad;
	
	// Current/running save game data - this is the active save game that gets updated when saving
	// It's either loaded from disk (when loading) or created fresh (when starting new game)
	// This ensures the baseline is preserved across saves
	UPROPERTY()
	class UGameSaveData* CurrentSaveData;
	
	// Current slot ID and save name for the running save game
	FString CurrentSlotId;
	FString CurrentSaveName;
};

