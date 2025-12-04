#include "Player/MainMenuCharacter.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

AMainMenuCharacter::AMainMenuCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a camera for rendering the main menu level
	MainMenuCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("MainMenuCamera"));
	MainMenuCamera->SetupAttachment(GetRootComponent());
	MainMenuCamera->bUsePawnControlRotation = false; // Static camera, no rotation control
	MainMenuCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	// Disable movement, gravity, and physics - this is a static menu character
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->DisableMovement();
		Move->SetComponentTickEnabled(false);
		Move->GravityScale = 0.f; // Disable gravity
		Move->SetCanEverAffectNavigation(false);
		Move->bCanWalkOffLedges = false;
		Move->bOrientRotationToMovement = false;
		Move->bUseControllerDesiredRotation = false;
	}
}

