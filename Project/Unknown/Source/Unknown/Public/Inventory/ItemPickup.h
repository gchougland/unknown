#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// Forward declarations
class UStaticMeshComponent;
class UItemDefinition;
struct FPropertyChangedEvent;

#include "ItemPickup.generated.h"

/** Simple world pickup actor that carries a single item definition */
UCLASS()
class UNKNOWN_API AItemPickup : public AActor
{
	GENERATED_BODY()
public:
	AItemPickup(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Visual mesh and simple collision
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	TObjectPtr<UStaticMeshComponent> Mesh;

	// Item payload (one unit)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Pickup")
	TObjectPtr<UItemDefinition> ItemDef;

	UFUNCTION(BlueprintPure, Category="Pickup")
	UItemDefinition* GetItemDef() const { return ItemDef; }
	UFUNCTION(BlueprintCallable, Category="Pickup")
	void SetItemDef(UItemDefinition* InDef);

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	void ApplyVisualsFromDef();
};
