#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "Player/FirstPersonCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/LevelStreaming.h"
#include "HAL/PlatformFilemanager.h"
#include "HAL/FileManagerGeneric.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "HAL/PlatformFile.h"
#include "UObject/Object.h"

void USaveSystemSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

FString USaveSystemSubsystem::SanitizeSaveNameForFilename(const FString& SaveName) const
{
	FString Sanitized = SaveName;
	
	// Replace invalid filename characters with underscores
	// Windows/Unreal invalid chars: < > : " / \ | ? * and control characters
	Sanitized.ReplaceInline(TEXT("<"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT(">"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT(":"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("\""), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("/"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("\\"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("|"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("?"), TEXT("_"));
	Sanitized.ReplaceInline(TEXT("*"), TEXT("_"));
	
	// Remove leading/trailing spaces and dots (Windows doesn't allow these)
	Sanitized.TrimStartAndEndInline();
	
	// Remove leading/trailing dots (Windows doesn't allow these as filenames)
	while (Sanitized.StartsWith(TEXT(".")))
	{
		Sanitized = Sanitized.RightChop(1);
	}
	while (Sanitized.EndsWith(TEXT(".")))
	{
		Sanitized = Sanitized.LeftChop(1);
	}
	
	// Limit length to avoid filesystem issues (keep reasonable filename length)
	if (Sanitized.Len() > 50)
	{
		Sanitized = Sanitized.Left(50);
	}
	
	// If empty after sanitization, use a default
	if (Sanitized.IsEmpty())
	{
		Sanitized = TEXT("Save");
	}
	
	return Sanitized;
}

FString USaveSystemSubsystem::GetSlotName(const FString& SlotId, const FString& SaveName) const
{
	FString SlotName = FString::Printf(TEXT("SaveSlot_%s"), *SlotId);
	
	// Append save name if provided (sanitized)
	if (!SaveName.IsEmpty())
	{
		FString SanitizedName = SanitizeSaveNameForFilename(SaveName);
		SlotName += FString::Printf(TEXT("_%s"), *SanitizedName);
	}
	
	return SlotName;
}

FString USaveSystemSubsystem::GetSlotIdFromSlotName(const FString& SlotName) const
{
	// Format: SaveSlot_{SlotId}_{SaveName}
	// SlotId format is always "slot_{timestamp}", so we extract the first two underscore-separated parts
	
	// Remove "SaveSlot_" prefix
	if (SlotName.StartsWith(TEXT("SaveSlot_")))
	{
		FString WithoutPrefix = SlotName.RightChop(9); // Length of "SaveSlot_"
		
		// Split by underscore - SlotId is always "slot_{timestamp}" (first two parts)
		TArray<FString> Parts;
		WithoutPrefix.ParseIntoArray(Parts, TEXT("_"), true);
		
		if (Parts.Num() >= 2)
		{
			// SlotId is "slot_{timestamp}", so combine first two parts
			return FString::Printf(TEXT("%s_%s"), *Parts[0], *Parts[1]);
		}
		
		return WithoutPrefix;
	}
	
	return SlotName;
}

bool USaveSystemSubsystem::SaveGame(const FString& SlotId, const FString& SaveName)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: SlotId is empty"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: World is null"));
		return false;
	}

	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot save: Player character not found"));
		return false;
	}

	// Create save game object
	UGameSaveData* SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create save game object"));
		return false;
	}

	// Save player location and rotation
	SaveGameInstance->PlayerLocation = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerRotation = PlayerCharacter->GetActorRotation();
	SaveGameInstance->SaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %s"), *SlotId) : SaveName;

	// Save current level package path (for standalone builds, use package path)
	// Get the level's package - this works in both editor and standalone
	FString LevelPackagePath;
	
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			// Get the package from the level's outer
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				LevelPackagePath = LevelPackage->GetName();
			}
		}
	}
	
	// Fallback: try to get from world's outer
	if (LevelPackagePath.IsEmpty())
	{
		if (UObject* WorldOuter = World->GetOuter())
		{
			if (UPackage* WorldPackage = WorldOuter->GetPackage())
			{
				LevelPackagePath = WorldPackage->GetName();
			}
		}
	}
	
	// Last resort: use map name and construct package path
	if (LevelPackagePath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			// Construct package path (e.g., "/Game/Levels/MainMap")
			LevelPackagePath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			// Absolute fallback: use world path
			FString WorldPath = World->GetPathName();
			int32 DotIndex = WorldPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (DotIndex != INDEX_NONE)
			{
				LevelPackagePath = WorldPath.Left(DotIndex);
			}
			else
			{
				LevelPackagePath = WorldPath;
			}
		}
	}
	
	// Normalize the level path by removing PIE prefixes (e.g., "UEDPIE_0_MainMap" -> "MainMap")
	// This ensures saves work in both PIE and standalone builds
	FString NormalizedPath = LevelPackagePath;
	
	// Remove PIE prefix if present (e.g., "/Game/Levels/UEDPIE_0_MainMap" -> "/Game/Levels/MainMap")
	// PIE prefixes can be like "UEDPIE_0_", "UEDPIE_1_", etc.
	if (NormalizedPath.Contains(TEXT("UEDPIE_")))
	{
		// Find the last slash before the level name
		int32 LastSlash = NormalizedPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (LastSlash != INDEX_NONE)
		{
			FString PathPrefix = NormalizedPath.Left(LastSlash + 1); // Include the slash
			FString LevelName = NormalizedPath.RightChop(LastSlash + 1);
			
			// Remove PIE prefix from level name (e.g., "UEDPIE_0_MainMap" -> "MainMap")
			// PIE prefix format is typically "UEDPIE_X_" where X is a number
			if (LevelName.StartsWith(TEXT("UEDPIE_")))
			{
				// Find the first underscore after "UEDPIE"
				int32 FirstUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
				if (FirstUnderscore != INDEX_NONE)
				{
					// Find the second underscore (after the number)
					int32 SecondUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstUnderscore + 1);
					if (SecondUnderscore != INDEX_NONE)
					{
						// Extract everything after the PIE prefix
						LevelName = LevelName.RightChop(SecondUnderscore + 1);
						NormalizedPath = PathPrefix + LevelName;
					}
				}
			}
		}
	}
	
	SaveGameInstance->LevelPackagePath = NormalizedPath;
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Saving level package path: %s (original: %s, MapName: %s)"), 
		*NormalizedPath, *LevelPackagePath, *World->GetMapName());

	// Set timestamp
	FDateTime Now = FDateTime::Now();
	SaveGameInstance->Timestamp = Now.ToString(TEXT("%Y.%m.%d %H:%M:%S"));

	// Save to slot (include save name in filename)
	FString SlotName = GetSlotName(SlotId, SaveName);
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, 0);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully saved game to slot: %s"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to save game to slot: %s"), *SlotName);
	}

	return bSuccess;
}

bool USaveSystemSubsystem::LoadGame(const FString& SlotId)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: SlotId is empty"));
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: World is null"));
		return false;
	}

	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	UGameSaveData* SaveGameInstance = nullptr;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - try to load it
			SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0));
			if (SaveGameInstance)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to load save game from slot: %s"), *SlotId);
		return false;
	}
	
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Loaded save data - LevelPath: %s, SaveName: %s, Position: %s"), 
		*SaveGameInstance->LevelPackagePath, *SaveGameInstance->SaveName, *SaveGameInstance->PlayerLocation.ToString());

	// Check if we need to load a different level
	FString CurrentLevelPath;
	
	// Get current level package path (same logic as saving)
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				CurrentLevelPath = LevelPackage->GetName();
			}
		}
	}
	
	if (CurrentLevelPath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			CurrentLevelPath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			FString WorldPath = World->GetPathName();
			int32 DotIndex = WorldPath.Find(TEXT("."), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
			if (DotIndex != INDEX_NONE)
			{
				CurrentLevelPath = WorldPath.Left(DotIndex);
			}
			else
			{
				CurrentLevelPath = WorldPath;
			}
		}
	}

	// Always load the save's level if we have a level path in the save
	// This ensures we load the correct level even if the current level detection fails
	bool bShouldLoadLevel = !SaveGameInstance->LevelPackagePath.IsEmpty();
	
	// If the save is from a different level, or we're loading from main menu (which might not detect correctly),
	// always load the save's level
	if (bShouldLoadLevel)
	{
		// Check if we're in MainMenu - if so, always load the save's level
		FString MapName = World->GetMapName();
		bool bIsMainMenu = MapName.Contains(TEXT("MainMenu"), ESearchCase::IgnoreCase);
		
		// Load level if we're in a different level OR if we're in MainMenu (always load from main menu)
		if (bIsMainMenu || CurrentLevelPath != SaveGameInstance->LevelPackagePath)
		{
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Loading level: %s (current: %s)"), 
				*SaveGameInstance->LevelPackagePath, *CurrentLevelPath);
			
			// Convert package path to level name for OpenLevel
			// OpenLevel expects the level name without the package path prefix
			FString LevelName = SaveGameInstance->LevelPackagePath;
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Original saved level path: %s"), *LevelName);
			
			// Remove "/Game/" prefix if present for OpenLevel
			if (LevelName.StartsWith(TEXT("/Game/")))
			{
				LevelName = LevelName.RightChop(6); // Remove "/Game/"
			}
			// Remove leading slash if present
			if (LevelName.StartsWith(TEXT("/")))
			{
				LevelName = LevelName.RightChop(1);
			}
			
			// If LevelName still contains "Levels/", extract just the level name
			// For example: "Levels/MainMap" -> "MainMap"
			if (LevelName.Contains(TEXT("/")))
			{
				int32 LastSlash = LevelName.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
				if (LastSlash != INDEX_NONE)
				{
					LevelName = LevelName.RightChop(LastSlash + 1); // Get everything after the last slash
				}
			}
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Converted level name for OpenLevel: %s"), *LevelName);
			
			// Store save data for restoration after level loads
			// Save position/rotation to temporary slot that will be loaded after level transition
			FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
			UGameplayStatics::SaveGameToSlot(SaveGameInstance, TempSlotName, 0);
			
			// Store the slot name in a static or game instance variable so it can be accessed after level loads
			// We'll restore the position in the PlayerController's BeginPlay or PostLogin
			
			// Open the level - player position will be restored after level loads
			UGameplayStatics::OpenLevel(World, *LevelName);
			
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Opened level: %s - position will be restored after load"), *LevelName);
			
			// Position restoration will be handled in FirstPersonPlayerController::BeginPlay
			// by checking for the temp save file
			
			return true;
		}
	}

	// Same level - just restore player position
	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot load: Player character not found"));
		return false;
	}

	// Restore player location and rotation
	PlayerCharacter->SetActorLocation(SaveGameInstance->PlayerLocation);
	PlayerCharacter->SetActorRotation(SaveGameInstance->PlayerRotation);

	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully loaded game from slot: %s"), *SlotName);
	return true;
}

bool USaveSystemSubsystem::DeleteSave(const FString& SlotId)
{
	if (SlotId.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot delete: SlotId is empty"));
		return false;
	}

	// Find and delete the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	bool bSuccess = false;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - delete it
			bSuccess = UGameplayStatics::DeleteGameInSlot(FileName, 0);
			if (bSuccess)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully deleted save slot: %s"), *SlotName);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Failed to delete save slot: %s"), *SlotName);
	}

	return bSuccess;
}

FSaveSlotInfo USaveSystemSubsystem::GetSaveSlotInfo(const FString& SlotId) const
{
	FSaveSlotInfo Info;
	Info.SlotId = SlotId;

	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	FString SlotName;
	UGameSaveData* SaveGameInstance = nullptr;
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file - try to load it
			SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0));
			if (SaveGameInstance)
			{
				SlotName = FileName;
				break;
			}
		}
	}
	
	if (SaveGameInstance)
	{
		Info.bExists = true;
		Info.SaveName = SaveGameInstance->GetSaveName();
		Info.Timestamp = SaveGameInstance->GetFormattedTimestamp();
	}
	else
	{
		Info.bExists = false;
	}

	return Info;
}

bool USaveSystemSubsystem::DoesSaveSlotExist(const FString& SlotId) const
{
	// Find the save file - it has the save name in the filename
	// Search for files starting with SaveSlot_{SlotId}_
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	TArray<FString> SaveFiles;
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));
	
	FString SlotPrefix = FString::Printf(TEXT("SaveSlot_%s_"), *SlotId);
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		if (FileName.StartsWith(SlotPrefix))
		{
			// Found a matching file
			return true;
		}
	}
	
	return false;
}

TArray<FSaveSlotInfo> USaveSystemSubsystem::GetAllSaveSlots() const
{
	TArray<FSaveSlotInfo> SaveSlots;

	// Enumerate save files from the save directory
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	
	if (!FileManager.DirectoryExists(*SaveDir))
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Save directory does not exist: %s"), *SaveDir);
		return SaveSlots;
	}

	// Use IFileManager::FindFiles - this requires full file paths with wildcards
	// FindFiles signature: FindFiles(OutFileNames, StartDirectory, FileExtension)
	// But IFileManager::FindFiles works differently - it takes a search pattern
	TArray<FString> SaveFiles;
	
	// Try using the correct IFileManager::FindFiles signature
	// Parameters: OutFileNames, StartDirectory, FileExtension, bFiles, bDirectories, bRecursive
	FileManager.FindFiles(SaveFiles, *SaveDir, TEXT("*.sav"));

	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Searching in directory: %s"), *SaveDir);
	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Found %d save files using FindFiles"), SaveFiles.Num());

	// If FindFiles still returns 0, try using IterateDirectory with a visitor
	if (SaveFiles.Num() == 0)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] FindFiles returned 0 files, trying directory iteration"));
		
		// Manually iterate through directory to find all .sav files
		struct FSaveFileVisitor : public IPlatformFile::FDirectoryVisitor
		{
			TArray<FString>& FoundFiles;
			FString SaveDirPath;
			
			FSaveFileVisitor(TArray<FString>& InFoundFiles, const FString& InSaveDir)
				: FoundFiles(InFoundFiles), SaveDirPath(InSaveDir)
			{}
			
			virtual bool Visit(const TCHAR* FilenameOrDirectory, bool bIsDirectory) override
			{
				if (!bIsDirectory)
				{
					FString FilePath(FilenameOrDirectory);
					if (FilePath.EndsWith(TEXT(".sav")))
					{
						FString RelativePath = FilePath;
						FPaths::MakePathRelativeTo(RelativePath, *(SaveDirPath / TEXT("")));
						FoundFiles.Add(RelativePath);
						UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Found save file via visitor: %s (relative: %s)"), 
							*FilePath, *RelativePath);
					}
				}
				return true;
			}
		};
		
		FSaveFileVisitor Visitor(SaveFiles, SaveDir);
		FileManager.IterateDirectory(*SaveDir, Visitor);
		
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Found %d save files via directory iteration"), SaveFiles.Num());
	}

	// Process each save file
	for (const FString& SaveFile : SaveFiles)
	{
		// Get the base filename without extension and path
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Processing save file: %s (base filename: %s)"), *SaveFile, *FileName);
		
		// Check if it's a save slot (starts with "SaveSlot_")
		if (FileName.StartsWith(TEXT("SaveSlot_")))
		{
			FString SlotId = GetSlotIdFromSlotName(FileName);
			UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Extracted slot ID: %s from filename: %s"), *SlotId, *FileName);
			
			FSaveSlotInfo Info = GetSaveSlotInfo(SlotId);
			if (Info.bExists)
			{
				UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Found valid save slot: %s (Name: %s, Timestamp: %s)"), 
					*SlotId, *Info.SaveName, *Info.Timestamp);
				SaveSlots.Add(Info);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Save slot file exists but failed to load: %s"), *SlotId);
			}
		}
		else
		{
			UE_LOG(LogTemp, Verbose, TEXT("[SaveSystem] File does not start with 'SaveSlot_': %s"), *FileName);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Returning %d valid save slots"), SaveSlots.Num());

	// Sort by timestamp (most recent first)
	SaveSlots.Sort([](const FSaveSlotInfo& A, const FSaveSlotInfo& B) {
		return A.Timestamp > B.Timestamp;
	});

	return SaveSlots;
}

FSaveSlotInfo USaveSystemSubsystem::GetMostRecentSaveSlot() const
{
	TArray<FSaveSlotInfo> AllSlots = GetAllSaveSlots();
	
	if (AllSlots.Num() > 0)
	{
		return AllSlots[0]; // Already sorted by timestamp
	}

	return FSaveSlotInfo();
}

bool USaveSystemSubsystem::CreateNewGameSave(const FString& SlotId, const FString& SaveName)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot create new game save: World is null"));
		return false;
	}

	// Get player character
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
	if (!PlayerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SaveSystem] Cannot create new game save: Player character not found"));
		return false;
	}

	// Create save game object
	UGameSaveData* SaveGameInstance = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
	if (!SaveGameInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create save game object"));
		return false;
	}

	// Save player location and rotation (current spawn location)
	SaveGameInstance->PlayerLocation = PlayerCharacter->GetActorLocation();
	SaveGameInstance->PlayerRotation = PlayerCharacter->GetActorRotation();
	SaveGameInstance->SaveName = SaveName.IsEmpty() ? FString::Printf(TEXT("Save %s"), *SlotId) : SaveName;
	
	// Save current level package path (should be MainMap for new games)
	FString LevelPackagePath;
	if (ULevel* PersistentLevel = World->GetCurrentLevel())
	{
		if (UObject* LevelOuter = PersistentLevel->GetOuter())
		{
			if (UPackage* LevelPackage = LevelOuter->GetPackage())
			{
				LevelPackagePath = LevelPackage->GetName();
			}
		}
	}
	
	if (LevelPackagePath.IsEmpty())
	{
		FString MapName = World->GetMapName();
		if (!MapName.IsEmpty())
		{
			LevelPackagePath = FString::Printf(TEXT("/Game/Levels/%s"), *MapName);
		}
		else
		{
			LevelPackagePath = TEXT("/Game/Levels/MainMap");
		}
	}
	
	// Normalize the level path by removing PIE prefixes (same as SaveGame)
	FString NormalizedPath = LevelPackagePath;
	if (NormalizedPath.Contains(TEXT("UEDPIE_")))
	{
		int32 LastSlash = NormalizedPath.Find(TEXT("/"), ESearchCase::CaseSensitive, ESearchDir::FromEnd);
		if (LastSlash != INDEX_NONE)
		{
			FString PathPrefix = NormalizedPath.Left(LastSlash + 1);
			FString LevelName = NormalizedPath.RightChop(LastSlash + 1);
			
			if (LevelName.StartsWith(TEXT("UEDPIE_")))
			{
				int32 FirstUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart);
				if (FirstUnderscore != INDEX_NONE)
				{
					int32 SecondUnderscore = LevelName.Find(TEXT("_"), ESearchCase::CaseSensitive, ESearchDir::FromStart, FirstUnderscore + 1);
					if (SecondUnderscore != INDEX_NONE)
					{
						LevelName = LevelName.RightChop(SecondUnderscore + 1);
						NormalizedPath = PathPrefix + LevelName;
					}
				}
			}
		}
	}
	
	SaveGameInstance->LevelPackagePath = NormalizedPath;
	
	// Set timestamp
	FDateTime Now = FDateTime::Now();
	SaveGameInstance->Timestamp = Now.ToString(TEXT("%Y.%m.%d %H:%M:%S"));

	// Save to slot (include save name in filename)
	FString SlotName = GetSlotName(SlotId, SaveName);
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveGameInstance, SlotName, 0);
	
	if (bSuccess)
	{
		UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Successfully created new game save to slot: %s (Level: %s)"), *SlotName, *LevelPackagePath);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SaveSystem] Failed to create new game save to slot: %s"), *SlotName);
	}

	return bSuccess;
}

void USaveSystemSubsystem::CheckAndCreatePendingNewGameSave()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check for any pending new game saves (stored with prefix "_NEWGAME_")
	FString SaveDir = FPaths::ProjectSavedDir() / TEXT("SaveGames");
	IFileManager& FileManager = IFileManager::Get();
	
	if (!FileManager.DirectoryExists(*SaveDir))
	{
		return;
	}

	// Find all pending new game saves
	TArray<FString> SaveFiles;
	FString SearchPattern = SaveDir / TEXT("*.sav");
	FileManager.FindFiles(SaveFiles, *SearchPattern, true, false);
	
	for (const FString& SaveFile : SaveFiles)
	{
		FString FileName = FPaths::GetBaseFilename(SaveFile);
		
		// Check if it's a pending new game save
		if (FileName.StartsWith(TEXT("_NEWGAME_")))
		{
			// Load the pending save info
			if (UGameSaveData* PendingSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(FileName, 0)))
			{
				// Extract slot ID from filename (remove "_NEWGAME_" prefix)
				FString SlotId = FileName.RightChop(9); // Length of "_NEWGAME_"
				
				// Wait for player to spawn, then create the actual save
				// Use a timer to ensure player is fully loaded
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, [this, World, SlotId, PendingSave]()
				{
					// Get player character
					APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
					AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn);
					
					if (PlayerCharacter)
					{
						// Create the actual save with current player position
						CreateNewGameSave(SlotId, PendingSave->SaveName);
						
						// Clean up pending save
						FString PendingSlotName = FString::Printf(TEXT("_NEWGAME_%s"), *SlotId);
						UGameplayStatics::DeleteGameInSlot(PendingSlotName, 0);
						
						UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Created new game save: %s"), *SlotId);
					}
				}, 0.5f, false); // 0.5 second delay to ensure player is spawned
				
				break; // Only handle one pending save at a time
			}
		}
	}
}

