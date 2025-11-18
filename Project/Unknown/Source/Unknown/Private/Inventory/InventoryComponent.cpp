#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UInventoryComponent::GetUsedVolume() const
{
	float Total = 0.f;
	for (const FItemEntry& E : Entries)
	{
		if (E.Def)
		{
			Total += FMath::Max(0.f, E.Def->VolumePerUnit);
		}
	}
	return Total;
}

bool UInventoryComponent::CanAdd(const FItemEntry& Entry) const
{
	if (!Entry.Def)
	{
		return false;
	}
	const float Vol = FMath::Max(0.f, Entry.Def->VolumePerUnit);
	return GetUsedVolume() + Vol <= MaxVolume + KINDA_SMALL_NUMBER;
}

bool UInventoryComponent::TryAdd(const FItemEntry& Entry)
{
	if (!CanAdd(Entry))
	{
		return false;
	}
	FItemEntry Copy = Entry;
	if (!Copy.ItemId.IsValid())
	{
		Copy.ItemId = FGuid::NewGuid();
	}
	Entries.Add(Copy);
	OnItemAdded.Broadcast(Copy);
	return true;
}

bool UInventoryComponent::RemoveById(const FGuid& ItemId)
{
	for (int32 i = 0; i < Entries.Num(); ++i)
	{
		if (Entries[i].ItemId == ItemId)
		{
			Entries.RemoveAtSwap(i);
			OnItemRemoved.Broadcast(ItemId);
			return true;
		}
	}
	return false;
}

int32 UInventoryComponent::CountByDef(const UItemDefinition* Def) const
{
	if (!Def)
	{
		return 0;
	}
	int32 Count = 0;
	for (const FItemEntry& E : Entries)
	{
		if (E.Def == Def)
		{
			++Count;
		}
	}
	return Count;
}
