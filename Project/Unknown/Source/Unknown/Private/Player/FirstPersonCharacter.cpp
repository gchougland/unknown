#include "Player/FirstPersonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Components/SpotLightComponent.h"

AFirstPersonCharacter::AFirstPersonCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a simple first-person camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	// Create a flashlight (spotlight) attached to the camera
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	if (Flashlight)
	{
		Flashlight->SetupAttachment(FirstPersonCamera);
		Flashlight->SetRelativeLocation(FVector(10.f, 0.f, -5.f));
		Flashlight->Intensity = 200.f;
		Flashlight->AttenuationRadius = 2000.f;
		Flashlight->InnerConeAngle = 15.f;
		Flashlight->OuterConeAngle = 45.f;
		Flashlight->bUseInverseSquaredFalloff = false; // more controllable for a flashlight feel
		Flashlight->SetUseTemperature(true);
		Flashlight->Temperature = 4000.f;
		Flashlight->SetVisibility(false);
	}

	// Create physics interaction component
	PhysicsInteraction = CreateDefaultSubobject<UPhysicsInteractionComponent>(TEXT("PhysicsInteraction"));

	// Configure default movement speeds per acceptance criteria
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move)
	{
		Move->MaxWalkSpeed = WalkSpeed;            // 600 uu/s
		Move->MaxWalkSpeedCrouched = CrouchSpeed;  // 300 uu/s
		Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

	bUseControllerRotationYaw = true; // typical for FPS
}

void AFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (FirstPersonCamera)
	{
		StandingCameraZ = FirstPersonCamera->GetRelativeLocation().Z;
	}
}

void AFirstPersonCharacter::StartSprint()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = SprintSpeed;
	}
}

void AFirstPersonCharacter::StopSprint()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}
}

void AFirstPersonCharacter::ToggleCrouch()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	// Flip internal toggle state
	bCrouchToggled = !bCrouchToggled;

	// Apply speeds
	if (bCrouchToggled)
	{
		Move->MaxWalkSpeed = CrouchSpeed;
	}
	else
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}

	// If we have a world (PIE/runtime), use engine crouch to reduce capsule and let crouch callbacks adjust camera
	if (GetWorld())
	{
		if (bCrouchToggled)
		{
			Crouch();
		}
		else
		{
			UnCrouch();
		}
	}
}

void AFirstPersonCharacter::ToggleFlashlight()
{
	if (Flashlight)
	{
		Flashlight->SetVisibility(!Flashlight->IsVisible());
	}
}

void AFirstPersonCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (FirstPersonCamera)
	{
		// Compute crouched camera Z from the captured standing Z so we always return to the exact same height
		const float EyeDelta = BaseEyeHeight - CrouchedEyeHeight; // positive value (e.g., 64 -> 32 = 32)
		FVector Rel = FirstPersonCamera->GetRelativeLocation();
		Rel.Z = StandingCameraZ - EyeDelta;
		FirstPersonCamera->SetRelativeLocation(Rel);
	}
}

void AFirstPersonCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (FirstPersonCamera)
	{
		FVector Rel = FirstPersonCamera->GetRelativeLocation();
		Rel.Z = StandingCameraZ;
		FirstPersonCamera->SetRelativeLocation(Rel);
	}
}
