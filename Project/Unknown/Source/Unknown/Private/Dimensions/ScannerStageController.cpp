#include "Dimensions/ScannerStageController.h"
#include "Dimensions/DimensionalSignal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/Box.h"
#include "Components/SceneComponent.h"

AScannerStageController::AScannerStageController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;
	SignalClass = ADimensionalSignal::StaticClass();

	// Create root component
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	// Create cursor mesh (sphere)
	CursorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CursorMesh"));
	CursorMesh->SetupAttachment(RootComponent);
	CursorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	CursorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Create scene capture component attached to cursor
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("SceneCapture"));
	SceneCapture->SetupAttachment(CursorMesh);
	SceneCapture->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	SceneCapture->SetRelativeRotation(FRotator(0.0f, 0.0f, 0.0f));

	// Create background cube
	BackgroundCube = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BackgroundCube"));
	BackgroundCube->SetupAttachment(RootComponent);
	BackgroundCube->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));
	BackgroundCube->SetRelativeScale3D(FVector(10.0f, 10.0f, 10.0f)); // 1000x1000x1000 if using 100 unit cube mesh

	// Create multiverse Niagara effect
	MultiverseEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("MultiverseEffect"));
	MultiverseEffect->SetupAttachment(RootComponent);

	// Create bounds volume (1000x1000x1000)
	BoundsVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsVolume"));
	BoundsVolume->SetupAttachment(RootComponent);
	BoundsVolume->SetBoxExtent(FVector(500.0f, 500.0f, 500.0f)); // Half-extent, so 1000x1000x1000 total
	BoundsVolume->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsVolume->SetVisibility(false);
}

void AScannerStageController::BeginPlay()
{
	Super::BeginPlay();

	// Initialize cursor at center
	CursorMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 0.0f));

	// Spawn initial signals
	for (int32 i = 0; i < TargetSignalCount; ++i)
	{
		SpawnDimensionalSignal();
	}
}

void AScannerStageController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update active signals
	UpdateActiveSignals();

	// Update spawn timer
	SignalSpawnTimer += DeltaTime;
	if (SignalSpawnTimer >= SignalSpawnInterval)
	{
		SignalSpawnTimer = 0.0f;
		// Spawn will be handled by UpdateActiveSignals if needed
	}
}

void AScannerStageController::MoveCursor(const FVector& DeltaMovement)
{
	FVector CurrentLocalPos = CursorMesh->GetRelativeLocation();
	FVector NewLocalPos = CurrentLocalPos + DeltaMovement;
	CursorMesh->SetRelativeLocation(NewLocalPos);
	ClampCursorToBounds();
}

FVector AScannerStageController::GetCursorLocalPosition() const
{
	return CursorMesh->GetRelativeLocation();
}

FVector AScannerStageController::GetCursorWorldPosition() const
{
	return CursorMesh->GetComponentLocation();
}

TArray<ADimensionalSignal*> AScannerStageController::GetSignalsNearPosition(const FVector& WorldPos, float Range) const
{
	TArray<ADimensionalSignal*> NearbySignals;

	for (ADimensionalSignal* Signal : ActiveSignals)
	{
		if (Signal && IsValid(Signal))
		{
			// Skip signals that have already been scanned
			if (Signal->HasBeenScanned())
			{
				continue;
			}

			float Distance = FVector::Dist(WorldPos, Signal->GetActorLocation());
			if (Distance <= Range)
			{
				NearbySignals.Add(Signal);
			}
		}
	}

	return NearbySignals;
}

void AScannerStageController::SetNiagaraCaptureX(float LocalX)
{
	if (MultiverseEffect)
	{
		MultiverseEffect->SetVariableFloat(FName("User.System_CaptureX"), LocalX);
		// Also try setting it as a user parameter
		MultiverseEffect->SetVariableFloat(FName("System_CaptureX"), LocalX);
	}
}

void AScannerStageController::SpawnDimensionalSignal()
{
	if (!SignalClass || !GetWorld())
	{
		return;
	}

	// Generate random position within spawn bounds
	FVector Center = SignalSpawnBounds.GetCenter();
	FVector Extent = SignalSpawnBounds.GetExtent();
	FVector SpawnLocation(
		FMath::RandRange(Center.X - Extent.X, Center.X + Extent.X),
		FMath::RandRange(Center.Y - Extent.Y, Center.Y + Extent.Y),
		FMath::RandRange(Center.Z - Extent.Z, Center.Z + Extent.Z)
	);

	// Convert to world space
	FVector WorldSpawnLocation = GetActorTransform().TransformPosition(SpawnLocation);

	// Generate random lifetime
	float Lifetime = FMath::RandRange(SignalLifetimeRange.X, SignalLifetimeRange.Y);

	// Spawn signal
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	
	ADimensionalSignal* NewSignal = GetWorld()->SpawnActor<ADimensionalSignal>(
		SignalClass,
		WorldSpawnLocation,
		FRotator::ZeroRotator,
		SpawnParams
	);

	if (NewSignal)
	{
		NewSignal->InitializeSignal(Lifetime);
		ActiveSignals.Add(NewSignal);
	}
}

void AScannerStageController::UpdateActiveSignals()
{
	// Remove invalid or expired signals
	ActiveSignals.RemoveAll([](const TObjectPtr<ADimensionalSignal>& Signal)
	{
		return !Signal || !IsValid(Signal) || Signal->IsExpired();
	});

	// Spawn new signals if needed
	while (ActiveSignals.Num() < TargetSignalCount)
	{
		SpawnDimensionalSignal();
	}
}

void AScannerStageController::ClampCursorToBounds()
{
	FVector CurrentPos = CursorMesh->GetRelativeLocation();
	FVector BoundsExtent = BoundsVolume->GetScaledBoxExtent();

	// Clamp to bounds (centered at origin, so -500 to +500 in each axis)
	FVector ClampedPos;
	ClampedPos.X = FMath::Clamp(CurrentPos.X, -BoundsExtent.X, BoundsExtent.X);
	ClampedPos.Y = FMath::Clamp(CurrentPos.Y, -BoundsExtent.Y, BoundsExtent.Y);
	ClampedPos.Z = FMath::Clamp(CurrentPos.Z, -BoundsExtent.Z, BoundsExtent.Z);

	CursorMesh->SetRelativeLocation(ClampedPos);
}
