#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Engine/EngineTypes.h" // FActorSpawnParameters
#include "Interfaces/IAttackable.h"

// Forward declarations
class UStaticMeshComponent;
class UItemDefinition;
struct FPropertyChangedEvent;
struct FItemEntry;

#include "ItemPickup.generated.h"

/** Simple world pickup actor that carries a single item definition */
UCLASS()
class UNKNOWN_API AItemPickup : public AActor, public IAttackable
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

	// Item metadata (ItemId and CustomData)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	FGuid ItemId;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Pickup")
	TMap<FName, FString> CustomData;

	UFUNCTION(BlueprintPure, Category="Pickup")
	UItemDefinition* GetItemDef() const { return ItemDef; }
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetItemDef(UItemDefinition* InDef);
    
    UFUNCTION(BlueprintCallable, Category="Pickup")
    void SetItemEntry(const FItemEntry& Entry);
    
    UFUNCTION(BlueprintPure, Category="Pickup")
    FItemEntry GetItemEntry() const;

    // IAttackable implementation
public:
	virtual void OnAttacked_Implementation(ACharacter* Attacker, UItemDefinition* Weapon, const FVector& HitLocation, const FVector& HitDirection) override;

    // Unified drop helpers
public:
    /**
     * Compute a safe base drop transform in front of the given context actor (typically a Pawn or its Controller),
     * then compose it with the item's DefaultDropTransform (rotation/translation/scale) if provided.
     * The resulting transform can be used to spawn an AItemPickup so that scale/orientation/offset are consistent
     * across all drop code paths.
     */
    static FTransform BuildDropTransform(const AActor* ContextActor, const UItemDefinition* Def,
        float ForwardDist = 150.f, float BackOff = 10.f, float UpOffset = 10.f,
        float FloorProbeUp = 50.f, float FloorProbeDown = 200.f);

    /**
     * Spawn a dropped pickup for the given item definition using the unified BuildDropTransform().
     * Returns the newly spawned actor (or nullptr if spawning failed due to collision rules in Params).
     * @deprecated Use DropItemToWorld() instead to preserve metadata
     */
    static AItemPickup* SpawnDropped(UWorld* World, const AActor* ContextActor, UItemDefinition* Def,
        const FActorSpawnParameters& Params);
    
    /**
     * Unified drop helper that drops an item to the world with proper physics/collision settings.
     * Sets gravity ON, interactable channel BLOCK, and preserves ItemId and CustomData.
     * Returns the spawned pickup actor or nullptr if spawning failed.
     */
    static AItemPickup* DropItemToWorld(UWorld* World, const AActor* ContextActor, const FItemEntry& Entry,
        const FActorSpawnParameters& Params);

protected:
    virtual void OnConstruction(const FTransform& Transform) override;
#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void ApplyVisualsFromDef();
};
