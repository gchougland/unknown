#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"

UStorageComponent::UStorageComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

float UStorageComponent::GetUsedVolume() const
{
	float Total = 0.f;
	for (const FItemEntry& E : Entries)
	{
		if (E.Def)
		{
			Total += E.Def->VolumePerUnit;
		}
	}
	return Total;
}

bool UStorageComponent::CanAdd(const FItemEntry& Entry) const
{
	if (!Entry.Def)
	{
		return false;
	}
	const float NewUsed = GetUsedVolume() + Entry.Def->VolumePerUnit;
	return NewUsed <= MaxVolume + KINDA_SMALL_NUMBER; // allow tiny epsilon
}

bool UStorageComponent::TryAdd(const FItemEntry& Entry)
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

bool UStorageComponent::RemoveById(const FGuid& ItemId)
{
	const int32 Index = Entries.IndexOfByPredicate([&](const FItemEntry& E){ return E.ItemId == ItemId; });
	if (Index != INDEX_NONE)
	{
		Entries.RemoveAtSwap(Index, 1, false);
		OnItemRemoved.Broadcast(ItemId);
		return true;
	}
	return false;
}

int32 UStorageComponent::CountByDef(const UItemDefinition* Def) const
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
