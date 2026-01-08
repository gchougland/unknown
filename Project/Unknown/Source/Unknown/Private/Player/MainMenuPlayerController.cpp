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
	}
	
	// Re-create widget if it was destroyed (e.g., after level transition)
	if (MainMenuWidget)
	{
		if (!MainMenuWidget->IsInViewport())
		{
			MainMenuWidget->AddToViewport();
		}
		
		// Always re-apply anchors and position AFTER AddToViewport
		// This ensures correct positioning both on initial creation and after returning from level
		// Use a timer to set on next frame to ensure viewport is fully initialized
		if (UWorld* World = GetWorld())
		{
			FTimerHandle TimerHandle;
			World->GetTimerManager().SetTimerForNextTick([this]()
			{
				if (MainMenuWidget && MainMenuWidget->IsInViewport())
				{
					MainMenuWidget->ReapplyPositionAndAnchors();
					// Set keyboard focus after positioning
					MainMenuWidget->SetIsFocusable(true);
					MainMenuWidget->SetKeyboardFocus();
				}
			});
		}
	}
}

