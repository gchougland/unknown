#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

// Intentionally include headers that do not exist yet to drive TDD
#include "Player/FirstPersonCharacter.h"
#include "Player/FirstPersonPlayerController.h"
#include "Player/BasicFpsGameMode.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_ClassesExist_Test,
    "Project.FPS.Basic.Controller.ClassesExist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_ClassesExist_Test::RunTest(const FString& Parameters)
{
    TestNotNull(TEXT("AFirstPersonCharacter class should exist"), AFirstPersonCharacter::StaticClass());
    TestNotNull(TEXT("AFirstPersonPlayerController class should exist"), AFirstPersonPlayerController::StaticClass());
    TestNotNull(TEXT("ABasicFpsGameMode class should exist"), ABasicFpsGameMode::StaticClass());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_DefaultSpeeds_Test,
    "Project.FPS.Basic.Controller.DefaultSpeeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_DefaultSpeeds_Test::RunTest(const FString& Parameters)
{
    const AFirstPersonCharacter* CDO = GetDefault<AFirstPersonCharacter>();
    TestNotNull(TEXT("CDO exists"), CDO);
    if (!CDO)
    {
        return false;
    }

    const UCharacterMovementComponent* Move = CDO->GetCharacterMovement();
    TestNotNull(TEXT("Movement component exists"), Move);

    if (Move)
    {
        // Defaults selected per acceptance criteria
        TestEqual(TEXT("Walk speed is 600 uu/s"), Move->MaxWalkSpeed, 600.f);
        TestEqual(TEXT("Crouch speed is 300 uu/s"), Move->MaxWalkSpeedCrouched, 300.f);
    }

    // Custom sprint speed accessor on our character
    TestEqual(TEXT("Sprint speed is 900 uu/s"), CDO->GetSprintSpeed(), 900.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_SprintBehavior_Test,
    "Project.FPS.Basic.Controller.SprintBehavior",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_SprintBehavior_Test::RunTest(const FString& Parameters)
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

    const float Walk = Move->MaxWalkSpeed;
    Char->StartSprint();
    TestEqual(TEXT("Sprinting sets MaxWalkSpeed to SprintSpeed"), Move->MaxWalkSpeed, Char->GetSprintSpeed());
    Char->StopSprint();
    TestEqual(TEXT("Stopping sprint restores walk speed"), Move->MaxWalkSpeed, Walk);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_CameraExists_Test,
    "Project.FPS.Basic.Controller.CameraExists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_CameraExists_Test::RunTest(const FString& Parameters)
{
    const AFirstPersonCharacter* CDO = GetDefault<AFirstPersonCharacter>();
    TestNotNull(TEXT("CDO exists"), CDO);
    if (!CDO)
    {
        return false;
    }

    const UCameraComponent* Cam = CDO->FindComponentByClass<UCameraComponent>();
    TestNotNull(TEXT("Camera component present on character"), Cam);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_ControllerAssetsExist_Test,
    "Project.FPS.Basic.Controller.ControllerAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_ControllerAssetsExist_Test::RunTest(const FString& Parameters)
{
    const AFirstPersonPlayerController* CDO = GetDefault<AFirstPersonPlayerController>();
    TestNotNull(TEXT("PC CDO exists"), CDO);
    if (!CDO)
    {
        return false;
    }

    TestNotNull(TEXT("Input Mapping Context exists"), CDO->GetDefaultMappingContext());
    TestNotNull(TEXT("Move action exists"), CDO->GetMoveAction());
    TestNotNull(TEXT("Look action exists"), CDO->GetLookAction());
    TestNotNull(TEXT("Jump action exists"), CDO->GetJumpAction());
    TestNotNull(TEXT("Crouch action exists"), CDO->GetCrouchAction());
    TestNotNull(TEXT("Sprint action exists"), CDO->GetSprintAction());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_PitchClamp_Test,
    "Project.FPS.Basic.Controller.PitchClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_PitchClamp_Test::RunTest(const FString& Parameters)
{
    const float ClampedHigh = AFirstPersonPlayerController::ClampPitchDegrees(120.f);
    const float ClampedLow = AFirstPersonPlayerController::ClampPitchDegrees(-120.f);
    TestEqual(TEXT("Clamp +120 -> +89"), ClampedHigh, 89.f);
    TestEqual(TEXT("Clamp -120 -> -89"), ClampedLow, -89.f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFps_GameModeDefaults_Test,
    "Project.FPS.Basic.Controller.GameModeDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FFps_GameModeDefaults_Test::RunTest(const FString& Parameters)
{
    const ABasicFpsGameMode* GMCDO = GetDefault<ABasicFpsGameMode>();
    TestNotNull(TEXT("GameMode CDO exists"), GMCDO);
    if (!GMCDO)
    {
        return false;
    }

    TestTrue(TEXT("DefaultPawnClass is AFirstPersonCharacter"), GMCDO->DefaultPawnClass == AFirstPersonCharacter::StaticClass());
    TestTrue(TEXT("PlayerControllerClass is AFirstPersonPlayerController"), GMCDO->PlayerControllerClass == AFirstPersonPlayerController::StaticClass());

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
