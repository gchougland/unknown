#include "Doors/SlidingDoor.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Math/UnrealMathUtility.h"

ASlidingDoor::ASlidingDoor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root scene component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Create left door mesh
	LeftDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftDoorMesh"));
	LeftDoorMesh->SetupAttachment(RootComponent);
	LeftDoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	LeftDoorMesh->SetCollisionObjectType(ECC_WorldDynamic);
	LeftDoorMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Create right door mesh
	RightDoorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightDoorMesh"));
	RightDoorMesh->SetupAttachment(RootComponent);
	RightDoorMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	RightDoorMesh->SetCollisionObjectType(ECC_WorldDynamic);
	RightDoorMesh->SetCollisionResponseToAllChannels(ECR_Block);

	// Create player detection box
	PlayerDetectionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PlayerDetectionBox"));
	PlayerDetectionBox->SetupAttachment(RootComponent);
	PlayerDetectionBox->SetBoxExtent(FVector(100.0f, 100.0f, 200.0f));
	PlayerDetectionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PlayerDetectionBox->SetCollisionObjectType(ECC_WorldDynamic);
	PlayerDetectionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	PlayerDetectionBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PlayerDetectionBox->SetGenerateOverlapEvents(true);
}

void ASlidingDoor::BeginPlay()
{
	Super::BeginPlay();

	// Store initial closed positions
	if (LeftDoorMesh)
	{
		LeftDoorClosedPosition = LeftDoorMesh->GetRelativeLocation();
		LeftDoorOpenPosition = LeftDoorClosedPosition + GetSlideDirection(true) * DoorSlideDistance;
	}

	if (RightDoorMesh)
	{
		RightDoorClosedPosition = RightDoorMesh->GetRelativeLocation();
		RightDoorOpenPosition = RightDoorClosedPosition + GetSlideDirection(false) * DoorSlideDistance;
	}

	// Bind to player detection events
	if (PlayerDetectionBox)
	{
		PlayerDetectionBox->OnComponentBeginOverlap.AddDynamic(this, &ASlidingDoor::OnPlayerDetectionBeginOverlap);
		PlayerDetectionBox->OnComponentEndOverlap.AddDynamic(this, &ASlidingDoor::OnPlayerDetectionEndOverlap);
	}

	// Initialize door state
	SetDoorState(EDoorState::Closed);
}

void ASlidingDoor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Update door animation
	UpdateDoorAnimation(DeltaTime);
}

void ASlidingDoor::Trigger_Implementation()
{
	// Toggle door state based on current state
	switch (CurrentState)
	{
	case EDoorState::Closed:
		OpenDoor();
		break;
	case EDoorState::Open:
		CloseDoor();
		break;
	case EDoorState::Opening:
		// Reverse direction: start closing from current position
		SetDoorState(EDoorState::Closing);
		StopAutoCloseTimer();
		break;
	case EDoorState::Closing:
		// Reverse direction: start opening from current position
		SetDoorState(EDoorState::Opening);
		StopAutoCloseTimer();
		break;
	}
}

void ASlidingDoor::OpenDoor()
{
	if (CurrentState == EDoorState::Open || CurrentState == EDoorState::Opening)
	{
		return; // Already open or opening
	}

	SetDoorState(EDoorState::Opening);
	StopAutoCloseTimer();
}

void ASlidingDoor::CloseDoor()
{
	if (CurrentState == EDoorState::Closed || CurrentState == EDoorState::Closing)
	{
		return; // Already closed or closing
	}

	// Check if player is in the way
	if (HasOverlappingPawns())
	{
		// Don't close if player is in the way
		return;
	}

	SetDoorState(EDoorState::Closing);
	StopAutoCloseTimer();
}

void ASlidingDoor::OnPlayerDetectionBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn)
	{
		OverlappingPawns.Add(Pawn);
		
		// Cancel auto-close if player enters
		if (CurrentState == EDoorState::Open)
		{
			StopAutoCloseTimer();
		}
	}
}

void ASlidingDoor::OnPlayerDetectionEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (Pawn)
	{
		OverlappingPawns.Remove(Pawn);

		// Clean up invalid pawns from set
		TSet<TObjectPtr<APawn>> ValidPawns;
		for (const TObjectPtr<APawn>& P : OverlappingPawns)
		{
			if (IsValid(P))
			{
				ValidPawns.Add(P);
			}
		}
		OverlappingPawns = ValidPawns;

		// Restart auto-close timer if door is open and no pawns are overlapping
		if (CurrentState == EDoorState::Open && !HasOverlappingPawns())
		{
			StartAutoCloseTimer();
		}
	}
}

void ASlidingDoor::OnAutoCloseTimer()
{
	// Try to close the door
	CloseDoor();
}

void ASlidingDoor::UpdateDoorAnimation(float DeltaTime)
{
	float TargetProgress = GetTargetOpenProgress();
	
	// Calculate interpolation speed (how fast we move from 0.0 to 1.0)
	// DoorSlideSpeed is in units/second, DoorSlideDistance is total distance
	// So time to complete = DoorSlideDistance / DoorSlideSpeed
	// Interpolation rate = 1.0 / time_to_complete = DoorSlideSpeed / DoorSlideDistance
	float InterpSpeed = DoorSlideDistance > 0.0f ? (DoorSlideSpeed / DoorSlideDistance) : 1.0f;
	
	// Interpolate left door progress
	if (LeftDoorMesh)
	{
		LeftDoorProgress = FMath::FInterpTo(LeftDoorProgress, TargetProgress, DeltaTime, InterpSpeed);
		
		// Calculate new position
		FVector NewPosition = FMath::Lerp(LeftDoorClosedPosition, LeftDoorOpenPosition, LeftDoorProgress);
		LeftDoorMesh->SetRelativeLocation(NewPosition);
	}

	// Interpolate right door progress
	if (RightDoorMesh)
	{
		RightDoorProgress = FMath::FInterpTo(RightDoorProgress, TargetProgress, DeltaTime, InterpSpeed);
		
		// Calculate new position
		FVector NewPosition = FMath::Lerp(RightDoorClosedPosition, RightDoorOpenPosition, RightDoorProgress);
		RightDoorMesh->SetRelativeLocation(NewPosition);
	}

	// Check if animation is complete
	const float ProgressThreshold = 0.01f; // Small threshold to account for floating point precision
	if (CurrentState == EDoorState::Opening)
	{
		if (FMath::Abs(LeftDoorProgress - 1.0f) < ProgressThreshold && FMath::Abs(RightDoorProgress - 1.0f) < ProgressThreshold)
		{
			LeftDoorProgress = 1.0f;
			RightDoorProgress = 1.0f;
			SetDoorState(EDoorState::Open);
		}
	}
	else if (CurrentState == EDoorState::Closing)
	{
		if (FMath::Abs(LeftDoorProgress) < ProgressThreshold && FMath::Abs(RightDoorProgress) < ProgressThreshold)
		{
			SetDoorState(EDoorState::Closed);
			LeftDoorProgress = 0.0f;
			RightDoorProgress = 0.0f;
		}
	}
}

void ASlidingDoor::SetDoorState(EDoorState NewState)
{
	if (CurrentState == NewState)
	{
		return;
	}

	EDoorState OldState = CurrentState;
	CurrentState = NewState;

	// Handle state transitions
	if (NewState == EDoorState::Open && OldState == EDoorState::Opening)
	{
		// Door just opened - start auto-close timer if no pawns are overlapping
		if (!HasOverlappingPawns())
		{
			StartAutoCloseTimer();
		}
	}
}

float ASlidingDoor::GetTargetOpenProgress() const
{
	switch (CurrentState)
	{
	case EDoorState::Closed:
	case EDoorState::Closing:
		return 0.0f;
	case EDoorState::Open:
	case EDoorState::Opening:
		return 1.0f;
	default:
		return 0.0f;
	}
}

FVector ASlidingDoor::GetSlideDirection(bool bIsLeftDoor) const
{
	FVector Direction = FVector::ZeroVector;
	
	switch (SlideAxis)
	{
	case 0: // X axis
		Direction = bIsLeftDoor ? FVector(-1.0f, 0.0f, 0.0f) : FVector(1.0f, 0.0f, 0.0f);
		break;
	case 1: // Y axis
		Direction = bIsLeftDoor ? FVector(0.0f, -1.0f, 0.0f) : FVector(0.0f, 1.0f, 0.0f);
		break;
	case 2: // Z axis
		Direction = bIsLeftDoor ? FVector(0.0f, 0.0f, -1.0f) : FVector(0.0f, 0.0f, 1.0f);
		break;
	}
	
	return Direction;
}

bool ASlidingDoor::HasOverlappingPawns() const
{
	// Check if any valid pawns are overlapping
	for (const TObjectPtr<APawn>& Pawn : OverlappingPawns)
	{
		if (IsValid(Pawn))
		{
			return true;
		}
	}
	
	return false;
}

void ASlidingDoor::StartAutoCloseTimer()
{
	if (AutoCloseDelay <= 0.0f)
	{
		return; // Auto-close disabled
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Clear any existing timer
	StopAutoCloseTimer();

	// Set new timer
	World->GetTimerManager().SetTimer(
		AutoCloseTimerHandle,
		this,
		&ASlidingDoor::OnAutoCloseTimer,
		AutoCloseDelay,
		false
	);
}

void ASlidingDoor::StopAutoCloseTimer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	World->GetTimerManager().ClearTimer(AutoCloseTimerHandle);
}
