#pragma once

#include "CoreMinimal.h"
#include "ActorFactories/ActorFactory.h"
#include "ActorFactoryItemPickup.generated.h"

class UItemDefinition;
class AItemPickup;

/** ActorFactory that allows dragging a UItemDefinition into the level to create an AItemPickup pre-configured with that definition. */
UCLASS(MinimalAPI)
class UActorFactoryItemPickup : public UActorFactory
{
	GENERATED_BODY()
public:
	UActorFactoryItemPickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// UActorFactory
	virtual bool CanCreateActorFrom(const FAssetData& AssetData, FText& OutErrorMsg) override;
	virtual UObject* GetAssetFromActorInstance(AActor* ActorInstance) override;
	virtual AActor* SpawnActor(UObject* Asset, ULevel* InLevel, const FTransform& Transform, const FActorSpawnParameters& SpawnParams) override;
};
