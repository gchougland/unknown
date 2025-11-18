#include "Inventory/ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Inventory/ItemDefinition.h"

AItemPickup::AItemPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Mesh = ObjectInitializer.CreateDefaultSubobject<UStaticMeshComponent>(this, TEXT("Mesh"));
	SetRootComponent(Mesh);
	if (Mesh)
	{
		// Spawned pickups should simulate and collide by default so they behave like physical props
		Mesh->SetSimulatePhysics(true);
		Mesh->SetEnableGravity(true);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionObjectType(ECC_WorldDynamic);
		// Block most things so it rests on the world and can be interacted with
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		// Ensure our interact trace channel definitely hits it
		Mesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

		// Try to use Engine BasicShapes Cube as a default so the pickup is visible
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh.Succeeded())
        {
            Mesh->SetStaticMesh(CubeMesh.Object);
            // Do not override scale here; preserve the actor's spawn/world scale so
            // DefaultDropTransform (including scale) can be applied by spawners.
        }
    }
}

void AItemPickup::ApplyVisualsFromDef()
{
    if (!Mesh)
    {
        return;
    }
 // Mesh
 UStaticMesh* NewMesh = (ItemDef ? ItemDef->PickupMesh : nullptr);
 Mesh->SetStaticMesh(NewMesh);
 // IMPORTANT: Do not change the component's world scale here. The actor's world
 // transform (including scale) may be intentionally set by drop/placer logic
 // via UItemDefinition::DefaultDropTransform. Keeping the mesh relative scale
 // at its default (1,1,1) ensures the actor's scale propagates correctly.
    // Physics mass
    if (ItemDef)
    {
        const float Mass = ItemDef->MassKg;
        if (Mass > 0.f)
        {
            Mesh->SetMassOverrideInKg(NAME_None, Mass, true);
        }
        else
        {
            // Use mesh default (clear override)
            Mesh->SetMassOverrideInKg(NAME_None, 0.f, false);
        }

        // Collision with Pawns per definition
        // Default mesh settings block all channels in constructor; override Pawn here
        const bool bCollideWithPawns = ItemDef->bCollideWithPawns;
        Mesh->SetCollisionResponseToChannel(ECC_Pawn, bCollideWithPawns ? ECR_Block : ECR_Ignore);
    }
    else
    {
        // No item def — keep constructor defaults (block pawns)
        Mesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    }
}

void AItemPickup::SetItemDef(UItemDefinition* InDef)
{
	ItemDef = InDef;
	ApplyVisualsFromDef();
}

void AItemPickup::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyVisualsFromDef();
}

#if WITH_EDITOR
void AItemPickup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ApplyVisualsFromDef();
}
#endif
