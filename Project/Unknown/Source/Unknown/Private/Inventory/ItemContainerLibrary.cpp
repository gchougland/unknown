#include "Inventory/ItemContainerLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"

static const FItemEntry* FindEntryById(const TArray<FItemEntry>& Entries, const FGuid& Id)
{
	const int32 Index = Entries.IndexOfByPredicate([&](const FItemEntry& E){ return E.ItemId == Id; });
	return Index != INDEX_NONE ? &Entries[Index] : nullptr;
}

float UItemContainerLibrary::GetFreeVolume(const UInventoryComponent* Container)
{
	if (!Container)
	{
		return 0.f;
	}
	return FMath::Max(0.f, Container->MaxVolume - Container->GetUsedVolume());
}

float UItemContainerLibrary::GetFreeVolumeStorage(const UStorageComponent* Container)
{
	if (!Container)
	{
		return 0.f;
	}
	return FMath::Max(0.f, Container->MaxVolume - Container->GetUsedVolume());
}

bool UItemContainerLibrary::CanTransfer_ItemId(const UInventoryComponent* Source, const UStorageComponent* Dest, const FGuid& ItemId)
{
	if (!Source || !Dest)
	{
		return false;
	}
	const FItemEntry* Entry = FindEntryById(Source->Entries, ItemId);
	return Entry && Dest->CanAdd(*Entry);
}

bool UItemContainerLibrary::Transfer_ItemId(UInventoryComponent* Source, UStorageComponent* Dest, const FGuid& ItemId)
{
	if (!Source || !Dest)
	{
		return false;
	}
	const FItemEntry* Entry = FindEntryById(Source->Entries, ItemId);
	if (!Entry)
	{
		return false;
	}
	// Try to add first to ensure atomic behavior
	if (!Dest->TryAdd(*Entry))
	{
		return false;
	}
	// Now remove from source
	const bool Removed = Source->RemoveById(ItemId);
	if (!Removed)
	{
		// Rollback: remove we just added
		const int32 NewIndex = Dest->Entries.IndexOfByPredicate([&](const FItemEntry& E){ return E.ItemId == ItemId; });
		if (NewIndex != INDEX_NONE)
		{
			const FGuid IdCopy = ItemId;
			Dest->Entries.RemoveAtSwap(NewIndex, 1, false);
			Dest->OnItemRemoved.Broadcast(IdCopy);
		}
		return false;
	}
	return true;
}

bool UItemContainerLibrary::CanTransfer_ItemId_StorageToInv(const UStorageComponent* Source, const UInventoryComponent* Dest, const FGuid& ItemId)
{
	if (!Source || !Dest)
	{
		return false;
	}
	const FItemEntry* Entry = FindEntryById(Source->Entries, ItemId);
	return Entry && Dest->CanAdd(*Entry);
}

bool UItemContainerLibrary::Transfer_ItemId_StorageToInv(UStorageComponent* Source, UInventoryComponent* Dest, const FGuid& ItemId)
{
	if (!Source || !Dest)
	{
		return false;
	}
	const FItemEntry* Entry = FindEntryById(Source->Entries, ItemId);
	if (!Entry)
	{
		return false;
	}
	if (!Dest->TryAdd(*Entry))
	{
		return false;
	}
	const bool Removed = Source->RemoveById(ItemId);
	if (!Removed)
	{
		// rollback
		const int32 NewIndex = Dest->Entries.IndexOfByPredicate([&](const FItemEntry& E){ return E.ItemId == ItemId; });
		if (NewIndex != INDEX_NONE)
		{
			const FGuid IdCopy = ItemId;
			Dest->Entries.RemoveAtSwap(NewIndex, 1, false);
			Dest->OnItemRemoved.Broadcast(IdCopy);
		}
		return false;
	}
	return true;
}

FTransferResult UItemContainerLibrary::MoveAllOfType(UInventoryComponent* Source, UStorageComponent* Dest, const UItemDefinition* Def)
{
	FTransferResult Result;
	if (!Source || !Dest || !Def)
	{
		return Result;
	}
	// Snapshot ids first to avoid mutation during iteration
	TArray<FGuid> Ids;
	Ids.Reserve(Source->Entries.Num());
	for (const FItemEntry& E : Source->Entries)
	{
		if (E.Def == Def)
		{
			Ids.Add(E.ItemId);
		}
	}
	for (const FGuid& Id : Ids)
	{
		if (Transfer_ItemId(Source, Dest, Id))
		{
			Result.MovedCount++;
		}
	}
	Result.bSuccess = Result.MovedCount > 0;
	return Result;
}
