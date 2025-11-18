#if WITH_DEV_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"

// Verify that an AItemPickup respects a scaled DefaultDropTransform and that
// calling SetItemDef does not clobber the actor/world scale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FItemPlacementScaleTest,
    "Project.Smoke.ItemPlacementScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter);

bool FItemPlacementScaleTest::RunTest(const FString& Parameters)
{
    // Create a transient item definition and set a non-uniform scale to catch axis mistakes
    UItemDefinition* Def = NewObject<UItemDefinition>(GetTransientPackage(), TEXT("TempItemDef"));
    if (!TestNotNull(TEXT("Create transient UItemDefinition"), Def))
    {
        return false;
    }

    const FVector ExpectedScale(2.0f, 1.5f, 0.5f);
    Def->DefaultDropTransform = FTransform(FQuat::Identity, FVector::ZeroVector, ExpectedScale);

    // Base transform at origin with identity rotation/scale
    const FTransform BaseTransform(FRotator::ZeroRotator, FVector::ZeroVector, FVector(1.0f));
    const FTransform FinalTransform = Def->DefaultDropTransform * BaseTransform;

    // Construct an actor instance without placing it in a World (consistent with other tests here)
    AItemPickup* Pickup = NewObject<AItemPickup>(GetTransientPackage(), TEXT("TempPickup"));
    if (!TestNotNull(TEXT("Create AItemPickup via NewObject"), Pickup))
    {
        return false;
    }

    // Apply the transform (including scale) as if spawned with FinalTransform
    Pickup->SetActorTransform(FinalTransform);

    // Apply definition visuals which must NOT clobber actor/world scale
    Pickup->SetItemDef(Def);

    const FVector ActualScale = Pickup->GetActorScale3D();
    const float Tolerance = 1e-3f;
    TestTrue(TEXT("World scale X preserved"), FMath::IsNearlyEqual(ActualScale.X, ExpectedScale.X, Tolerance));
    TestTrue(TEXT("World scale Y preserved"), FMath::IsNearlyEqual(ActualScale.Y, ExpectedScale.Y, Tolerance));
    TestTrue(TEXT("World scale Z preserved"), FMath::IsNearlyEqual(ActualScale.Z, ExpectedScale.Z, Tolerance));
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
