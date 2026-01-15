#include "Dimensions/DimensionScanningSubsystem.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"
#include "GameplayTagContainer.h"
#include "Player/FirstPersonCharacter.h"
#include "GameplayTagsManager.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UObject/UObjectGlobals.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"

void UDimensionScanningSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	AvailableDimensions.Empty();
	
	// Load dimension definitions on initialization
	LoadDimensionDefinitions();
}

void UDimensionScanningSubsystem::LoadDimensionDefinitions()
{
	AvailableDimensions.Empty();

	// Use asset registry to find all dimension definition assets
	FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();

	// Search for all UDimensionDefinition assets
	TArray<FAssetData> AssetDataList;
	FARFilter Filter;
	Filter.ClassPaths.Add(UDimensionDefinition::StaticClass()->GetClassPathName());
	Filter.bRecursivePaths = true;
	
	AssetRegistry.GetAssets(Filter, AssetDataList);

	// Load all found dimension definitions
	for (const FAssetData& AssetData : AssetDataList)
	{
		if (UDimensionDefinition* DimensionDef = Cast<UDimensionDefinition>(AssetData.GetAsset()))
		{
			AvailableDimensions.Add(DimensionDef);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[DimensionScanning] Loaded %d dimension definitions"), AvailableDimensions.Num());
}

TArray<UDimensionDefinition*> UDimensionScanningSubsystem::GetAvailableDimensions() const
{
	TArray<UDimensionDefinition*> Result;
	for (const TObjectPtr<UDimensionDefinition>& Def : AvailableDimensions)
	{
		if (Def)
		{
			Result.Add(Def);
		}
	}
	return Result;
}

TArray<UDimensionDefinition*> UDimensionScanningSubsystem::GetEligibleDimensions() const
{
	TArray<UDimensionDefinition*> Eligible;
	
	for (const TObjectPtr<UDimensionDefinition>& Def : AvailableDimensions)
	{
		if (!Def)
		{
			continue;
		}

		if (CheckPlayerRequirements(Def))
		{
			Eligible.Add(Def);
		}
	}

	return Eligible;
}

bool UDimensionScanningSubsystem::CheckPlayerRequirements(UDimensionDefinition* DimensionDef) const
{
	if (!DimensionDef)
	{
		return false;
	}

	// If no requirements, dimension is always eligible
	if (DimensionDef->RequiredTags.IsEmpty())
	{
		return true;
	}

	// Get player character to check tags
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return false;
	}

	// Check if player has the required tags
	// This is a simplified check - you may need to extend this based on your tag system
	// For now, we'll assume the player character or a component has a way to check tags
	// You may need to add a component or interface to check player tags
	
	// TODO: Implement actual tag checking based on your player tag system
	// For now, return true if no requirements (placeholder)
	return true;
}

UDimensionDefinition* UDimensionScanningSubsystem::ScanForDimension()
{
	// Get eligible dimensions
	TArray<UDimensionDefinition*> EligibleDimensions = GetEligibleDimensions();

	if (EligibleDimensions.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[DimensionScanning] No eligible dimensions found"));
		return nullptr;
	}

	// Perform weighted random selection
	return SelectDimensionByWeight(EligibleDimensions);
}

UDimensionDefinition* UDimensionScanningSubsystem::SelectDimensionByWeight(const TArray<UDimensionDefinition*>& EligibleDimensions) const
{
	if (EligibleDimensions.Num() == 0)
	{
		return nullptr;
	}

	// Calculate total weight
	float TotalWeight = 0.0f;
	for (UDimensionDefinition* Def : EligibleDimensions)
	{
		if (Def)
		{
			TotalWeight += FMath::Max(0.0f, Def->RarityWeight);
		}
	}

	if (TotalWeight <= 0.0f)
	{
		// If all weights are zero or negative, return first dimension
		return EligibleDimensions[0];
	}

	// Generate random value
	float RandomValue = FMath::RandRange(0.0f, TotalWeight);

	// Select dimension based on weight
	float CurrentWeight = 0.0f;
	for (UDimensionDefinition* Def : EligibleDimensions)
	{
		if (!Def)
		{
			continue;
		}

		CurrentWeight += FMath::Max(0.0f, Def->RarityWeight);
		if (RandomValue <= CurrentWeight)
		{
			return Def;
		}
	}

	// Fallback to last dimension (shouldn't happen)
	return EligibleDimensions.Last();
}
