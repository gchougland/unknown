#include "Save/GameSaveData.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/DateTime.h"

UGameSaveData::UGameSaveData()
	: SaveName(TEXT("Untitled Save"))
	, Timestamp(TEXT(""))
	, PlayerLocation(FVector::ZeroVector)
	, PlayerRotation(FRotator::ZeroRotator)
	, LevelPackagePath(TEXT(""))
	, PlayerData()
	, HotbarData()
	, InventoryData()
	, EquipmentData()
{
	// Set timestamp to current time
	FDateTime Now = FDateTime::Now();
	Timestamp = Now.ToString(TEXT("%Y.%m.%d %H:%M:%S"));
	
	// Initialize hotbar data
	HotbarData.ActiveIndex = INDEX_NONE;
}

