#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

// Drive TDD for physics interaction system
#include "Components/PhysicsInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_ComponentExists,
    "Project.Interaction.Physics.ComponentExists",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_ComponentExists::RunTest(const FString&)
{
    TestNotNull(TEXT("UPhysicsInteractionComponent class exists"), UPhysicsInteractionComponent::StaticClass());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_Defaults,
    "Project.Interaction.Physics.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_Defaults::RunTest(const FString&)
{
    UPhysicsInteractionComponent* Comp = NewObject<UPhysicsInteractionComponent>();
    TestNotNull(TEXT("Component instance"), Comp);
    if (!Comp) return false;

    TestEqual(TEXT("MaxPickupDistance default"), Comp->GetMaxPickupDistance(), 300.f);
    TestEqual(TEXT("MinHoldDistance default"), Comp->GetMinHoldDistance(), 50.f);
    TestEqual(TEXT("MaxHoldDistance default"), Comp->GetMaxHoldDistance(), 300.f);
    TestEqual(TEXT("ThrowImpulse default"), Comp->GetThrowImpulse(), 4000.f);
    TestEqual(TEXT("RotationSensitivity default"), Comp->GetRotationSensitivity(), 1.f);

    // Utility clamp
    TestEqual(TEXT("Clamp small to min"), Comp->ClampHoldDistance(10.f), 50.f);
    TestEqual(TEXT("Clamp large to max"), Comp->ClampHoldDistance(10000.f), 300.f);
    TestEqual(TEXT("Clamp mid unchanged"), Comp->ClampHoldDistance(120.f), 120.f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_BeginHold_Releases,
    "Project.Interaction.Physics.BeginHoldAndRelease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_BeginHold_Releases::RunTest(const FString&)
{
    // Create an outer actor to hold a primitive
    AActor* Outer = NewObject<AActor>();
    TestNotNull(TEXT("Outer actor"), Outer);

    UStaticMeshComponent* Mesh = NewObject<UStaticMeshComponent>(Outer);
    TestNotNull(TEXT("Mesh component"), Mesh);
    if (!Mesh) return false;

    UPhysicsInteractionComponent* Comp = NewObject<UPhysicsInteractionComponent>(Outer);
    TestNotNull(TEXT("Interaction component"), Comp);

    const FVector Pivot = FVector(10, 20, 30);
    const float PickDistance = 275.f;

    bool bPickedUp = false;
    bool bReleased = false;
    bool bThrown = false;
    Comp->OnPickedUp.AddLambda([&](UPrimitiveComponent*, const FVector&){ bPickedUp = true; });
    Comp->OnReleased.AddLambda([&](UPrimitiveComponent*){ bReleased = true; });
    Comp->OnThrown.AddLambda([&](UPrimitiveComponent*, const FVector&){ bThrown = true; });

    // Begin hold
    const bool bBegin = Comp->BeginHold(Mesh, Pivot, PickDistance);
    TestTrue(TEXT("BeginHold returns true"), bBegin);
    TestTrue(TEXT("IsHolding == true"), Comp->IsHolding());
    TestTrue(TEXT("Picked up delegate fired"), bPickedUp);

    // Validate pivot local mapping round-trip
    const FVector RecalcPivot = Mesh->GetComponentTransform().TransformPosition(Comp->GetPivotLocal());
    TestTrue(TEXT("Pivot world matches original within small epsilon"), RecalcPivot.Equals(Pivot, 0.01f));

    // Throw should clear holding and fire throw
    Comp->Throw(FVector(0,0,1));
    TestTrue(TEXT("Thrown delegate fired"), bThrown);

    // After throw we are no longer holding
    TestFalse(TEXT("IsHolding == false after throw"), Comp->IsHolding());

    // Begin again then release
    Comp->BeginHold(Mesh, Pivot, PickDistance);
    Comp->Release();
    TestTrue(TEXT("Release delegate fired"), bReleased);
    TestFalse(TEXT("IsHolding == false after release"), Comp->IsHolding());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FPhysicsInteract_RotationInput,
    "Project.Interaction.Physics.RotationInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FPhysicsInteract_RotationInput::RunTest(const FString&)
{
    UPhysicsInteractionComponent* Comp = NewObject<UPhysicsInteractionComponent>();
    TestNotNull(TEXT("Component"), Comp);

    Comp->SetRotateHeld(true);
    Comp->AddRotationInput(FVector2D(10.f, -5.f));

    const FVector2D Accum = Comp->GetAccumulatedRotationInput();
    TestEqual(TEXT("Accumulated yaw"), Accum.X, 10.0);
    TestEqual(TEXT("Accumulated pitch"), Accum.Y, -5.0);

    Comp->Release();
    const FVector2D After = Comp->GetAccumulatedRotationInput();
    TestEqual(TEXT("Rotation input resets on release (yaw)"), After.X, 0.0);
    TestEqual(TEXT("Rotation input resets on release (pitch)"), After.Y, 0.0);

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
