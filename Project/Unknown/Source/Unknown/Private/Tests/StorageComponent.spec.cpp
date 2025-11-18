#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"

static UItemDefinition* MakeItemDef_Storage(float Volume)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorage_VolumeAdd_Succeeds,
    "Project.Storage.Core.Volume.AddWithinCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FStorage_VolumeAdd_Succeeds::RunTest(const FString& Parameters)
{
    UStorageComponent* Box = NewObject<UStorageComponent>();
    TestNotNull(TEXT("Storage created"), Box);
    Box->MaxVolume = 20.f;

    FItemEntry A; A.Def = MakeItemDef_Storage(5.f);
    FItemEntry B; B.Def = MakeItemDef_Storage(7.f);
    FItemEntry C; C.Def = MakeItemDef_Storage(9.f);

    TestTrue(TEXT("Add A"), Box->TryAdd(A));
    TestTrue(TEXT("Add B"), Box->TryAdd(B));
    TestEqual(TEXT("UsedVolume == 12"), Box->GetUsedVolume(), 12.f);
    TestFalse(TEXT("Cannot add C (would exceed)"), Box->CanAdd(C));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorage_RemoveById_Works,
    "Project.Storage.Core.RemoveById",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FStorage_RemoveById_Works::RunTest(const FString& Parameters)
{
    UStorageComponent* Box = NewObject<UStorageComponent>();
    UItemDefinition* Def = MakeItemDef_Storage(1.f);
    FItemEntry A; A.Def = Def; A.ItemId = FGuid::NewGuid();
    FItemEntry B; B.Def = Def; B.ItemId = FGuid::NewGuid();

    TestTrue(TEXT("Add A"), Box->TryAdd(A));
    TestTrue(TEXT("Add B"), Box->TryAdd(B));
    TestEqual(TEXT("Count == 2"), Box->CountByDef(Def), 2);

    TestTrue(TEXT("Remove A"), Box->RemoveById(A.ItemId));
    TestEqual(TEXT("Count == 1"), Box->CountByDef(Def), 1);

    TestFalse(TEXT("Remove unknown fails"), Box->RemoveById(FGuid::NewGuid()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FStorage_NullDef_Rejected,
    "Project.Storage.Core.Validation.NullDefRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FStorage_NullDef_Rejected::RunTest(const FString& Parameters)
{
    UStorageComponent* Box = NewObject<UStorageComponent>();
    FItemEntry Bad;
    TestFalse(TEXT("Cannot add null-def"), Box->CanAdd(Bad));
    TestFalse(TEXT("TryAdd returns false"), Box->TryAdd(Bad));
    TestEqual(TEXT("UsedVolume remains 0"), Box->GetUsedVolume(), 0.f);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
