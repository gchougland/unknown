// Basic First Person Character for TDD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

class UCameraComponent;
class UPhysicsInteractionComponent;

#include "FirstPersonCharacter.generated.h"

UCLASS()
class UNKNOWN_API AFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AFirstPersonCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Sprint speed accessor for tests
	UFUNCTION(BlueprintPure, Category="Movement")
	float GetSprintSpeed() const { return SprintSpeed; }

	// TDD: declare sprint API (stubbed initially)
	UFUNCTION(BlueprintCallable, Category="Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category="Movement")
	void StopSprint();

	// Toggle crouch behavior (hold-to-toggle style per acceptance criteria)
	UFUNCTION(BlueprintCallable, Category="Movement")
	void ToggleCrouch();

protected:
	virtual void BeginPlay() override;

	// Desired configured speeds
	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float WalkSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float CrouchSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float SprintSpeed = 900.f;

	// Camera used for first-person view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Interaction component used for physics interactions
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPhysicsInteractionComponent> PhysicsInteraction;

	// Remember standing camera Z captured at runtime so uncrouch returns exactly
	UPROPERTY(Transient)
	float StandingCameraZ = 0.f;

	// Internal state used for toggle crouch in unit-test contexts where Crouch/UnCrouch may be no-op
	UPROPERTY(Transient)
	bool bCrouchToggled = false;

	// ACharacter hooks to keep camera height consistent with crouch state
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
};
