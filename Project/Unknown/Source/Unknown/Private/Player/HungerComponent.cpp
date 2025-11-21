#include "Player/HungerComponent.h"

UHungerComponent::UHungerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UHungerComponent::BeginPlay()
{
	Super::BeginPlay();
	CurrentHunger = InitialHunger;
	NotifyHungerChanged();
}

void UHungerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Decay hunger over time
	if (CurrentHunger > 0.0f && DecayRate > 0.0f)
	{
		const float OldHunger = CurrentHunger;
		CurrentHunger = FMath::Max(0.0f, CurrentHunger - (DecayRate * DeltaTime));
		
		// Notify if hunger changed significantly (avoid spamming events)
		if (FMath::Abs(CurrentHunger - OldHunger) > 0.1f)
		{
			NotifyHungerChanged();
		}
	}
}

void UHungerComponent::RestoreHunger(float Amount)
{
	if (Amount > 0.0f)
	{
		const float OldHunger = CurrentHunger;
		CurrentHunger += Amount;
		NotifyHungerChanged();
	}
}

void UHungerComponent::NotifyHungerChanged()
{
	OnHungerChanged.Broadcast(CurrentHunger, MaxHunger);
}

