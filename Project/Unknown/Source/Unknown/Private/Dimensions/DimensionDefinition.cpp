#include "Dimensions/DimensionDefinition.h"

UDimensionDefinition::UDimensionDefinition()
{
	// Generate a new GUID if one doesn't exist
	if (!Guid.IsValid())
	{
		Guid = FGuid::NewGuid();
	}
}
