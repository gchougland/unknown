#include "Dimensions/DimensionCartridgeData.h"

void UDimensionCartridgeData::InitializeCartridge(UDimensionDefinition* InDimensionDef, const FVector& SpawnPosition)
{
	if (!InDimensionDef)
	{
		return;
	}

	DimensionDef = InDimensionDef;
	CartridgeId = FGuid::NewGuid();

	// Create instance data if it doesn't exist
	if (!InstanceData)
	{
		InstanceData = NewObject<UDimensionInstanceData>(this);
	}

	// Initialize instance data
	InstanceData->InitializeInstance(CartridgeId, SpawnPosition, InDimensionDef->DefaultStability);
}
