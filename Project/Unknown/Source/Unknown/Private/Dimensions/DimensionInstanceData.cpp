#include "Dimensions/DimensionInstanceData.h"

void UDimensionInstanceData::InitializeInstance(const FGuid& InCartridgeId, const FVector& InWorldPosition, float InStability)
{
	InstanceId = FGuid::NewGuid();
	CartridgeId = InCartridgeId;
	WorldPosition = InWorldPosition;
	Stability = InStability;
	bIsLoaded = false;
}
