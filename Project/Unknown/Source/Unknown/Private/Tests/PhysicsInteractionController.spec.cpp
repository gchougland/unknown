#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

#include "Player/FirstPersonCharacter.h"
#include "Player/FirstPersonPlayerController.h"
#include "Components/PhysicsInteractionComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_CharacterHasComponent,
    "Project.Interaction.Physics.CharacterHasComponent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_CharacterHasComponent::RunTest(const FString&)
{
    const AFirstPersonCharacter* CDO = GetDefault<AFirstPersonCharacter>();
    TestNotNull(TEXT("Character CDO"), CDO);
    if (!CDO) return false;

    const UPhysicsInteractionComponent* Pic = CDO->FindComponentByClass<UPhysicsInteractionComponent>();
    TestNotNull(TEXT("PhysicsInteractionComponent present"), Pic);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_ControllerActions,
    "Project.Interaction.Physics.ControllerActionsExist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_ControllerActions::RunTest(const FString&)
{
    const AFirstPersonPlayerController* CDO = GetDefault<AFirstPersonPlayerController>();
    TestNotNull(TEXT("PC CDO"), CDO);
    if (!CDO) return false;

    TestNotNull(TEXT("Interact action exists"), CDO->GetInteractAction());
    TestNotNull(TEXT("RotateHeld action exists"), CDO->GetRotateHeldAction());
    TestNotNull(TEXT("Throw action exists"), CDO->GetThrowAction());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
