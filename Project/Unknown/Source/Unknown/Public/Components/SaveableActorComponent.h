#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SaveableActorComponent.generated.h"

/**
 * Component that provides persistent GUID for actor identification in save system.
 * Stores original spawn transform for physics objects to detect modifications.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API USaveableActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	USaveableActorComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void PostInitProperties() override;
	virtual void BeginPlay() override;

	// Persistent unique identifier for this actor
	// Marked with SaveGame so it persists across save/load cycles
	UPROPERTY(SaveGame, EditAnywhere, BlueprintReadWrite, Category="Save")
	FGuid PersistentId;

	// Original spawn/placed transform (used to detect if physics objects have moved)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Save")
	FTransform OriginalTransform;

	// Whether to save this actor's transform
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Save")
	bool bSaveTransform = true;

	// Whether to save physics velocity/angular velocity
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Save")
	bool bSavePhysicsState = true;

	// Dimension instance ID this actor belongs to (empty for main world actors)
	UPROPERTY(SaveGame, VisibleAnywhere, BlueprintReadOnly, Category="Save|Dimension")
	FGuid DimensionInstanceId;

	// Get the persistent ID (generates one if not set)
	UFUNCTION(BlueprintPure, Category="Save")
	FGuid GetPersistentId() const { return PersistentId; }

	// Set the persistent ID (used when restoring from save)
	UFUNCTION(BlueprintCallable, Category="Save")
	void SetPersistentId(const FGuid& NewId) { PersistentId = NewId; }

	// Set dimension instance ID (used to tag actors in dimensions)
	UFUNCTION(BlueprintCallable, Category="Save|Dimension")
	void SetDimensionInstanceId(const FGuid& InstanceId) { DimensionInstanceId = InstanceId; }

	// Get dimension instance ID
	UFUNCTION(BlueprintPure, Category="Save|Dimension")
	FGuid GetDimensionInstanceId() const { return DimensionInstanceId; }

	// Check if actor belongs to a dimension
	UFUNCTION(BlueprintPure, Category="Save|Dimension")
	bool BelongsToDimension() const { return DimensionInstanceId.IsValid(); }

	// Static helper to find an actor by GUID in the world
	UFUNCTION(BlueprintCallable, Category="Save")
	static AActor* FindActorByGuid(UWorld* World, const FGuid& ActorId);
	
	// Find actor by GUID with metadata fallback (for when GUIDs are regenerated)
	// First tries GUID match, then falls back to matching by class path and original transform
	static AActor* FindActorByGuidOrMetadata(
		UWorld* World, 
		const FGuid& ActorId,
		const FString& ActorClassPath,
		const FTransform& OriginalTransform,
		float TransformTolerance = 10.0f
	);
};

