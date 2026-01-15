#pragma once

#include "CoreMinimal.h"
#include "Computers/ComputerBase.h"
#include "Components/PhysicsObjectSocketComponent.h"
#include "DimensionalScannerComputer.generated.h"

class APlayerController;
class AScannerStageController;
class UInputMappingContext;
class UInputAction;
class UNiagaraSystem;

/**
 * Computer for scanning dimensions using a 3D cursor-controlled scene capture system.
 */
UCLASS(BlueprintType, Blueprintable)
class UNKNOWN_API ADimensionalScannerComputer : public AComputerBase
{
	GENERATED_BODY()

public:
	ADimensionalScannerComputer(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	// Override base class methods
	virtual void EnterComputerMode(APlayerController* PC) override;
	virtual void ExitComputerMode(APlayerController* PC) override;
	virtual void SetupComputerControls(APlayerController* PC) override;
	virtual void RestorePlayerControls(APlayerController* PC) override;

	// Physics socket for dimension cartridges
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Scanner")
	TObjectPtr<UPhysicsObjectSocketComponent> CartridgeSocket;

	// Reference to the scanner stage controller
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner")
	TObjectPtr<AScannerStageController> ScannerStageController;

	// Scan detection range (configurable)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner", meta=(ClampMin="0.0"))
	float ScanDetectionRange = 100.0f;

	// Niagara effect to play when scanning
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner")
	TObjectPtr<UNiagaraSystem> ScanNiagaraEffect;

	// Cursor movement speed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner", meta=(ClampMin="0.0"))
	float CursorMoveSpeed = 100.0f;

	// Scroll wheel sensitivity for forward/backward movement
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Scanner", meta=(ClampMin="0.0"))
	float ScrollSensitivity = 10.0f;

protected:
	// Input actions for computer controls
	UPROPERTY()
	TObjectPtr<UInputMappingContext> ComputerMappingContext;

	UPROPERTY()
	TObjectPtr<UInputAction> ScannerMoveAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ScannerScrollAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ScannerScanAction;

	UPROPERTY()
	TObjectPtr<UInputAction> ComputerExitAction;

	// Input handlers
	void OnScannerMove(const struct FInputActionValue& Value);
	void OnScannerScroll(const struct FInputActionValue& Value);
	void OnScannerScan(const struct FInputActionValue& Value);
	void OnComputerExit(const struct FInputActionValue& Value);

	// Scanner functionality
	void HandleScannerMove(const FVector2D& Input);
	void HandleScannerScroll(float ScrollDelta);
	void PerformScan();
	bool CheckCartridgeInserted() const;
	bool IsCartridgeEmpty() const;
	void UpdateNiagaraParameter(float LocalX);

	// Clear scan flag after cooldown
	void ClearScanFlag();

private:
	// Store reference to default mapping context for restoration
	UPROPERTY()
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	// Flag to prevent double-scanning
	bool bIsScanning = false;

	// Timer handle for scan cooldown
	FTimerHandle ScanCooldownTimerHandle;
};
