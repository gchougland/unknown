// Basic First Person Player Controller for TDD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

// Forward decl to avoid heavy includes in header
struct FInputActionValue;
class UHotbarWidget;
class UInventoryScreenWidget;
class UItemDefinition;

#include "FirstPersonPlayerController.generated.h"

UCLASS()
class UNKNOWN_API AFirstPersonPlayerController : public APlayerController
{
	GENERATED_BODY()
public:
	AFirstPersonPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Accessors used by tests
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputMappingContext* GetDefaultMappingContext() const { return DefaultMappingContext; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetMoveAction() const { return MoveAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetLookAction() const { return LookAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetJumpAction() const { return JumpAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetCrouchAction() const { return CrouchAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetSprintAction() const { return SprintAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetInteractAction() const { return InteractAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetRotateHeldAction() const { return RotateHeldAction; }
	UFUNCTION(BlueprintPure, Category="Input")
	class UInputAction* GetThrowAction() const { return ThrowAction; }

	// Utility for clamping pitch
	static float ClampPitchDegrees(float Degrees);
	
	// Getter for inventory screen widget (for UI refresh)
	UFUNCTION(BlueprintPure, Category="UI")
	UInventoryScreenWidget* GetInventoryScreen() const { return InventoryScreen; }

protected:
	// APlayerController
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;
	virtual void OnPossess(APawn* InPawn) override;

	// Handlers
	void OnMove(const struct FInputActionValue& Value);
	void OnLook(const struct FInputActionValue& Value);
	void OnJumpPressed(const struct FInputActionValue& Value);
	void OnJumpReleased(const struct FInputActionValue& Value);
	void OnCrouchToggle(const struct FInputActionValue& Value);
	void OnSprintPressed(const struct FInputActionValue& Value);
	void OnSprintReleased(const struct FInputActionValue& Value);
	void OnInteract(const struct FInputActionValue& Value);
	void OnRotateHeldPressed(const struct FInputActionValue& Value);
	void OnRotateHeldReleased(const struct FInputActionValue& Value);
	void OnThrow(const struct FInputActionValue& Value);
	void OnFlashlightToggle(const struct FInputActionValue& Value);
	void OnPickup(const struct FInputActionValue& Value);
	void OnPickupPressed(const struct FInputActionValue& Value);
	void OnPickupOngoing(const struct FInputActionValue& Value);
	void OnPickupReleased(const struct FInputActionValue& Value);
	void OnSpawnItem(const struct FInputActionValue& Value);

	// Ensure mapping context and actions exist and have keys mapped
	void InitializeInputAssetsIfNeeded();

 // UI toggles
	UFUNCTION()
	void ToggleInventory();

	// Blueprint-configurable debug spawn item (for P key)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Debug|Spawn")
	TSoftObjectPtr<class UItemDefinition> DebugSpawnItem;

	// Code-defined Enhanced Input assets
	UPROPERTY()
	TObjectPtr<class UInputMappingContext> DefaultMappingContext;
	UPROPERTY()
	TObjectPtr<class UInputAction> MoveAction;
	UPROPERTY()
	TObjectPtr<class UInputAction> LookAction;
	UPROPERTY()
	TObjectPtr<class UInputAction> JumpAction;
	UPROPERTY()
	TObjectPtr<class UInputAction> CrouchAction;
	UPROPERTY()
	TObjectPtr<class UInputAction> SprintAction;
 // Interaction actions
 UPROPERTY()
 TObjectPtr<class UInputAction> InteractAction;
 UPROPERTY()
 TObjectPtr<class UInputAction> RotateHeldAction;
 UPROPERTY()
 TObjectPtr<class UInputAction> ThrowAction;
 UPROPERTY()
 TObjectPtr<class UInputAction> FlashlightAction;
 UPROPERTY()
 TObjectPtr<class UInputAction> PickupAction;
 UPROPERTY()
 TObjectPtr<class UInputAction> SpawnCrowbarAction;

	// Internal state: whether RMB is currently held for rotating a held object
	bool bRotateHeld = false;

    // UI: highlight widget that draws a 2D border around current interactable
    UPROPERTY()
    TObjectPtr<class UInteractHighlightWidget> InteractHighlightWidget;

    // UI: always-on hotbar widget (left side)
    UPROPERTY()
    TObjectPtr<UHotbarWidget> HotbarWidget;

    // UI: modal inventory screen
    UPROPERTY()
    TObjectPtr<class UInventoryScreenWidget> InventoryScreen;

    // UI: progress bar for hold-to-drop
    UPROPERTY()
    TObjectPtr<class UDropProgressBarWidget> DropProgressBarWidget;

    // Cached UI-open state used to quickly gate inputs without dereferencing widgets
    bool bInventoryUIOpen = false;

    // Hold-to-drop state
    float HoldDropTimer = 0.0f;
    static constexpr float HoldDropDuration = 0.75f;
    bool bIsHoldingDrop = false;
    bool bInstantActionExecuted = false;
};
