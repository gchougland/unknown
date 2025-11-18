#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemContainerLibrary.h"
#include "Inventory/ItemDefinition.h"

static UItemDefinition* MakeItemDef_Transfer(float Volume)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransfer_InventoryToStorage_Succeeds,
    "Project.Transfer.Core.InventoryToStorage.Succeeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FTransfer_InventoryToStorage_Succeeds::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    UStorageComponent* Box = NewObject<UStorageComponent>();
    Inv->MaxVolume = 10.f; Box->MaxVolume = 10.f;

    UItemDefinition* Def = MakeItemDef_Transfer(3.f);
    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add A to inventory"), Inv->TryAdd(A));

    TestTrue(TEXT("Can transfer by id"), UItemContainerLibrary::CanTransfer_ItemId(Inv, Box, A.ItemId));
    TestTrue(TEXT("Transfer returns true"), UItemContainerLibrary::Transfer_ItemId(Inv, Box, A.ItemId));

    TestEqual(TEXT("Inv count now 0"), Inv->CountByDef(Def), 0);
    TestEqual(TEXT("Box count now 1"), Box->CountByDef(Def), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransfer_InventoryToStorage_CapacityBlocks,
    "Project.Transfer.Core.InventoryToStorage.CapacityBlocks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FTransfer_InventoryToStorage_CapacityBlocks::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    UStorageComponent* Box = NewObject<UStorageComponent>();
    Box->MaxVolume = 2.f;

    UItemDefinition* Def = MakeItemDef_Transfer(3.f);
    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add A to inventory"), Inv->TryAdd(A));

    TestFalse(TEXT("Cannot transfer due to capacity"), UItemContainerLibrary::CanTransfer_ItemId(Inv, Box, A.ItemId));
    TestFalse(TEXT("Transfer returns false"), UItemContainerLibrary::Transfer_ItemId(Inv, Box, A.ItemId));

    TestEqual(TEXT("Inv still has item"), Inv->CountByDef(Def), 1);
    TestEqual(TEXT("Box still empty"), Box->CountByDef(Def), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransfer_MoveAllOfType_PartialByCapacity,
    "Project.Transfer.Core.MoveAllOfType.PartialByCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FTransfer_MoveAllOfType_PartialByCapacity::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    UStorageComponent* Box = NewObject<UStorageComponent>();
    Box->MaxVolume = 5.f; // can hold only one of volume 3

    UItemDefinition* Def = MakeItemDef_Transfer(3.f);

    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    FItemEntry B; B.Def = Def; B.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add A"), Inv->TryAdd(A));
    TestTrue(TEXT("Add B"), Inv->TryAdd(B));

    const FTransferResult Res = UItemContainerLibrary::MoveAllOfType(Inv, Box, Def);
    TestTrue(TEXT("Result success when any moved"), Res.bSuccess);
    TestEqual(TEXT("Moved only one due to capacity"), Res.MovedCount, 1);

    TestEqual(TEXT("Inv now has 1 left"), Inv->CountByDef(Def), 1);
    TestEqual(TEXT("Box has 1"), Box->CountByDef(Def), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTransfer_StorageToInventory_Succeeds,
    "Project.Transfer.Core.StorageToInventory.Succeeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FTransfer_StorageToInventory_Succeeds::RunTest(const FString& Parameters)
{
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    UStorageComponent* Box = NewObject<UStorageComponent>();
    Inv->MaxVolume = 10.f; Box->MaxVolume = 10.f;

    UItemDefinition* Def = MakeItemDef_Transfer(2.f);
    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add A to storage"), Box->TryAdd(A));

    TestTrue(TEXT("Can transfer"), UItemContainerLibrary::CanTransfer_ItemId_StorageToInv(Box, Inv, A.ItemId));
    TestTrue(TEXT("Transfer returns true"), UItemContainerLibrary::Transfer_ItemId_StorageToInv(Box, Inv, A.ItemId));

    TestEqual(TEXT("Box empty"), Box->CountByDef(Def), 0);
    TestEqual(TEXT("Inv has 1"), Inv->CountByDef(Def), 1);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
