#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/FirstPersonCharacter.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

static UItemDefinition* MakeItemDef_HotbarInput(float Volume)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHotbar_Input_SelectViaCharacter,
    "Project.Hotbar.Input.SelectViaCharacter",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FHotbar_Input_SelectViaCharacter::RunTest(const FString& Parameters)
{
    // Create character with components
    AFirstPersonCharacter* Char = NewObject<AFirstPersonCharacter>();
    TestNotNull(TEXT("Character created"), Char);
    if (!Char) return false;

    UHotbarComponent* Hotbar = Char->GetHotbar();
    UInventoryComponent* Inv = Char->GetInventory();
    TestNotNull(TEXT("Hotbar exists"), Hotbar);
    TestNotNull(TEXT("Inventory exists"), Inv);

    // Prepare two item types and entries
    UItemDefinition* Apple = MakeItemDef_HotbarInput(1.f);
    UItemDefinition* Berry = MakeItemDef_HotbarInput(1.f);
    FItemEntry A; A.Def = Apple; A.ItemId = FGuid::NewGuid();
    FItemEntry B; B.Def = Berry; B.ItemId = FGuid::NewGuid();

    // Add to inventory
    TestTrue(TEXT("Add A"), Inv->TryAdd(A));
    TestTrue(TEXT("Add B"), Inv->TryAdd(B));

    // Assign types to slot 0 and 1
    TestTrue(TEXT("Assign slot 0 -> Apple"), Hotbar->AssignSlot(0, Apple));
    TestTrue(TEXT("Assign slot 1 -> Berry"), Hotbar->AssignSlot(1, Berry));

    // Select slot 0 then 1 via character API
    TestTrue(TEXT("SelectHotbarSlot(0) returns true"), Char->SelectHotbarSlot(0));
    TestEqual(TEXT("Active index == 0"), Hotbar->GetActiveIndex(), 0);
    TestTrue(TEXT("Active item id valid"), Hotbar->GetActiveItemId().IsValid());

    TestTrue(TEXT("SelectHotbarSlot(1) returns true"), Char->SelectHotbarSlot(1));
    TestEqual(TEXT("Active index == 1"), Hotbar->GetActiveIndex(), 1);

    // Remove the held item and refresh
    const FGuid Held = Hotbar->GetActiveItemId();
    TestTrue(TEXT("Remove held from inventory"), Inv->RemoveById(Held));
    Hotbar->RefreshHeldFromInventory(Inv);
    // After removal, should still be index 1 but ActiveItemId may be invalid if no more Berry
    TestEqual(TEXT("Active index stays 1"), Hotbar->GetActiveIndex(), 1);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
