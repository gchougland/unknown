#include "Triggers/TriggerActorBase.h"
#include "Interfaces/ITriggerable.h"
#include "Engine/World.h"

ATriggerActorBase::ATriggerActorBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
}

void ATriggerActorBase::BeginPlay()
{
	Super::BeginPlay();

	// Validate targets on begin play
	if (!AreTargetsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriggerActorBase] %s has invalid target actors. Some targets may not implement ITriggerable."), *GetName());
	}
}

void ATriggerActorBase::TriggerTargets()
{
	if (TargetActors.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriggerActorBase] %s has no target actors assigned."), *GetName());
		return;
	}

	// Trigger each target actor
	for (AActor* TargetActor : TargetActors)
	{
		if (IsValid(TargetActor))
		{
			TriggerTargetActor(TargetActor);
		}
	}

	// Notify derived classes
	OnTriggerActivated();
}

bool ATriggerActorBase::AreTargetsValid() const
{
	for (const AActor* TargetActor : TargetActors)
	{
		if (!IsValid(TargetActor))
		{
			return false;
		}

		// Check if target implements ITriggerable
		if (!Cast<ITriggerable>(const_cast<AActor*>(TargetActor)))
		{
			return false;
		}
	}

	return TargetActors.Num() > 0;
}

void ATriggerActorBase::OnTriggerActivated()
{
	// Override in derived classes for custom behavior
}

void ATriggerActorBase::TriggerTargetActor(AActor* TargetActor)
{
	if (!IsValid(TargetActor))
	{
		return;
	}

	// Check if target implements ITriggerable
	if (ITriggerable* Triggerable = Cast<ITriggerable>(TargetActor))
	{
		ITriggerable::Execute_Trigger(TargetActor);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[TriggerActorBase] Target actor %s does not implement ITriggerable interface."), *TargetActor->GetName());
	}
}
