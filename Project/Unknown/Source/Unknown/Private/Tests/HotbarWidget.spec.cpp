#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "UI/HotbarWidget.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"

static UItemDefinition* MakeItemDef_HotbarWidget(float Volume)
{
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage());
    Def->VolumePerUnit = Volume;
    return Def;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FUI_HotbarWidget_BindsAndUpdates,
    "Project.UI.HotbarWidget.BindsAndUpdates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FUI_HotbarWidget_BindsAndUpdates::RunTest(const FString& Parameters)
{
    UHotbarComponent* Hotbar = NewObject<UHotbarComponent>();
    UInventoryComponent* Inv = NewObject<UInventoryComponent>();
    TestNotNull(TEXT("Hotbar exists"), Hotbar);

    // Create widget
    UHotbarWidget* Widget = NewObject<UHotbarWidget>();
    TestNotNull(TEXT("Widget created"), Widget);

    // Bind
    Widget->SetHotbar(Hotbar);
    TestEqual(TEXT("Initial active index INDEX_NONE"), Widget->GetActiveIndex(), INDEX_NONE);

    // Assign and select
    UItemDefinition* Type = MakeItemDef_HotbarWidget(1.f);
    FItemEntry E; E.Def = Type; E.ItemId = FGuid::NewGuid();
    TestTrue(TEXT("Add to inventory"), Inv->TryAdd(E));
    TestTrue(TEXT("Assign slot 0"), Hotbar->AssignSlot(0, Type));
    TestTrue(TEXT("Select slot 0"), Hotbar->SelectSlot(0, Inv));

    // Widget should mirror
    TestEqual(TEXT("Widget active index == 0"), Widget->GetActiveIndex(), 0);
    TestTrue(TEXT("Widget active item id valid"), Widget->GetActiveItemId().IsValid());
    TestTrue(TEXT("AssignedType mirrored"), Widget->GetAssignedType(0) == Type);

    // Clear slot and verify widget updates
    TestTrue(TEXT("Clear slot 0"), Hotbar->ClearSlot(0));
    TestTrue(TEXT("Assigned type null after clear"), Widget->GetAssignedType(0) == nullptr);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
