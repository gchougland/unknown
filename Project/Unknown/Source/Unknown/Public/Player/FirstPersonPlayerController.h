// Basic First Person Player Controller for TDD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"

// Forward decl to avoid heavy includes in header
struct FInputActionValue;

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

protected:
	// APlayerController
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

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

 // Internal state: whether RMB is currently held for rotating a held object
 bool bRotateHeld = false;
 };
