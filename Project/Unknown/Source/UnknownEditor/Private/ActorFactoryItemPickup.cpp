#include "ActorFactoryItemPickup.h"

#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "AssetRegistry/AssetData.h"
#include "Engine/World.h"
#include "Editor.h"
#include "GameFramework/Actor.h"

UActorFactoryItemPickup::UActorFactoryItemPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayName = NSLOCTEXT("ActorFactory", "ActorFactoryItemPickup", "Item Pickup");
	NewActorClass = AItemPickup::StaticClass();
	bShowInEditorQuickMenu = true;
}

bool UActorFactoryItemPickup::CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg)
{
	if (!AssetData.IsValid())
	{
		OutErrorMsg = NSLOCTEXT("ActorFactory", "NoAsset", "A valid ItemDefinition must be specified.");
		return false;
	}

	// Accept only UItemDefinition assets
	if (AssetData.AssetClassPath == UItemDefinition::StaticClass()->GetClassPathName())
	{
		return true;
	}

	OutErrorMsg = NSLOCTEXT("ActorFactory", "WrongClass", "Asset must be an ItemDefinition.");
	return false;
}

UObject* UActorFactoryItemPickup::GetAssetFromActorInstance(AActor* ActorInstance)
{
	if (AItemPickup* Pickup = Cast<AItemPickup>(ActorInstance))
	{
		return Pickup->GetItemDef();
	}
	return nullptr;
}

AActor* UActorFactoryItemPickup::SpawnActor(UObject* Asset, ULevel* InLevel, const FTransform& Transform, const FActorSpawnParameters& SpawnParams)
{
    UItemDefinition* ItemDef = Cast<UItemDefinition>(Asset);
    if (!ItemDef)
    {
        return nullptr;
    }

    UWorld* World = InLevel ? InLevel->OwningWorld : nullptr;
    if (!World)
    {
        return nullptr;
    }

    // Apply the item's DefaultDropTransform relative to the provided placement transform
    // so authored offsets and scale are respected during drag/drop placement.
    const FTransform FinalTransform = ItemDef ? (ItemDef->DefaultDropTransform * Transform) : Transform;

    AItemPickup* NewPickup = World->SpawnActorDeferred<AItemPickup>(AItemPickup::StaticClass(), FinalTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
    if (!NewPickup)
    {
        return nullptr;
    }

    NewPickup->SetItemDef(ItemDef);
    // Finish spawning so physics/collision initialize correctly
    NewPickup->FinishSpawning(FinalTransform);

    return NewPickup;
}
