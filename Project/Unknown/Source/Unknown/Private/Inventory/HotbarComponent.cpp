#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

UHotbarComponent::UHotbarComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	// Initialize with 9 slots
	Slots.SetNum(9);
}

const FHotbarSlot& UHotbarComponent::GetSlot(int32 Index) const
{
	check(Slots.IsValidIndex(Index));
	return Slots[Index];
}

bool UHotbarComponent::AssignSlot(int32 Index, UItemDefinition* Type)
{
	if (!Slots.IsValidIndex(Index))
	{
		return false;
	}
	Slots[Index].AssignedType = Type;
	// Do not change active selection here, only assignment
	OnSlotAssigned.Broadcast(Index, Type);
	return true;
}

bool UHotbarComponent::ClearSlot(int32 Index)
{
	if (!Slots.IsValidIndex(Index))
	{
		return false;
	}
	Slots[Index].AssignedType = nullptr;
	Slots[Index].ActiveItemId.Invalidate();
	if (ActiveIndex == Index)
	{
		ActiveItemId.Invalidate();
	}
	OnSlotAssigned.Broadcast(Index, nullptr);
	return true;
}

static bool InventoryHasItemId(const UInventoryComponent* Inventory, const FGuid& Id)
{
	if (!Inventory || !Id.IsValid()) return false;
	for (const FItemEntry& E : Inventory->GetEntries())
	{
		if (E.ItemId == Id)
		{
			return true;
		}
	}
	return false;
}

FGuid UHotbarComponent::PickFirstItemIdOfType(const UInventoryComponent* Inventory, const UItemDefinition* Type) const
{
	if (!Inventory || !Type)
	{
		return FGuid();
	}
	const TArray<FItemEntry>& Entries = Inventory->GetEntries();
	for (const FItemEntry& E : Entries)
	{
		if (E.Def == Type)
		{
			UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] PickFirstItemIdOfType: Found %s with ItemId %s"), 
				*GetNameSafe(Type), *E.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			return E.ItemId;
		}
	}
	UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] PickFirstItemIdOfType: No items of type %s found"), *GetNameSafe(Type));
	return FGuid();
}

bool UHotbarComponent::SelectSlot(int32 Index, UInventoryComponent* Inventory)
{
	if (!Slots.IsValidIndex(Index))
	{
		return false;
	}
	ActiveIndex = Index;

	FGuid NewId;
	UItemDefinition* Type = Slots[Index].AssignedType;
	UE_LOG(LogTemp, Display, TEXT("[HotbarComponent] SelectSlot(%d): AssignedType=%s"), Index, *GetNameSafe(Type));
	
	if (Type)
	{
		// First, check if the current ActiveItemId for this slot is still valid
		FGuid CurrentId = Slots[Index].ActiveItemId;
		UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] SelectSlot(%d): Current ActiveItemId=%s"), Index, 
			*CurrentId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
		
		if (CurrentId.IsValid() && InventoryHasItemId(Inventory, CurrentId))
		{
			// Verify it's still the correct type
			const TArray<FItemEntry>& Entries = Inventory->GetEntries();
			for (const FItemEntry& E : Entries)
			{
				if (E.ItemId == CurrentId)
				{
					if (E.Def == Type)
					{
						// Current ID is still valid and correct type - keep it
						UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] SelectSlot(%d): Keeping existing ActiveItemId (correct type)"), Index);
						NewId = CurrentId;
						break;
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[HotbarComponent] SelectSlot(%d): Current ActiveItemId exists but wrong type! Expected %s, got %s"), 
							Index, *GetNameSafe(Type), *GetNameSafe(E.Def));
					}
				}
			}
		}
		
		// If we didn't find a valid existing ID, pick the first item of this type
		if (!NewId.IsValid())
		{
			UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] SelectSlot(%d): Picking first item of type %s"), Index, *GetNameSafe(Type));
			NewId = PickFirstItemIdOfType(Inventory, Type);
			if (NewId.IsValid())
			{
				UE_LOG(LogTemp, Display, TEXT("[HotbarComponent] SelectSlot(%d): Picked ItemId=%s"), Index, 
					*NewId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			}
			else
			{
				UE_LOG(LogTemp, Display, TEXT("[HotbarComponent] SelectSlot(%d): No items of type %s found in inventory"), Index, *GetNameSafe(Type));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[HotbarComponent] SelectSlot(%d): No assigned type"), Index);
	}
	
	Slots[Index].ActiveItemId = NewId;
	ActiveItemId = NewId;

	OnActiveChanged.Broadcast(ActiveIndex, ActiveItemId);
	return true;
}

void UHotbarComponent::RefreshHeldFromInventory(UInventoryComponent* Inventory)
{
	if (!Slots.IsValidIndex(ActiveIndex))
	{
		return;
	}

	FHotbarSlot& Slot = Slots[ActiveIndex];
	FGuid NewId = Slot.ActiveItemId;

	// If current active id is no longer present, pick a new one
	if (!InventoryHasItemId(Inventory, Slot.ActiveItemId))
	{
		NewId = PickFirstItemIdOfType(Inventory, Slot.AssignedType);
	}

	if (NewId != Slot.ActiveItemId)
	{
		Slot.ActiveItemId = NewId;
		ActiveItemId = NewId;
		OnActiveChanged.Broadcast(ActiveIndex, ActiveItemId);
	}
}
