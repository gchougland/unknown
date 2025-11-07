#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Player/FirstPersonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_CrouchToggle_ChangesSpeed,
    "Project.FPS.Basic.Controller.CrouchToggleSpeed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_CrouchToggle_ChangesSpeed::RunTest(const FString& Parameters)
{
    AFirstPersonCharacter* Char = NewObject<AFirstPersonCharacter>();
    TestNotNull(TEXT("Character instance created"), Char);
    if (!Char)
    {
        return false;
    }

    UCharacterMovementComponent* Move = Char->GetCharacterMovement();
    TestNotNull(TEXT("Movement component exists"), Move);
    if (!Move)
    {
        return false;
    }

    const float Initial = Move->MaxWalkSpeed;
    Char->ToggleCrouch();
    TestEqual(TEXT("After toggle crouch -> speed is crouch speed (300)"), Move->MaxWalkSpeed, 300.f);
    Char->ToggleCrouch();
    TestEqual(TEXT("After toggle again -> speed restored to walk (600)"), Move->MaxWalkSpeed, Initial);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
