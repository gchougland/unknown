#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

// Helper: create a transient item definition with a given volume
static UItemDefinition* MakeItemDef(float Volume)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventory_VolumeAdd_Succeeds,
    "Project.Inventory.Core.Volume.AddWithinCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FInventory_VolumeAdd_Succeeds::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    TestNotNull(TEXT("Inventory created"), Inv);
    Inv->MaxVolume = 10.f;

    FItemEntry A; A.Def = MakeItemDef(4.f);
    FItemEntry B; B.Def = MakeItemDef(6.f);

    TestTrue(TEXT("Can add A"), Inv->CanAdd(A));
    TestTrue(TEXT("Add A"), Inv->TryAdd(A));
    TestTrue(TEXT("Can add B"), Inv->CanAdd(B));
    TestTrue(TEXT("Add B"), Inv->TryAdd(B));
    TestEqual(TEXT("UsedVolume sum == 10"), Inv->GetUsedVolume(), 10.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventory_VolumeAdd_FailsWhenExceeds,
    "Project.Inventory.Core.Volume.AddExceedsCapacityFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FInventory_VolumeAdd_FailsWhenExceeds::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    Inv->MaxVolume = 5.f;

    FItemEntry A; A.Def = MakeItemDef(4.f);
    FItemEntry B; B.Def = MakeItemDef(2.f);

    TestTrue(TEXT("Add A"), Inv->TryAdd(A));
    TestFalse(TEXT("Cannot add B (exceeds)"), Inv->CanAdd(B));
    TestFalse(TEXT("TryAdd returns false"), Inv->TryAdd(B));
    TestEqual(TEXT("UsedVolume remains 4"), Inv->GetUsedVolume(), 4.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventory_RemoveById_Works,
    "Project.Inventory.Core.RemoveById",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FInventory_RemoveById_Works::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    Inv->MaxVolume = 100.f;

    UItemDefinition* Def = MakeItemDef(1.f);

    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    FItemEntry B; B.Def = Def; B.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add A"), Inv->TryAdd(A));
    TestTrue(TEXT("Add B"), Inv->TryAdd(B));

    TestEqual(TEXT("Count for Def is 2"), Inv->CountByDef(Def), 2);

    const bool Removed = Inv->RemoveById(A.ItemId);
    TestTrue(TEXT("Removed A by id"), Removed);
    TestEqual(TEXT("Count now 1"), Inv->CountByDef(Def), 1);

    // Removing nonexistent id fails
    TestFalse(TEXT("Remove unknown id fails"), Inv->RemoveById(FGuid::NewGuid()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInventory_NullDefinition_Rejected,
    "Project.Inventory.Core.Validation.NullDefRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FInventory_NullDefinition_Rejected::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    FItemEntry Bad; // no Def set
    TestFalse(TEXT("Cannot add null-def entry"), Inv->CanAdd(Bad));
    TestFalse(TEXT("TryAdd returns false"), Inv->TryAdd(Bad));
    TestEqual(TEXT("UsedVolume remains 0"), Inv->GetUsedVolume(), 0.f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
