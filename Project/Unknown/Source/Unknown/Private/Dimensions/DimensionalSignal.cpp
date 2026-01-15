#include "Dimensions/DimensionalSignal.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Components/SceneComponent.h"
#include "TimerManager.h"

ADimensionalSignal::ADimensionalSignal(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create root scene component
	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
	RootComponent = RootSceneComponent;

	// Create Niagara component
	SignalEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SignalEffect"));
	SignalEffect->SetupAttachment(RootComponent);
}

void ADimensionalSignal::BeginPlay()
{
	Super::BeginPlay();

	// Set Niagara system if provided
	if (SignalNiagaraSystem && SignalEffect)
	{
		SignalEffect->SetAsset(SignalNiagaraSystem);
		SignalEffect->Activate();
	}
}

void ADimensionalSignal::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (RemainingLifetime > 0.0f)
	{
		RemainingLifetime -= DeltaTime;

		// Auto-destroy when lifetime expires
		if (RemainingLifetime <= 0.0f)
		{
			Destroy();
		}
	}
}

void ADimensionalSignal::InitializeSignal(float Lifetime)
{
	RemainingLifetime = Lifetime;
}

void ADimensionalSignal::OnScanned()
{
	// Prevent multiple scans
	if (bHasBeenScanned)
	{
		return;
	}

	// Mark as scanned
	bHasBeenScanned = true;

	// Stop the Niagara emitter
	if (SignalEffect)
	{
		SignalEffect->Deactivate();
	}

	// Set timer to destroy after 5 seconds
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			DelayedDestroyTimerHandle,
			this,
			&ADimensionalSignal::OnDelayedDestroy,
			5.0f,
			false
		);
	}
}

void ADimensionalSignal::OnDelayedDestroy()
{
	// Destroy the signal after the delay
	Destroy();
}
