// Player controller for main menu - shows cursor and displays menu widget
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MainMenuPlayerController.generated.h"

class UMainMenuWidget;

UCLASS()
class UNKNOWN_API AMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMainMenuPlayerController(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;

	// Main menu widget
	UPROPERTY()
	TObjectPtr<UMainMenuWidget> MainMenuWidget;
};

