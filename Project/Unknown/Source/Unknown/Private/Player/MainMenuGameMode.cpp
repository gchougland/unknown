#include "Player/MainMenuGameMode.h"
#include "Player/MainMenuCharacter.h"
#include "Player/MainMenuPlayerController.h"

AMainMenuGameMode::AMainMenuGameMode(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DefaultPawnClass = AMainMenuCharacter::StaticClass();
	PlayerControllerClass = AMainMenuPlayerController::StaticClass();
}

