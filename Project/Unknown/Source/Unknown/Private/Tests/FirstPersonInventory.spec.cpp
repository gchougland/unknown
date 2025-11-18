#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/FirstPersonCharacter.h"
#include "Inventory/InventoryComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_Character_HasInventory,
    "Project.Inventory.Core.CharacterHasInventory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_Character_HasInventory::RunTest(const FString& Parameters)
{
    const AFirstPersonCharacter* CDO = GetDefault<AFirstPersonCharacter>();
    TestNotNull(TEXT("FirstPersonCharacter CDO exists"), CDO);
    const UInventoryComponent* Inv = CDO ? CDO->FindComponentByClass<UInventoryComponent>() : nullptr;
    TestNotNull(TEXT("Inventory component exists on character"), Inv);
    if (Inv)
    {
        TestTrue(TEXT("Default MaxVolume is positive"), Inv->MaxVolume > 0.f);
    }
    return true;
}
#endif // WITH_DEV_AUTOMATION_TESTS
