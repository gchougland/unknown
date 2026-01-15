#include "Computers/ComputerBase.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

AComputerBase::AComputerBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create camera component
	ComputerCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ComputerCamera"));
	ComputerCamera->SetupAttachment(RootComponent);
	ComputerCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	// Create screen mesh
	ScreenMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ScreenMesh"));
	ScreenMesh->SetupAttachment(RootComponent);

	// Create interaction collider
	InteractionCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollider"));
	InteractionCollider->SetupAttachment(RootComponent);
	InteractionCollider->SetBoxExtent(FVector(50.0f, 50.0f, 50.0f));
	InteractionCollider->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollider->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
	InteractionCollider->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
	InteractionCollider->SetCollisionResponseToChannel(ECollisionChannel::ECC_GameTraceChannel1, ECollisionResponse::ECR_Block);

	// Create crosshair widget
	CrosshairWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("CrosshairWidget"));
	CrosshairWidget->SetupAttachment(ScreenMesh);
	CrosshairWidget->SetWidgetSpace(EWidgetSpace::World);
	CrosshairWidget->SetDrawSize(FVector2D(100.0f, 100.0f));
}

void AComputerBase::BeginPlay()
{
	Super::BeginPlay();
}

void AComputerBase::EnterComputerMode(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	CurrentPlayerController = PC;
	OriginalViewTarget = PC->GetViewTarget();

	// Blend camera to computer camera
	if (ComputerCamera)
	{
		PC->SetViewTargetWithBlend(this, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
	}

	// Setup computer controls
	SetupComputerControls(PC);
}

void AComputerBase::ExitComputerMode(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	// Restore player controls
	RestorePlayerControls(PC);

	// Restore original camera
	if (OriginalViewTarget.IsValid())
	{
		PC->SetViewTargetWithBlend(OriginalViewTarget.Get(), CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
	}
	else if (APawn* Pawn = PC->GetPawn())
	{
		PC->SetViewTargetWithBlend(Pawn, CameraBlendTime, EViewTargetBlendFunction::VTBlend_Cubic);
	}

	CurrentPlayerController = nullptr;
	OriginalViewTarget = nullptr;
}

void AComputerBase::SetupComputerControls(APlayerController* PC)
{
	// Override in derived classes
}

void AComputerBase::RestorePlayerControls(APlayerController* PC)
{
	// Override in derived classes
}
