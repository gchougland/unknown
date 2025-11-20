#include "Inventory/ItemPickup.h"
#include "Components/StaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

AItemPickup::AItemPickup(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, ItemId(FGuid())
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

void AItemPickup::SetItemEntry(const FItemEntry& Entry)
{
    ItemDef = Entry.Def;
    ItemId = Entry.ItemId;
    CustomData = Entry.CustomData;
    ApplyVisualsFromDef();
}

FItemEntry AItemPickup::GetItemEntry() const
{
    FItemEntry Entry;
    Entry.Def = ItemDef;
    Entry.ItemId = ItemId;
    Entry.CustomData = CustomData;
    return Entry;
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

FTransform AItemPickup::BuildDropTransform(const AActor* ContextActor, const UItemDefinition* Def,
    float ForwardDist, float BackOff, float UpOffset, float FloorProbeUp, float FloorProbeDown)
{
    if (!ContextActor)
    {
        // No context; just return the definition's drop transform as world (unlikely path)
        return Def ? Def->DefaultDropTransform : FTransform::Identity;
    }

    const UWorld* World = ContextActor->GetWorld();
    FVector ViewLoc = ContextActor->GetActorLocation() + FVector(0,0,50);
    FRotator ViewRot = ContextActor->GetActorRotation();

    const APawn* AsPawn = Cast<APawn>(ContextActor);
    const APlayerController* PC = nullptr;
    if (AsPawn)
    {
        PC = Cast<APlayerController>(AsPawn->GetController());
    }
    else
    {
        PC = Cast<APlayerController>(ContextActor);
    }
    if (PC)
    {
        PC->GetPlayerViewPoint(ViewLoc, ViewRot);
    }

    const FVector Forward = ViewRot.Vector();
    const FVector TraceEnd = ViewLoc + Forward * ForwardDist;

    FVector SpawnLoc = TraceEnd + FVector(0,0,UpOffset);
    FRotator SpawnRot = FRotator(0.f, ViewRot.Yaw, 0.f);

    if (World)
    {
        FCollisionQueryParams QParams(SCENE_QUERY_STAT(ItemDrop), /*bTraceComplex*/ false, AsPawn);
        FHitResult Hit;
        const bool bHit = World->LineTraceSingleByChannel(Hit, ViewLoc, TraceEnd, ECC_Visibility, QParams);
        if (bHit && Hit.bBlockingHit)
        {
            SpawnLoc = Hit.ImpactPoint - Forward * BackOff + Hit.ImpactNormal * UpOffset;
        }
        else
        {
            const FVector ProbeStart = TraceEnd + FVector(0,0,FloorProbeUp);
            const FVector ProbeEnd = TraceEnd - FVector(0,0,FloorProbeDown);
            FHitResult FloorHit;
            if (World->LineTraceSingleByChannel(FloorHit, ProbeStart, ProbeEnd, ECC_Visibility, QParams) && FloorHit.bBlockingHit)
            {
                SpawnLoc = FloorHit.ImpactPoint + FVector(0,0,UpOffset);
            }
        }

        // If we have a pawn, prefer aligning to pawn yaw
        if (AsPawn)
        {
            SpawnRot = FRotator(0.f, AsPawn->GetActorRotation().Yaw, 0.f);
        }
    }

    const FTransform BaseTransform(SpawnRot, SpawnLoc, FVector(1.f,1.f,1.f));
    return Def ? (Def->DefaultDropTransform * BaseTransform) : BaseTransform;
}

AItemPickup* AItemPickup::SpawnDropped(UWorld* World, const AActor* ContextActor, UItemDefinition* Def,
    const FActorSpawnParameters& Params)
{
    if (!World)
    {
        return nullptr;
    }
    const FTransform SpawnXform = BuildDropTransform(ContextActor, Def);
    AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), SpawnXform, Params);
    if (Pickup)
    {
        Pickup->SetItemDef(Def);
    }
    return Pickup;
}

AItemPickup* AItemPickup::DropItemToWorld(UWorld* World, const AActor* ContextActor, const FItemEntry& Entry,
    const FActorSpawnParameters& Params)
{
    if (!World || !Entry.IsValid() || !Entry.Def)
    {
        return nullptr;
    }
    
    const FTransform SpawnXform = BuildDropTransform(ContextActor, Entry.Def);
    AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), SpawnXform, Params);
    if (!Pickup)
    {
        return nullptr;
    }
    
    // Set the full item entry (includes metadata)
    Pickup->SetItemEntry(Entry);
    
    // Configure physics for dropped items: gravity ON, interactable channel BLOCK
    if (UStaticMeshComponent* ItemMesh = Pickup->Mesh)
    {
        ItemMesh->SetSimulatePhysics(true);
        ItemMesh->SetEnableGravity(true);
        ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
        ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
        // Ensure interactable channel blocks
        ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
    }
    
    return Pickup;
}
