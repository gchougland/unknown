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
			return E.ItemId;
		}
	}
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
	if (Type)
	{
		NewId = PickFirstItemIdOfType(Inventory, Type);
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
