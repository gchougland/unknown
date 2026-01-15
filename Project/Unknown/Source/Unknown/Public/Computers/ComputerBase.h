#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ComputerBase.generated.h"

class APlayerController;
class UCameraComponent;
class UStaticMeshComponent;
class UBoxComponent;
class UWidgetComponent;

/**
 * Base class for computer actors that can be interacted with.
 * Provides camera transition and control scheme switching functionality.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API AComputerBase : public AActor
{
	GENERATED_BODY()

public:
	AComputerBase(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Camera component for blending player view to computer view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Computer")
	TObjectPtr<UCameraComponent> ComputerCamera;

	// Screen mesh displaying render target
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Computer")
	TObjectPtr<UStaticMeshComponent> ScreenMesh;

	// Interaction collider
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Computer")
	TObjectPtr<UBoxComponent> InteractionCollider;

	// World space widget for crosshair overlay
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Computer")
	TObjectPtr<UWidgetComponent> CrosshairWidget;

	// Enter computer mode - transitions camera and switches controls
	UFUNCTION(BlueprintCallable, Category="Computer")
	virtual void EnterComputerMode(APlayerController* PC);

	// Exit computer mode - restores camera and controls
	UFUNCTION(BlueprintCallable, Category="Computer")
	virtual void ExitComputerMode(APlayerController* PC);

	// Setup computer-specific controls (override in derived classes)
	UFUNCTION(BlueprintCallable, Category="Computer")
	virtual void SetupComputerControls(APlayerController* PC);

	// Restore player controls (override in derived classes)
	UFUNCTION(BlueprintCallable, Category="Computer")
	virtual void RestorePlayerControls(APlayerController* PC);

	// Camera blend time when entering/exiting computer mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Computer", meta=(ClampMin="0.0"))
	float CameraBlendTime = 0.5f;

	// Reference to the player controller currently using this computer
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Computer")
	TWeakObjectPtr<APlayerController> CurrentPlayerController;

protected:
	// Store original view target to restore on exit
	UPROPERTY()
	TWeakObjectPtr<AActor> OriginalViewTarget;
};
