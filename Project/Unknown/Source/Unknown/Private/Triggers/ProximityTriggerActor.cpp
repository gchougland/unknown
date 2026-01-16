#include "Triggers/ProximityTriggerActor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

AProximityTriggerActor::AProximityTriggerActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create root scene component
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Create trigger box
	TriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerBox"));
	TriggerBox->SetupAttachment(RootComponent);
	TriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 200.0f));
	TriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	TriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	TriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	TriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	TriggerBox->SetGenerateOverlapEvents(true);
}

void AProximityTriggerActor::BeginPlay()
{
	Super::BeginPlay();

	// Bind to trigger box events
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AProximityTriggerActor::OnTriggerBeginOverlap);
		TriggerBox->OnComponentEndOverlap.AddDynamic(this, &AProximityTriggerActor::OnTriggerEndOverlap);
	}
}

void AProximityTriggerActor::OnTriggerBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's a pawn (player)
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// Check if this is a one-time trigger that's already been triggered
	if (!bRetriggerOnExit && bHasBeenTriggered)
	{
		return;
	}

	// Trigger targets
	TriggerTargets();
	bHasBeenTriggered = true;
}

void AProximityTriggerActor::OnTriggerEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	// Check if it's a pawn (player)
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// If retrigger on exit is enabled, trigger again
	if (bRetriggerOnExit)
	{
		TriggerTargets();
	}
}

void AProximityTriggerActor::OnTriggerActivated()
{
	Super::OnTriggerActivated();
	// Could add visual/audio feedback here
}
