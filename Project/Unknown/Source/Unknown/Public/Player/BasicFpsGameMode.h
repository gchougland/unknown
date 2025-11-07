// Basic FPS GameMode to wire default pawn/controller
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "BasicFpsGameMode.generated.h"

UCLASS()
class UNKNOWN_API ABasicFpsGameMode : public AGameModeBase
{
	GENERATED_BODY()
public:
	ABasicFpsGameMode(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
};
