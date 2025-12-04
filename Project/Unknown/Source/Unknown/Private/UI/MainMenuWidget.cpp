#include "UI/MainMenuWidget.h"
#include "UI/MainMenuButtonWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"

UMainMenuWidget::UMainMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UMainMenuWidget::RebuildWidget()
{
	// Build the widget tree before Slate is generated
	if (WidgetTree)
	{
		// Create terminal-styled container
		UBorder* Container = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
			WidgetTree,
			ProjectStyle::GetTerminalWhite(),
			ProjectStyle::GetTerminalBlack(),
			FMargin(2.f),
			FMargin(24.f, 20.f) // Padding between container and buttons (more vertical padding)
		);

		if (Container)
		{
			WidgetTree->RootWidget = Container;

			// Create vertical box for buttons
			UVerticalBox* ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonBox"));
			if (ButtonBox)
			{
				Container->SetContent(ButtonBox);

				// Create buttons
				NewGameButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("NewGameButton"));
				if (NewGameButton)
				{
					NewGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnNewGameClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(NewGameButton))
					{
						// Padding creates black space: FMargin(left, top, right, bottom)
						// Left/right padding creates space on sides, top/bottom creates space between buttons
						ButtonSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 8.f)); // 8px on sides, 8px below
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				ContinueButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ContinueButton"));
				if (ContinueButton)
				{
					ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnContinueClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ContinueButton))
					{
						ButtonSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 8.f)); // 8px on sides, 8px below
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				SettingsButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("SettingsButton"));
				if (SettingsButton)
				{
					SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(SettingsButton))
					{
						ButtonSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 8.f)); // 8px on sides, 8px below
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				ExitButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ExitButton"));
				if (ExitButton)
				{
					ExitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnExitClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ExitButton))
					{
						ButtonSlot->SetPadding(FMargin(8.f, 0.f, 8.f, 0.f)); // 8px on sides, no bottom padding for last button
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}
			}
		}
	}

	return Super::RebuildWidget();
}

void UMainMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set button text after widget tree is built
	if (NewGameButton)
	{
		NewGameButton->SetButtonText(TEXT("New Game"));
	}
	if (ContinueButton)
	{
		ContinueButton->SetButtonText(TEXT("Continue"));
	}
	if (SettingsButton)
	{
		SettingsButton->SetButtonText(TEXT("Settings"));
	}
	if (ExitButton)
	{
		ExitButton->SetButtonText(TEXT("Exit"));
	}

	// Position widget on left side of screen with vertical offset
	// Anchor to top-left, but offset down
	SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f)); // Top-left anchor
	SetAlignmentInViewport(FVector2D(0.f, 0.f)); // Top-left aligned
	SetPositionInViewport(FVector2D(50.f, 150.f)); // 50px from left, 150px from top
	SetDesiredSizeInViewport(FVector2D(300.f, 400.f)); // Set a reasonable size
}

void UMainMenuWidget::OnNewGameClicked()
{
	// Scaffold - no implementation yet
	UE_LOG(LogTemp, Display, TEXT("[MainMenu] New Game clicked"));
}

void UMainMenuWidget::OnContinueClicked()
{
	// Scaffold - no implementation yet
	UE_LOG(LogTemp, Display, TEXT("[MainMenu] Continue clicked"));
}

void UMainMenuWidget::OnSettingsClicked()
{
	// Scaffold - no implementation yet
	UE_LOG(LogTemp, Display, TEXT("[MainMenu] Settings clicked"));
}

void UMainMenuWidget::OnExitClicked()
{
	// Exit the game
	UWorld* World = GetWorld();
	if (World)
	{
		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (PC)
		{
			UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
		}
	}
}

