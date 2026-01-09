#include "Player/MainMenuPlayerController.h"
#include "UI/MainMenuWidget.h"
#include "UI/LoadingFadeWidget.h"
#include "Blueprint/UserWidget.h"

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

	// Create and add the loading fade widget (fullscreen black overlay)
	if (!LoadingFadeWidget)
	{
		LoadingFadeWidget = CreateWidget<ULoadingFadeWidget>(this, ULoadingFadeWidget::StaticClass());
		if (LoadingFadeWidget)
		{
			LoadingFadeWidget->AddToViewport(3000); // Highest Z-order to appear above everything during loading
			// Fill the entire viewport
			LoadingFadeWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			LoadingFadeWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			LoadingFadeWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			// Start invisible (will fade in/out as needed)
			LoadingFadeWidget->SetOpacity(0.0f);
		}
	}
}

