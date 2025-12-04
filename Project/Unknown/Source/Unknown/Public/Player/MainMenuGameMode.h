// Game mode for main menu level
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MainMenuGameMode.generated.h"

UCLASS()
class UNKNOWN_API AMainMenuGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMainMenuGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};

