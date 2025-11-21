#include "Inventory/FoodItemData.h"
#include "Inventory/ItemDefinition.h"

UStaticMesh* UFoodItemData::GetMeshForUsesRemaining(int32 UsesRemaining) const
{
	if (UsesRemainingMeshVariants.Num() == 0)
	{
		return nullptr;
	}

	// Clamp uses remaining to valid range
	const int32 ClampedUses = FMath::Clamp(UsesRemaining, 1, MaxUses);
	
	// Calculate index: 0 = max uses, N-1 = 1 use remaining
	// If we have MaxUses=3 and 3 variants: [0]=full, [1]=2 uses, [2]=1 use
	// If UsesRemaining=3, index should be 0 (full)
	// If UsesRemaining=2, index should be 1
	// If UsesRemaining=1, index should be 2
	const int32 Index = MaxUses - ClampedUses;
	
	if (Index >= 0 && Index < UsesRemainingMeshVariants.Num())
	{
		return UsesRemainingMeshVariants[Index];
	}
	
	return nullptr;
}

