#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScannerStageController.generated.h"

class ADimensionalSignal;
class UNiagaraSystem;
class UNiagaraComponent;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class UBoxComponent;

/**
 * Manages the 3D scanning environment with scene capture, cursor, and dimensional signals.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API AScannerStageController : public AActor
{
	GENERATED_BODY()

public:
	AScannerStageController(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Scene capture component (attached to cursor mesh)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<USceneCaptureComponent2D> SceneCapture;

	// 3D cursor mesh (sphere) - acts as 3D cursor and camera attachment point
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<UStaticMeshComponent> CursorMesh;

	// Inverted black cube background
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<UStaticMeshComponent> BackgroundCube;

	// Multiverse visualization Niagara effect
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<UNiagaraComponent> MultiverseEffect;

	// Bounds volume (1000x1000x1000)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<UBoxComponent> BoundsVolume;

	// Move cursor within bounds
	UFUNCTION(BlueprintCallable, Category="Scanner")
	void MoveCursor(const FVector& DeltaMovement);

	// Get cursor local position
	UFUNCTION(BlueprintPure, Category="Scanner")
	FVector GetCursorLocalPosition() const;

	// Get cursor world position
	UFUNCTION(BlueprintPure, Category="Scanner")
	FVector GetCursorWorldPosition() const;

	// Get signals near a position
	UFUNCTION(BlueprintCallable, Category="Scanner")
	TArray<ADimensionalSignal*> GetSignalsNearPosition(const FVector& WorldPos, float Range) const;

	// Set Niagara capture X parameter (local space)
	UFUNCTION(BlueprintCallable, Category="Scanner")
	void SetNiagaraCaptureX(float LocalX);

	// Signal spawn interval (seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner|Signals", meta=(ClampMin="0.0"))
	float SignalSpawnInterval = 5.0f;

	// Signal lifetime range (min, max in seconds)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner|Signals", meta=(ClampMin="0.0"))
	FVector2D SignalLifetimeRange = FVector2D(10.0f, 20.0f);

	// Target number of active signals
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner|Signals", meta=(ClampMin="1"))
	int32 TargetSignalCount = 3;

	// Signal spawn bounds (relative to actor origin)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner|Signals")
	FBox SignalSpawnBounds = FBox(FVector(-500.0f, -500.0f, -500.0f), FVector(500.0f, 500.0f, 500.0f));

	// Dimensional signal class to spawn
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner|Signals")
	TSubclassOf<ADimensionalSignal> SignalClass;

protected:
	// Spawn a new dimensional signal
	void SpawnDimensionalSignal();

	// Update active signals (spawn new ones if needed, remove expired ones)
	void UpdateActiveSignals();

private:
	// Active signals
	UPROPERTY()
	TArray<TObjectPtr<ADimensionalSignal>> ActiveSignals;

	// Timer for signal spawning
	float SignalSpawnTimer = 0.0f;

	// Clamp cursor position to bounds
	void ClampCursorToBounds();
};
