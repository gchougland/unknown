#include "Inventory/EquipmentComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemEquipEffect.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemPickup.h"
#include "UI/MessageLogSubsystem.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

UEquipmentComponent::UEquipmentComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

bool UEquipmentComponent::GetEquipped(EEquipmentSlot Slot, FItemEntry& OutEntry) const
{
    if (const FItemEntry* Found = Equipped.Find(Slot))
    {
        OutEntry = *Found;
        return true;
    }
    return false;
}

int32 UEquipmentComponent::FindEntryIndexById(const FGuid& ItemId) const
{
    if (!Inventory)
    {
        return INDEX_NONE;
    }
    const TArray<FItemEntry>& Arr = Inventory->GetEntries();
    for (int32 i = 0; i < Arr.Num(); ++i)
    {
        if (Arr[i].ItemId == ItemId)
        {
            return i;
        }
    }
    return INDEX_NONE;
}

bool UEquipmentComponent::ResolveTargetSlot(const UItemDefinition* Def, EEquipmentSlot& OutSlot) const
{
    if (!Def)
    {
        return false;
    }
    if (Def->EquipEffects.Num() > 0 && Def->EquipEffects[0])
    {
        OutSlot = Def->EquipEffects[0]->GetTargetSlot();
        return true;
    }
    return false;
}

void UEquipmentComponent::ApplyEffects(const UItemDefinition* Def) const
{
    if (!Def || !Inventory)
    {
        return;
    }
    for (UItemEquipEffect* Eff : Def->EquipEffects)
    {
        if (Eff)
        {
            Eff->ApplyEffect(GetOwner(), Inventory);
        }
    }
}

bool UEquipmentComponent::CanRemoveEffects(const UItemDefinition* Def, FText& OutError) const
{
    if (!Def || !Inventory)
    {
        return true;
    }
    for (UItemEquipEffect* Eff : Def->EquipEffects)
    {
        if (Eff)
        {
            if (!Eff->CanRemoveEffect(GetOwner(), Inventory, OutError))
            {
                return false;
            }
        }
    }
    return true;
}

void UEquipmentComponent::RemoveEffects(const UItemDefinition* Def) const
{
    if (!Def || !Inventory)
    {
        return;
    }
    for (UItemEquipEffect* Eff : Def->EquipEffects)
    {
        if (Eff)
        {
            Eff->RemoveEffect(GetOwner(), Inventory);
        }
    }
}

bool UEquipmentComponent::EquipFromInventory(const FGuid& ItemId, FText& OutError)
{
    if (!Inventory)
    {
        OutError = FText::FromString(TEXT("No inventory assigned."));
        return false;
    }
    const int32 Index = FindEntryIndexById(ItemId);
    if (Index == INDEX_NONE)
    {
        OutError = FText::FromString(TEXT("Item not found in inventory."));
        return false;
    }
    const FItemEntry Entry = Inventory->GetEntries()[Index];
    if (!Entry.Def || !Entry.Def->bEquippable)
    {
        OutError = FText::FromString(TEXT("Item is not equippable."));
        return false;
    }
    EEquipmentSlot Slot;
    if (!ResolveTargetSlot(Entry.Def, Slot))
    {
        OutError = FText::FromString(TEXT("Unable to resolve target slot."));
        return false;
    }

    // If slot has an item, try to return it to inventory first, else drop to world
    if (FItemEntry* Existing = Equipped.Find(Slot))
    {
        const FItemEntry ToReturn = *Existing;
        Equipped.Remove(Slot);
        // Try add back
        if (!Inventory->TryAdd(ToReturn))
        {
            // Drop to world instead
            DropToWorld(ToReturn);
            if (UMessageLogSubsystem* Msg = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
            {
                const FText Name = (ToReturn.Def && !ToReturn.Def->DisplayName.IsEmpty()) ? ToReturn.Def->DisplayName : FText::FromString(GetNameSafe(ToReturn.Def));
                Msg->PushMessage(FText::Format(FText::FromString(TEXT("Dropped {0}.")), Name));
            }
        }
        OnItemUnequipped.Broadcast(Slot, ToReturn);
        // Removing effects from the previously equipped item
        if (ToReturn.Def)
        {
            RemoveEffects(ToReturn.Def);
        }
    }

    // Remove the chosen item from inventory and equip
    if (!Inventory->RemoveById(ItemId))
    {
        OutError = FText::FromString(TEXT("Failed to remove item from inventory."));
        return false;
    }
    Equipped.Add(Slot, Entry);
    OnItemEquipped.Broadcast(Slot, Entry);
    ApplyEffects(Entry.Def);
    return true;
}

bool UEquipmentComponent::TryUnequipToInventory(EEquipmentSlot Slot, FText& OutError)
{
    FItemEntry Existing;
    if (!GetEquipped(Slot, Existing))
    {
        OutError = FText::FromString(TEXT("Nothing equipped in slot."));
        return false;
    }
    // Check effect removal rules
    if (Existing.Def && !CanRemoveEffects(Existing.Def, OutError))
    {
        // Block unequip; effects decided the reason
        if (!OutError.IsEmpty())
        {
            if (UMessageLogSubsystem* Msg = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
            {
                Msg->PushMessage(OutError, 4.0f);
            }
        }
        return false;
    }
    // Remove effects now that approved
    if (Existing.Def)
    {
        RemoveEffects(Existing.Def);
    }
    Equipped.Remove(Slot);
    OnItemUnequipped.Broadcast(Slot, Existing);

    // Try add back to inventory else drop
    if (!Inventory || !Inventory->TryAdd(Existing))
    {
        DropToWorld(Existing);
        if (UMessageLogSubsystem* Msg = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
        {
            const FText Name = (Existing.Def && !Existing.Def->DisplayName.IsEmpty()) ? Existing.Def->DisplayName : FText::FromString(GetNameSafe(Existing.Def));
            Msg->PushMessage(FText::Format(FText::FromString(TEXT("Dropped {0}.")), Name));
        }
    }
    return true;
}

void UEquipmentComponent::DropToWorld(const FItemEntry& Entry) const
{
    if (!GetWorld() || !Entry.IsValid())
    {
        return;
    }
    const AActor* OwnerActor = GetOwner();
    FActorSpawnParameters Params;
    // Equipment drops due to inventory full should always materialize, nudging if needed
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    // Use unified drop helper to preserve metadata
    AItemPickup* Pickup = AItemPickup::DropItemToWorld(GetWorld(), OwnerActor, Entry, Params);
    if (!Pickup)
    {
        // If for some reason spawning failed, attempt a best-effort always-spawn as a fallback
        const FTransform Fallback = AItemPickup::BuildDropTransform(OwnerActor, Entry.Def);
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Pickup = GetWorld()->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), Fallback, Params);
        if (Pickup)
        {
            // Set the full item entry to preserve metadata
            Pickup->SetItemEntry(Entry);
            // Configure physics for dropped items
            if (UStaticMeshComponent* ItemMesh = Pickup->Mesh)
            {
                ItemMesh->SetSimulatePhysics(true);
                ItemMesh->SetEnableGravity(true);
                ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
                ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
                ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
                ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
            }
        }
    }
}
