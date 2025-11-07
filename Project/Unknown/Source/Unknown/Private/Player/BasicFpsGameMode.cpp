#include "Player/BasicFpsGameMode.h"
#include "Player/FirstPersonCharacter.h"
#include "Player/FirstPersonPlayerController.h"

ABasicFpsGameMode::ABasicFpsGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = AFirstPersonCharacter::StaticClass();
	PlayerControllerClass = AFirstPersonPlayerController::StaticClass();
}
