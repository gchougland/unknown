#include "UI/PauseMenuWidget.h"
#include "UI/MainMenuButtonWidget.h"
#include "UI/SaveSlotMenuWidget.h"
#include "UI/ConfirmationMenuWidget.h"
#include "UI/SaveNameInputWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "Save/SaveSystemSubsystem.h"
#include "Player/FirstPersonPlayerController.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Misc/DateTime.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Framework/Application/SlateApplication.h"
#include "Input/Events.h"
#include "Input/Reply.h"

UPauseMenuWidget::UPauseMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PendingOperation(EPendingOperation::None)
	, PendingSlotId(TEXT(""))
{
}

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		// Create terminal-styled container
		UBorder* Container = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
			WidgetTree,
			ProjectStyle::GetTerminalWhite(),
			ProjectStyle::GetTerminalBlack(),
			FMargin(2.f),
			FMargin(24.f, 20.f)
		);

		if (Container)
		{
			WidgetTree->RootWidget = Container;

			// Get the inner border (which has the black background)
			// CreateTerminalBorderedPanel sets the inner border as the content of the outer border
			UWidget* InnerBorderWidget = Container->GetContent();
			UBorder* InnerBorder = Cast<UBorder>(InnerBorderWidget);
			
			// Create vertical box for content
			UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
			if (VBox && InnerBorder)
			{
				// Set VBox as content of inner border (not outer border)
				// This preserves the black background from the inner border
				InnerBorder->SetContent(VBox);

				// Create "Paused" title
				TitleText = TerminalWidgetHelpers::CreateTerminalTextBlock(
					WidgetTree,
					TEXT("Paused"),
					24,
					ProjectStyle::GetTerminalWhite(),
					TEXT("TitleText")
				);

				if (TitleText)
				{
					if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText))
					{
						TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
						TitleSlot->SetHorizontalAlignment(HAlign_Center);
					}
				}

				// Create vertical box for buttons
				UVerticalBox* ButtonBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ButtonBox"));
				if (ButtonBox)
				{
					if (UVerticalBoxSlot* ButtonBoxSlot = VBox->AddChildToVerticalBox(ButtonBox))
					{
						ButtonBoxSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
					}

					// Resume button
					ResumeButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ResumeButton"));
					if (ResumeButton)
					{
						ResumeButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnResumeClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ResumeButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}

					// Save button
					SaveButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("SaveButton"));
					if (SaveButton)
					{
						SaveButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSaveClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(SaveButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}

					// Load button
					LoadButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("LoadButton"));
					if (LoadButton)
					{
						LoadButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnLoadClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(LoadButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}

					// Settings button
					SettingsButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("SettingsButton"));
					if (SettingsButton)
					{
						SettingsButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnSettingsClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(SettingsButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}

					// Exit to Main Menu button
					ExitToMainMenuButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ExitToMainMenuButton"));
					if (ExitToMainMenuButton)
					{
						ExitToMainMenuButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnExitToMainMenuClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ExitToMainMenuButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}

					// Exit to Desktop button
					ExitToDesktopButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ExitToDesktopButton"));
					if (ExitToDesktopButton)
					{
						ExitToDesktopButton->OnClicked.AddDynamic(this, &UPauseMenuWidget::OnExitToDesktopClicked);
						if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ExitToDesktopButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f));
							ButtonSlot->SetSize(ESlateSizeRule::Automatic); // Size based on content, not fill
						}
					}
				}
			}
		}

		// Create save system menus
		CreateSaveSystemMenus();
	}

	return Super::RebuildWidget();
}

void UPauseMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set button text
	if (ResumeButton)
	{
		ResumeButton->SetButtonText(TEXT("Resume"));
	}
	if (SaveButton)
	{
		SaveButton->SetButtonText(TEXT("Save"));
	}
	if (LoadButton)
	{
		LoadButton->SetButtonText(TEXT("Load"));
	}
	if (SettingsButton)
	{
		SettingsButton->SetButtonText(TEXT("Settings"));
	}
	if (ExitToMainMenuButton)
	{
		ExitToMainMenuButton->SetButtonText(TEXT("Main Menu"));
	}
	if (ExitToDesktopButton)
	{
		ExitToDesktopButton->SetButtonText(TEXT("Quit"));
	}

	// Center the widget on screen - positioning is set in FirstPersonPlayerControllerUI after AddToViewport
	// Initially hidden
	SetVisibility(ESlateVisibility::Collapsed);
	
	// Make widget focusable to receive keyboard input
	SetIsFocusable(true);
}

FReply UPauseMenuWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Handle Escape key when pause menu is visible (when game is paused)
	// Enhanced Input might not fire when paused, so handle it directly in the widget
	if (InKeyEvent.GetKey() == EKeys::Escape && GetVisibility() == ESlateVisibility::Visible)
	{
		// Check if we're in a sub-menu first
		if (SaveSlotMenu && SaveSlotMenu->GetVisibility() == ESlateVisibility::Visible)
		{
			// Let the sub-menu handle it (back button)
			return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
		}
		
		if (ConfirmationMenu && ConfirmationMenu->GetVisibility() == ESlateVisibility::Visible)
		{
			// Let the confirmation menu handle it (cancel)
			return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
		}
		
		if (SaveNameInput && SaveNameInput->GetVisibility() == ESlateVisibility::Visible)
		{
			// Let the name input handle it (cancel)
			return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
		}
		
		// Escape pressed in main pause menu - unpause
		Hide();
		return FReply::Handled();
	}
	
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPauseMenuWidget::Show()
{
	SetVisibility(ESlateVisibility::Visible);
	ShowMainMenu(); // Show the main pause menu panel
	
	// Ensure widget is focusable and set keyboard focus so it can receive Escape key input
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void UPauseMenuWidget::Hide()
{
	// Get the owning player controller first
	APlayerController* PC = GetOwningPlayer();
	AFirstPersonPlayerController* FirstPersonPC = Cast<AFirstPersonPlayerController>(PC);
	
	if (PC)
	{
		// Unpause the game first (before hiding menu)
		PC->SetPause(false);
		
		// Set input mode back to game only
		FInputModeGameOnly InputMode;
		PC->SetInputMode(InputMode);
		PC->bShowMouseCursor = false;
		
		// Set flag to prevent immediate re-pausing (input buffering issue)
		// This flag will be checked in OnPausePressed when trying to pause again
		// Clear it after a short delay to allow normal pause behavior
		if (FirstPersonPC)
		{
			FirstPersonPC->bIsUnpausing = true;
			
			// Clear the flag after a small delay to prevent buffered Escape input from immediately re-pausing
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick([FirstPersonPC]()
				{
					if (FirstPersonPC)
					{
						FirstPersonPC->bIsUnpausing = false;
					}
				});
			}
		}
	}
	
	// Hide the menu after unpausing
	SetVisibility(ESlateVisibility::Collapsed);
	
	// Hide sub-menus without showing main menu
	if (SaveSlotMenu)
	{
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ConfirmationMenu)
	{
		ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveNameInput)
	{
		SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::Toggle()
{
	if (IsPauseMenuVisible())
	{
		Hide();
	}
	else
	{
		Show();
	}
}

bool UPauseMenuWidget::IsPauseMenuVisible() const
{
	return GetVisibility() == ESlateVisibility::Visible;
}

void UPauseMenuWidget::OnResumeClicked()
{
	// Hide the pause menu (which will unpause the game and restore input)
	Hide();
}

void UPauseMenuWidget::OnSaveClicked()
{
	PendingOperation = EPendingOperation::Save;
	ShowSaveSlotMenu();
}

void UPauseMenuWidget::OnLoadClicked()
{
	PendingOperation = EPendingOperation::Load;
	ShowSaveSlotMenu();
}

void UPauseMenuWidget::OnSettingsClicked()
{
	UE_LOG(LogTemp, Display, TEXT("[PauseMenu] Settings clicked"));
	// TODO: Implement settings menu
}

void UPauseMenuWidget::OnExitToMainMenuClicked()
{
	ShowConfirmationMenu(TEXT("Are you sure you want to exit to the main menu? Unsaved progress will be lost."));
	PendingOperation = EPendingOperation::ExitToMainMenu;
}

void UPauseMenuWidget::OnExitToDesktopClicked()
{
	ShowConfirmationMenu(TEXT("Are you sure you want to exit to desktop? Unsaved progress will be lost."));
	PendingOperation = EPendingOperation::ExitToDesktop;
}

void UPauseMenuWidget::CreateSaveSystemMenus()
{
	if (!WidgetTree)
	{
		return;
	}

	// Get the player controller - same pattern as FirstPersonPlayerControllerUI
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PauseMenuWidget] Failed to get player controller for SaveSlotMenu"));
		return;
	}

	// Create save slot menu - use PlayerController directly (same as PauseMenuWidget pattern in FirstPersonPlayerControllerUI)
	SaveSlotMenu = CreateWidget<USaveSlotMenuWidget>(PC, USaveSlotMenuWidget::StaticClass());
	if (SaveSlotMenu)
	{
		SaveSlotMenu->AddToViewport(2500); // Higher than pause menu (2000), lower than confirmation/name input (3000)
		
		// Verify widget is actually in viewport before setting anchors
		if (!SaveSlotMenu->IsInViewport())
		{
			UE_LOG(LogTemp, Error, TEXT("[PauseMenuWidget] SaveSlotMenu AddToViewport() failed - widget not in viewport!"));
		}
		
		// Center on screen - MUST be set AFTER AddToViewport
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		SaveSlotMenu->SetDesiredSizeInViewport(FVector2D(400.f, 500.f));
		SaveSlotMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		SaveSlotMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		SaveSlotMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		// Verify anchors were set
		FAnchors CheckAnchors = SaveSlotMenu->GetAnchorsInViewport();
		if (CheckAnchors.Minimum.X == 0.f && CheckAnchors.Minimum.Y == 0.f && 
		    CheckAnchors.Maximum.X == 0.f && CheckAnchors.Maximum.Y == 0.f)
		{
			UE_LOG(LogTemp, Error, TEXT("[PauseMenuWidget] SetAnchorsInViewport FAILED - anchors still (0,0,0,0) after setting!"));
		}
		else
		{
			UE_LOG(LogTemp, Display, TEXT("[PauseMenuWidget] SaveSlotMenu anchors set successfully: (%.2f,%.2f,%.2f,%.2f)"), 
				CheckAnchors.Minimum.X, CheckAnchors.Minimum.Y, CheckAnchors.Maximum.X, CheckAnchors.Maximum.Y);
		}
		
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
		SaveSlotMenu->OnBackClicked.AddDynamic(this, &UPauseMenuWidget::OnSaveSlotMenuBack);
		SaveSlotMenu->OnSaveButtonClicked.AddDynamic(this, &UPauseMenuWidget::OnSaveButtonClicked);
		SaveSlotMenu->OnLoadButtonClicked.AddDynamic(this, &UPauseMenuWidget::OnLoadButtonClicked);
		SaveSlotMenu->OnCreateNewSlotClicked.AddDynamic(this, &UPauseMenuWidget::OnCreateNewSlotClicked);
		SaveSlotMenu->OnDeleteSlotButtonClicked.AddDynamic(this, &UPauseMenuWidget::OnDeleteSlotClicked);
	}

	// Create confirmation menu (high Z-order to appear above everything) - use PlayerController directly
	ConfirmationMenu = CreateWidget<UConfirmationMenuWidget>(PC, UConfirmationMenuWidget::StaticClass());
	if (ConfirmationMenu)
	{
		ConfirmationMenu->AddToViewport(3000); // Higher than pause menu (2000)
		
		// Center on screen - MUST be set AFTER AddToViewport
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		ConfirmationMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		ConfirmationMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		ConfirmationMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmationMenu->OnConfirmed.AddDynamic(this, &UPauseMenuWidget::OnConfirmationConfirmed);
		ConfirmationMenu->OnCancelled.AddDynamic(this, &UPauseMenuWidget::OnConfirmationCancelled);
	}

	// Create save name input (high Z-order to appear above everything) - use PlayerController directly
	SaveNameInput = CreateWidget<USaveNameInputWidget>(PC, USaveNameInputWidget::StaticClass());
	if (SaveNameInput)
	{
		SaveNameInput->AddToViewport(3000); // Higher than pause menu (2000)
		
		// Center on screen - MUST be set AFTER AddToViewport
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		SaveNameInput->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		SaveNameInput->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		SaveNameInput->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
		SaveNameInput->OnNameEntered.AddDynamic(this, &UPauseMenuWidget::OnSaveNameEntered);
		SaveNameInput->OnCancelled.AddDynamic(this, &UPauseMenuWidget::OnSaveNameCancelled);
	}
}

void UPauseMenuWidget::OnSaveSlotMenuBack()
{
	ShowMainMenu();
}

void UPauseMenuWidget::OnSaveButtonClicked()
{
	if (!SaveSlotMenu)
	{
		return;
	}

	// Get selected slot from menu
	FString SlotId = SaveSlotMenu->GetSelectedSlot();
	if (SlotId.IsEmpty())
	{
		return;
	}

	PendingSlotId = SlotId;

	// Check if slot exists
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
		{
			if (SaveSystem->DoesSaveSlotExist(SlotId))
			{
				// Show overwrite confirmation
				ShowConfirmationMenu(TEXT("Are you sure you want to overwrite this save?"));
				PendingOperation = EPendingOperation::Overwrite;
				SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
				return;
			}
		}
	}

	// New slot - show name input
	ShowSaveNameInput();
	SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
}

void UPauseMenuWidget::OnLoadButtonClicked()
{
	if (!SaveSlotMenu)
	{
		return;
	}

	// Get selected slot from menu
	FString SlotId = SaveSlotMenu->GetSelectedSlot();
	if (SlotId.IsEmpty())
	{
		return;
	}

	PendingSlotId = SlotId;
	PendingOperation = EPendingOperation::Load;

	// Show load confirmation
	ShowConfirmationMenu(TEXT("Are you sure you want to load this save? Unsaved progress will be lost."));
	SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
}

void UPauseMenuWidget::OnSaveNameEntered(const FString& SaveName)
{
	if (PendingSlotId.IsEmpty())
	{
		return;
	}

	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
		{
			SaveSystem->SaveGame(PendingSlotId, SaveName);
			
			// Refresh save list after saving so new saves appear
			if (SaveSlotMenu)
			{
				SaveSlotMenu->RefreshSaveList();
			}
		}
	}

	SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
	ShowMainMenu();
	PendingSlotId = TEXT("");
	PendingOperation = EPendingOperation::None;
}

void UPauseMenuWidget::OnSaveNameCancelled()
{
	SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
	ShowMainMenu();
	PendingSlotId = TEXT("");
	PendingOperation = EPendingOperation::None;
}

void UPauseMenuWidget::OnCreateNewSlotClicked()
{
	// Generate a new slot ID (use timestamp-based ID)
	FDateTime Now = FDateTime::Now();
	PendingSlotId = FString::Printf(TEXT("slot_%lld"), Now.ToUnixTimestamp());
	
	// Show save name input
	ShowSaveNameInput();
	SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	PendingOperation = EPendingOperation::Save;
}

void UPauseMenuWidget::OnDeleteSlotClicked()
{
	if (PendingSlotId.IsEmpty())
	{
		// Get selected slot from menu
		if (SaveSlotMenu)
		{
			PendingSlotId = SaveSlotMenu->GetSelectedSlot();
		}
	}
	
	if (!PendingSlotId.IsEmpty())
	{
		// Show delete confirmation
		ShowConfirmationMenu(TEXT("Are you sure you want to delete this save?"));
		PendingOperation = EPendingOperation::Delete;
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::OnConfirmationConfirmed()
{
	ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);

	if (PendingOperation == EPendingOperation::Load)
	{
		// Load the game
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				SaveSystem->LoadGame(PendingSlotId);
				Hide(); // Close pause menu after loading
			}
		}
		PendingSlotId = TEXT("");
		PendingOperation = EPendingOperation::None;
	}
		else if (PendingOperation == EPendingOperation::Overwrite)
		{
			// Overwrite save - get save name from existing save
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
				{
					FSaveSlotInfo Info = SaveSystem->GetSaveSlotInfo(PendingSlotId);
					SaveSystem->SaveGame(PendingSlotId, Info.SaveName);
					
					// Refresh save list after saving
					if (SaveSlotMenu)
					{
						SaveSlotMenu->RefreshSaveList();
					}
				}
			}
			PendingSlotId = TEXT("");
			PendingOperation = EPendingOperation::None;
			ShowMainMenu();
		}
		else if (PendingOperation == EPendingOperation::Delete)
		{
			// Delete the save
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
				{
					SaveSystem->DeleteSave(PendingSlotId);
					// Refresh save list
					if (SaveSlotMenu)
					{
						SaveSlotMenu->RefreshSaveList();
					}
				}
			}
			PendingSlotId = TEXT("");
			PendingOperation = EPendingOperation::None;
			ShowMainMenu();
		}
		else if (PendingOperation == EPendingOperation::ExitToMainMenu)
		{
			// Exit to main menu
			UWorld* World = GetWorld();
			if (World)
			{
				UGameplayStatics::OpenLevel(World, TEXT("MainMenu"));
			}
			PendingOperation = EPendingOperation::None;
		}
		else if (PendingOperation == EPendingOperation::ExitToDesktop)
		{
			// Exit to desktop
			UWorld* World = GetWorld();
			if (World)
			{
				APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
				if (PC)
				{
					UKismetSystemLibrary::QuitGame(World, PC, EQuitPreference::Quit, false);
				}
			}
			PendingOperation = EPendingOperation::None;
		}
}

void UPauseMenuWidget::OnConfirmationCancelled()
{
	ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
	
	// Return to appropriate menu based on pending operation
	if (PendingOperation == EPendingOperation::Overwrite || PendingOperation == EPendingOperation::Load || PendingOperation == EPendingOperation::Delete)
	{
		// Return to save slot menu (came from there)
		// First show the pause menu, then show the save slot menu on top
		SetVisibility(ESlateVisibility::Visible);
		if (SaveSlotMenu)
		{
			SaveSlotMenu->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// If no save slot menu, show main pause menu
			ShowMainMenu();
		}
	}
	else
	{
		// For other operations (exit to main menu/desktop), return to main pause menu
		SetVisibility(ESlateVisibility::Visible);
		ShowMainMenu();
	}
	
	PendingSlotId = TEXT("");
	PendingOperation = EPendingOperation::None;
}

void UPauseMenuWidget::ShowMainMenu()
{
	// Show the pause menu itself
	SetVisibility(ESlateVisibility::Visible);
	
	// Hide sub-menus
	if (SaveSlotMenu)
	{
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ConfirmationMenu)
	{
		ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (SaveNameInput)
	{
		SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::ShowSaveSlotMenu()
{
	if (!SaveSlotMenu)
	{
		return;
	}

	// Set mode based on pending operation
	if (PendingOperation == EPendingOperation::Save)
	{
		SaveSlotMenu->SetMode(ESaveSlotMenuMode::Save);
	}
	else if (PendingOperation == EPendingOperation::Load)
	{
		SaveSlotMenu->SetMode(ESaveSlotMenuMode::LoadGame);
	}

	SaveSlotMenu->RefreshSaveList();
	
	// Re-apply anchors after RefreshSaveList() in case it triggered a rebuild
	// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
	SaveSlotMenu->SetDesiredSizeInViewport(FVector2D(400.f, 500.f));
	SaveSlotMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SaveSlotMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
	// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
	SaveSlotMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
	
	SaveSlotMenu->SetVisibility(ESlateVisibility::Visible);
}

void UPauseMenuWidget::ShowConfirmationMenu(const FString& Message)
{
	if (ConfirmationMenu)
	{
		ConfirmationMenu->SetMessage(Message);
		ConfirmationMenu->SetVisibility(ESlateVisibility::Visible);
		
		// Hide the main pause menu
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UPauseMenuWidget::ShowSaveNameInput(const FString& DefaultName)
{
	if (SaveNameInput)
	{
		SaveNameInput->SetDefaultName(DefaultName);
		SaveNameInput->SetVisibility(ESlateVisibility::Visible);
		
		// Hide the main pause menu
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

