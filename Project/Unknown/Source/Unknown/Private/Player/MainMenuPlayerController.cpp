#include "Player/MainMenuPlayerController.h"
#include "UI/MainMenuWidget.h"

AMainMenuPlayerController::AMainMenuPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Enable mouse cursor and click events for menu interaction
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
}

void AMainMenuPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Create and display the main menu widget
	if (!MainMenuWidget)
	{
		MainMenuWidget = CreateWidget<UMainMenuWidget>(this, UMainMenuWidget::StaticClass());
		if (MainMenuWidget)
		{
			MainMenuWidget->AddToViewport();
		}
	}
}

