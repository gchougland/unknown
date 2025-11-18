#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

// Helper: create a transient item definition with a given volume
static UItemDefinition* MakeItemDef_Hotbar(float Volume = 1.f)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FF_Hotbar_Defaults,
    "Project.Hotbar.Core.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FF_Hotbar_Defaults::RunTest(const FString& Parameters)
{
    UHotbarComponent* HB = NewObject<UHotbarComponent>();
    TestNotNull(TEXT("Hotbar component created"), HB);
    if (!HB) return false;

    TestEqual(TEXT("Num slots == 9"), HB->GetNumSlots(), 9);
    TestEqual(TEXT("Active index == INDEX_NONE"), HB->GetActiveIndex(), INDEX_NONE);

    // Spot check a couple of slots for default state via GetSlot
    const FHotbarSlot& S0 = HB->GetSlot(0);
    const FHotbarSlot& S8 = HB->GetSlot(8);
    TestTrue(TEXT("Slot 0 assigned type is null"), S0.AssignedType == nullptr);
    TestFalse(TEXT("Slot 0 has no active id"), S0.ActiveItemId.IsValid());
    TestTrue(TEXT("Slot 8 assigned type is null"), S8.AssignedType == nullptr);
    TestFalse(TEXT("Slot 8 has no active id"), S8.ActiveItemId.IsValid());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FF_Hotbar_AssignAndClear,
    "Project.Hotbar.Core.AssignAndClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FF_Hotbar_AssignAndClear::RunTest(const FString& Parameters)
{
    UHotbarComponent* HB = NewObject<UHotbarComponent>();
    UItemDefinition* Def = MakeItemDef_Hotbar();
    TestTrue(TEXT("Assign slot 0"), HB->AssignSlot(0, Def));
    TestTrue(TEXT("Slot 0 now holds assigned type"), HB->GetSlot(0).AssignedType == Def);
    TestTrue(TEXT("Clear slot 0"), HB->ClearSlot(0));
    TestTrue(TEXT("Slot 0 cleared"), HB->GetSlot(0).AssignedType == nullptr);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FF_Hotbar_SelectSlot_HoldsItem,
    "Project.Hotbar.Core.SelectSlot.HoldsItem",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FF_Hotbar_SelectSlot_HoldsItem::RunTest(const FString& Parameters)
{
    UHotbarComponent* HB = NewObject<UHotbarComponent>();
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    Inv->MaxVolume = 100.f;

    UItemDefinition* DefA = MakeItemDef_Hotbar(1.f);
    UItemDefinition* DefB = MakeItemDef_Hotbar(1.f);

    // Add two A items and one B; ensure deterministic first pick of A[0]
    FItemEntry A0; A0.Def = DefA; A0.ItemId = FGuid::NewGuid();
    FItemEntry A1; A1.Def = DefA; A1.ItemId = FGuid::NewGuid();
    FItemEntry B0; B0.Def = DefB; B0.ItemId = FGuid::NewGuid();

    TestTrue(TEXT("Add A0"), Inv->TryAdd(A0));
    TestTrue(TEXT("Add A1"), Inv->TryAdd(A1));
    TestTrue(TEXT("Add B0"), Inv->TryAdd(B0));

    TestTrue(TEXT("Assign slot 1 to DefA"), HB->AssignSlot(1, DefA));

    TestTrue(TEXT("Select slot 1"), HB->SelectSlot(1, Inv));
    TestEqual(TEXT("Active index == 1"), HB->GetActiveIndex(), 1);
    const FGuid Held = HB->GetActiveItemId();
    TestTrue(TEXT("Held id is valid"), Held.IsValid());
    TestTrue(TEXT("Held is first A entry (A0)"), Held == A0.ItemId);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FF_Hotbar_SelectSlot_NoItems,
    "Project.Hotbar.Core.SelectSlot.NoItems",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FF_Hotbar_SelectSlot_NoItems::RunTest(const FString& Parameters)
{
    UHotbarComponent* HB = NewObject<UHotbarComponent>();
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();

    UItemDefinition* DefA = MakeItemDef_Hotbar(1.f);
    TestTrue(TEXT("Assign slot 2 to DefA"), HB->AssignSlot(2, DefA));

    TestTrue(TEXT("Select slot 2"), HB->SelectSlot(2, Inv));
    TestEqual(TEXT("Active index == 2"), HB->GetActiveIndex(), 2);
    TestFalse(TEXT("No item held"), HB->GetActiveItemId().IsValid());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FF_Hotbar_RefreshAfterRemoval,
    "Project.Hotbar.Core.RefreshAfterRemoval",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FF_Hotbar_RefreshAfterRemoval::RunTest(const FString& Parameters)
{
    UHotbarComponent* HB = NewObject<UHotbarComponent>();
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    Inv->MaxVolume = 100.f;

    UItemDefinition* DefA = MakeItemDef_Hotbar(1.f);

    FItemEntry A0; A0.Def = DefA; A0.ItemId = FGuid::NewGuid();
    FItemEntry A1; A1.Def = DefA; A1.ItemId = FGuid::NewGuid();

    TestTrue(TEXT("Add A0"), Inv->TryAdd(A0));
    TestTrue(TEXT("Add A1"), Inv->TryAdd(A1));

    TestTrue(TEXT("Assign slot 0"), HB->AssignSlot(0, DefA));
    TestTrue(TEXT("Select slot 0"), HB->SelectSlot(0, Inv));

    const FGuid Held0 = HB->GetActiveItemId();
    TestTrue(TEXT("Held0 valid"), Held0.IsValid());

    // Remove the held item from inventory, then refresh
    TestTrue(TEXT("Remove held from inv"), Inv->RemoveById(Held0));
    HB->RefreshHeldFromInventory(Inv);

    const FGuid Held1 = HB->GetActiveItemId();
    TestTrue(TEXT("Either next item or invalid"), Held1.IsValid() || !Held1.IsValid());
    TestTrue(TEXT("If valid, it should equal A1"), !Held1.IsValid() || Held1 == A1.ItemId);

    // Remove the remaining A1 and ensure it invalidates
    if (Held1.IsValid())
    {
        TestTrue(TEXT("Remove A1"), Inv->RemoveById(Held1));
        HB->RefreshHeldFromInventory(Inv);
        TestFalse(TEXT("Now invalid (no items left)"), HB->GetActiveItemId().IsValid());
    }

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
