#include "Triggers/ButtonTriggerActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/FirstPersonPlayerController.h"
#include "Engine/World.h"

AButtonTriggerActor::AButtonTriggerActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create root scene component
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Create button mesh
	ButtonMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ButtonMesh"));
	ButtonMesh->SetupAttachment(RootComponent);
	ButtonMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	ButtonMesh->SetCollisionObjectType(ECC_WorldDynamic);
	ButtonMesh->SetCollisionResponseToAllChannels(ECR_Block);
	ButtonMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);

	// Create interaction collider
	InteractionCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollider"));
	InteractionCollider->SetupAttachment(RootComponent);
	InteractionCollider->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	InteractionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollider->SetCollisionObjectType(ECC_WorldDynamic);
	InteractionCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollider->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
}

void AButtonTriggerActor::BeginPlay()
{
	Super::BeginPlay();
}

bool AButtonTriggerActor::CanBeInteractedWith(APawn* InteractingPawn) const
{
	if (!InteractingPawn || !InteractionCollider)
	{
		return false;
	}

	// Check if pawn has a player controller
	APlayerController* PC = Cast<APlayerController>(InteractingPawn->GetController());
	if (!PC)
	{
		return false;
	}

	// Perform line trace from camera
	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Dir = CamRot.Vector();
	const float MaxDist = 500.0f;
	const FVector Start = CamLoc;
	const FVector End = Start + Dir * MaxDist;

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ButtonInteract), false);
	Params.AddIgnoredActor(InteractingPawn);

	UWorld* World = GetWorld();
	if (World && World->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
	{
		// Check if we hit this button's interaction collider
		return Hit.GetActor() == this || Hit.GetComponent() == InteractionCollider || Hit.GetComponent() == ButtonMesh;
	}

	return false;
}

void AButtonTriggerActor::OnInteracted()
{
	// Trigger all targets
	TriggerTargets();
}

void AButtonTriggerActor::OnTriggerActivated()
{
	Super::OnTriggerActivated();
	// Could add button press animation or sound here
}
