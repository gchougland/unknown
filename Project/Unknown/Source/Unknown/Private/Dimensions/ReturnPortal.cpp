#include "Dimensions/ReturnPortal.h"
#include "Components/BoxComponent.h"
#include "Components/SaveableActorComponent.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

AReturnPortal::AReturnPortal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

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

void AReturnPortal::BeginPlay()
{
	Super::BeginPlay();

	// Bind overlap event
	if (TriggerBox)
	{
		// Remove any existing bindings first to avoid duplicate delegate errors
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &AReturnPortal::OnBeginOverlap);
		// Now add the binding
		TriggerBox->OnComponentBeginOverlap.AddDynamic(this, &AReturnPortal::OnBeginOverlap);
	}
}

void AReturnPortal::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Clean up delegate binding when actor is destroyed
	if (TriggerBox)
	{
		TriggerBox->OnComponentBeginOverlap.RemoveDynamic(this, &AReturnPortal::OnBeginOverlap);
	}

	Super::EndPlay(EndPlayReason);
}

void AReturnPortal::OnBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if it's the player
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	// Teleport the player
	TeleportPlayer();
}

void AReturnPortal::TeleportPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check if dimension is loaded (don't teleport if dimension isn't loaded)
	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	UDimensionManagerSubsystem* DimensionManager = GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
	if (!DimensionManager || !DimensionManager->IsDimensionLoaded())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ReturnPortal] Cannot teleport: No dimension is currently loaded"));
		return;
	}

	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0);
	if (!PlayerPawn)
	{
		return;
	}

	// Teleport player to exit location
	FVector TeleportLocation = ExitLocation;
	FRotator TeleportRotation = ExitRotation;

	// If exit rotation is zero, use player's current rotation
	if (TeleportRotation.IsZero())
	{
		TeleportRotation = PlayerPawn->GetActorRotation();
	}

	PlayerPawn->SetActorLocation(TeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
	PlayerPawn->SetActorRotation(TeleportRotation);

	// Also update controller rotation
	if (APlayerController* PlayerController = Cast<APlayerController>(PlayerPawn->GetController()))
	{
		PlayerController->SetControlRotation(TeleportRotation);
	}

	// Clear player's dimension instance ID (player is back in main world)
	DimensionManager->ClearPlayerDimensionInstanceId();
	UE_LOG(LogTemp, Log, TEXT("[ReturnPortal] Cleared player dimension instance ID"));

	UE_LOG(LogTemp, Log, TEXT("[ReturnPortal] Teleported player to exit location: %s"), *TeleportLocation.ToString());
}
