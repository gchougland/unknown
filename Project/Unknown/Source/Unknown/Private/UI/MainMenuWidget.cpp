#include "UI/MainMenuWidget.h"
#include "UI/MainMenuButtonWidget.h"
#include "UI/SaveSlotMenuWidget.h"
#include "UI/ConfirmationMenuWidget.h"
#include "UI/SaveNameInputWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

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
						// Add bottom padding to create gap between buttons (prevents doubled borders)
						ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f)); // 4px gap below to separate borders
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				ContinueButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ContinueButton"));
				if (ContinueButton)
				{
					ContinueButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnContinueClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ContinueButton))
					{
						ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f)); // 4px gap below to separate borders
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				LoadGameButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("LoadGameButton"));
				if (LoadGameButton)
				{
					LoadGameButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnLoadGameClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(LoadGameButton))
					{
						ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f)); // 4px gap below to separate borders
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				SettingsButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("SettingsButton"));
				if (SettingsButton)
				{
					SettingsButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnSettingsClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(SettingsButton))
					{
						ButtonSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f)); // 4px gap below to separate borders
						ButtonSlot->SetSize(ESlateSizeRule::Fill);
					}
				}

				ExitButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ExitButton"));
				if (ExitButton)
				{
					ExitButton->OnClicked.AddDynamic(this, &UMainMenuWidget::OnExitClicked);
					if (UVerticalBoxSlot* ButtonSlot = ButtonBox->AddChildToVerticalBox(ExitButton))
					{
						ButtonSlot->SetPadding(FMargin(0.f)); // No padding for last button
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

	// Set widget to be focusable so it can receive keyboard input
	SetIsFocusable(true);

	// Set button text after widget tree is built
	if (NewGameButton)
	{
		NewGameButton->SetButtonText(TEXT("New Game"));
	}
	if (ContinueButton)
	{
		ContinueButton->SetButtonText(TEXT("Continue"));
	}
	if (LoadGameButton)
	{
		LoadGameButton->SetButtonText(TEXT("Load Game"));
	}
	if (SettingsButton)
	{
		SettingsButton->SetButtonText(TEXT("Settings"));
	}
	if (ExitButton)
	{
		ExitButton->SetButtonText(TEXT("Exit"));
	}

	// Position and anchors are set AFTER AddToViewport in MainMenuPlayerController::BeginPlay()
	// This ensures they're set correctly both on initial creation and after level transitions

	// Create save system menu widgets
	CreateSaveSystemMenus();
	
	// Re-apply position and anchors after NativeConstruct completes
	// Use a timer to ensure this runs after all initialization
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimer(TimerHandle, [this]()
		{
			ReapplyPositionAndAnchors();
			// Set keyboard focus after positioning
			SetKeyboardFocus();
		}, 0.15f, false);
	}
}

void UMainMenuWidget::ReapplyPositionAndAnchors()
{
	if (!IsInViewport())
	{
		return;
	}
	
	// Set anchors and position to center vertically, keep on left side
	// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
	SetDesiredSizeInViewport(FVector2D(300.f, 400.f));
	SetAlignmentInViewport(FVector2D(0.f, 0.5f)); // Left-aligned, vertically centered
	SetPositionInViewport(FVector2D(50.f, 0.f)); // 50px from left, 0 offset from center (since aligned to center)
	// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
	SetAnchorsInViewport(FAnchors(0.f, 0.5f, 0.f, 0.5f)); // Left side, vertically centered
	
	// Set keyboard focus so buttons can be clicked immediately
	SetIsFocusable(true);
	SetKeyboardFocus();
}

void UMainMenuWidget::OnNewGameClicked()
{
	// New game - immediately open name input (no slot selection needed)
	// Generate a new slot ID
	FDateTime Now = FDateTime::Now();
	PendingSlotId = FString::Printf(TEXT("slot_%lld"), Now.ToUnixTimestamp());
	
	// Show save name input directly
	if (SaveNameInput)
	{
		SaveNameInput->SetVisibility(ESlateVisibility::Visible);
		SetVisibility(ESlateVisibility::Collapsed);
	}
	PendingOperation = EPendingOperation::NewGame;
}

void UMainMenuWidget::OnContinueClicked()
{
	// Load most recent save
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
		{
			FSaveSlotInfo MostRecent = SaveSystem->GetMostRecentSaveSlot();
			if (MostRecent.bExists)
			{
				// Load the most recent save
				SaveSystem->LoadGame(MostRecent.SlotId);
			}
		}
	}
}

void UMainMenuWidget::OnLoadGameClicked()
{
	// Open save slot menu in Load Game mode
	if (SaveSlotMenu)
	{
		SaveSlotMenu->SetMode(ESaveSlotMenuMode::LoadGame);
		// Re-apply anchors AFTER SetMode (SetMode calls RefreshSaveList which might reset anchors)
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		SaveSlotMenu->SetDesiredSizeInViewport(FVector2D(400.f, 500.f));
		SaveSlotMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		SaveSlotMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		SaveSlotMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		SaveSlotMenu->SetVisibility(ESlateVisibility::Visible);
		SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::OnSettingsClicked()
{
	// Scaffold - no implementation yet
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

void UMainMenuWidget::CreateSaveSystemMenus()
{
	// Get the player controller - in main menu, GetOwningPlayer() might not work, so use GetWorld()
	UWorld* World = GetWorld();
	APlayerController* PC = World ? UGameplayStatics::GetPlayerController(World, 0) : nullptr;
	if (!PC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenuWidget] Failed to get player controller for SaveSlotMenu"));
		return;
	}
	
	// Create save slot menu - use PlayerController directly (same as PauseMenuWidget pattern)
	SaveSlotMenu = CreateWidget<USaveSlotMenuWidget>(PC, USaveSlotMenuWidget::StaticClass());
	if (SaveSlotMenu)
	{
		SaveSlotMenu->AddToViewport();
		
		// Center on screen - MUST be set AFTER AddToViewport
		// FAnchors constructor: FAnchors(MinX, MinY, MaxX, MaxY) - all 4 parameters required!
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position, or don't set position at all for center
		SaveSlotMenu->SetDesiredSizeInViewport(FVector2D(400.f, 500.f));
		SaveSlotMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		SaveSlotMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		SaveSlotMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
		SaveSlotMenu->OnBackClicked.AddDynamic(this, &UMainMenuWidget::OnSaveSlotMenuBack);
		SaveSlotMenu->OnSaveSlotSelected.AddDynamic(this, &UMainMenuWidget::OnSaveSlotSelected);
		SaveSlotMenu->OnLoadButtonClicked.AddDynamic(this, &UMainMenuWidget::OnLoadButtonClicked);
		SaveSlotMenu->OnCreateNewSlotClicked.AddDynamic(this, &UMainMenuWidget::OnCreateNewSlotClicked);
		SaveSlotMenu->OnDeleteSlotButtonClicked.AddDynamic(this, &UMainMenuWidget::OnDeleteSlotClicked);
		
		// Don't call RefreshSaveList() here - it might trigger a rebuild that resets anchors
		// RefreshSaveList() will be called when the menu is first shown
	}

	// Create confirmation menu (high Z-order to appear above everything) - use PlayerController directly
	ConfirmationMenu = CreateWidget<UConfirmationMenuWidget>(PC, UConfirmationMenuWidget::StaticClass());
	if (ConfirmationMenu)
	{
		ConfirmationMenu->AddToViewport(3000); // High Z-order
		
		// Center on screen - MUST be set AFTER AddToViewport
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		ConfirmationMenu->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		ConfirmationMenu->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		ConfirmationMenu->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
		ConfirmationMenu->OnConfirmed.AddDynamic(this, &UMainMenuWidget::OnConfirmationConfirmed);
		ConfirmationMenu->OnCancelled.AddDynamic(this, &UMainMenuWidget::OnConfirmationCancelled);
	}

	// Create save name input - use PlayerController directly
	SaveNameInput = CreateWidget<USaveNameInputWidget>(PC, USaveNameInputWidget::StaticClass());
	if (SaveNameInput)
	{
		SaveNameInput->AddToViewport();
		
		// Center on screen - MUST be set AFTER AddToViewport
		// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
		SaveNameInput->SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
		SaveNameInput->SetPositionInViewport(FVector2D(0.f, 0.f));
		// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
		SaveNameInput->SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
		
		SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
		SaveNameInput->OnNameEntered.AddDynamic(this, &UMainMenuWidget::OnSaveNameEntered);
		SaveNameInput->OnCancelled.AddDynamic(this, &UMainMenuWidget::OnSaveNameCancelled);
	}
}

void UMainMenuWidget::OnSaveSlotMenuBack()
{
	// Return to main menu
	if (SaveSlotMenu)
	{
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
	SetVisibility(ESlateVisibility::Visible);
	
	// Re-apply position and anchors when returning to main menu
	// Use a timer to set on next frame to ensure viewport is fully updated
	if (UWorld* World = GetWorld())
	{
		FTimerHandle TimerHandle;
		World->GetTimerManager().SetTimerForNextTick([this]()
		{
			ReapplyPositionAndAnchors();
			// Restore keyboard focus when returning to main menu
			SetKeyboardFocus();
		});
	}
}

void UMainMenuWidget::OnSaveSlotSelected(const FString& SlotId)
{
	if (!SaveSlotMenu)
	{
		return;
	}

	PendingSlotId = SlotId;

	// Check mode
	if (SaveSlotMenu->GetMode() == ESaveSlotMenuMode::NewGame)
	{
		// Check if slot exists
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				if (SaveSystem->DoesSaveSlotExist(SlotId))
				{
					// Overwriting existing save - show confirmation
					if (ConfirmationMenu)
					{
						FSaveSlotInfo Info = SaveSystem->GetSaveSlotInfo(SlotId);
						ConfirmationMenu->SetMessage(FString::Printf(TEXT("Overwrite existing save \"%s\"?"), *Info.SaveName));
						ConfirmationMenu->SetVisibility(ESlateVisibility::Visible);
						SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
					}
					return;
				}
			}
		}

		// New save - open name input
		if (SaveNameInput)
		{
			SaveNameInput->SetVisibility(ESlateVisibility::Visible);
			SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	// Load game mode is handled by OnLoadButtonClicked now
}

void UMainMenuWidget::OnCreateNewSlotClicked()
{
	// Generate a new slot ID
	FDateTime Now = FDateTime::Now();
	PendingSlotId = FString::Printf(TEXT("slot_%lld"), Now.ToUnixTimestamp());
	
	// Show save name input
	if (SaveNameInput)
	{
		SaveNameInput->SetVisibility(ESlateVisibility::Visible);
		SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UMainMenuWidget::OnDeleteSlotClicked()
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
		if (ConfirmationMenu)
		{
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
				{
					FSaveSlotInfo Info = SaveSystem->GetSaveSlotInfo(PendingSlotId);
					ConfirmationMenu->SetMessage(FString::Printf(TEXT("Are you sure you want to delete save \"%s\"?"), *Info.SaveName));
				}
				else
				{
					ConfirmationMenu->SetMessage(TEXT("Are you sure you want to delete this save?"));
				}
			}
			ConfirmationMenu->SetVisibility(ESlateVisibility::Visible);
			SaveSlotMenu->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UMainMenuWidget::OnLoadButtonClicked()
{
	// Get selected slot from menu
	FString SlotId;
	if (SaveSlotMenu)
	{
		SlotId = SaveSlotMenu->GetSelectedSlot();
	}
	
	if (!SlotId.IsEmpty())
	{
		// Load the game
		if (UGameInstance* GameInstance = GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				SaveSystem->LoadGame(SlotId);
			}
		}
	}
}

void UMainMenuWidget::OnSaveNameEntered(const FString& SaveName)
{
	if (!SaveNameInput)
	{
		return;
	}

	SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);

	// Create new save
	if (PendingSlotId.IsEmpty())
	{
		// Generate slot ID
		FDateTime Now = FDateTime::Now();
		PendingSlotId = FString::Printf(TEXT("slot_%lld"), Now.ToUnixTimestamp());
	}
	
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
		{
			// Generate slot ID if it's "new"
			FString SlotId = PendingSlotId;
			if (SlotId == TEXT("new"))
			{
				FDateTime Now = FDateTime::Now();
				SlotId = FString::Printf(TEXT("slot_%lld"), Now.ToUnixTimestamp());
			}
			
			// For New Game, we need to start in MainMap first, then save there
			if (PendingOperation == EPendingOperation::NewGame)
			{
				// Store save info in GameInstance to create save after level loads
				// We'll use a GameMode callback or timer after level loads
				// Create a temp save that we'll restore after level loads
				UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::CreateSaveGameObject(UGameSaveData::StaticClass()));
				if (TempSave)
				{
					TempSave->SaveName = SaveName;
					TempSave->LevelPackagePath = TEXT("/Game/Levels/MainMap");
					FString TempSlotName = FString::Printf(TEXT("_NEWGAME_%s"), *SlotId);
					UGameplayStatics::SaveGameToSlot(TempSave, TempSlotName, 0);
					
					// Start the game in MainMap
					UWorld* World = GetWorld();
					if (World)
					{
						// Open MainMap level - save will be created after level loads
						UGameplayStatics::OpenLevel(World, TEXT("MainMap"));
					}
				}
			}
			else
			{
				// Regular save operation (not new game)
				SaveSystem->SaveGame(SlotId, SaveName);
			}
		}
	}

	// Return to main menu (unless starting new game, in which case we're leaving the menu)
	if (PendingOperation != EPendingOperation::NewGame)
	{
		SetVisibility(ESlateVisibility::Visible);
	}
	PendingSlotId = TEXT("");
	PendingOperation = EPendingOperation::None;
}

void UMainMenuWidget::OnSaveNameCancelled()
{
	if (SaveNameInput)
	{
		SaveNameInput->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Return to appropriate menu based on pending operation
	if (PendingOperation == EPendingOperation::NewGame)
	{
		// Cancelling new game - return to main menu
		SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		// Return to save slot menu
		if (SaveSlotMenu)
		{
			SaveSlotMenu->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// Return to main menu
			SetVisibility(ESlateVisibility::Visible);
		}
	}
	
	PendingSlotId = TEXT("");
	PendingOperation = EPendingOperation::None;
}

void UMainMenuWidget::OnConfirmationConfirmed()
{
	if (!ConfirmationMenu)
	{
		return;
	}

	ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);

	if (!PendingSlotId.IsEmpty())
	{
		// Handle confirmation - overwrite save or delete save
		if (SaveSlotMenu && SaveSlotMenu->GetMode() == ESaveSlotMenuMode::NewGame)
		{
			// Overwrite save
			if (UGameInstance* GameInstance = GetGameInstance())
			{
				if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
				{
					FSaveSlotInfo Info = SaveSystem->GetSaveSlotInfo(PendingSlotId);
					SaveSystem->SaveGame(PendingSlotId, Info.SaveName);
				}
			}
		}
		else
		{
			// Delete save
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
		}
	}

	// Return to main menu
	SetVisibility(ESlateVisibility::Visible);
	PendingSlotId.Empty();
}

void UMainMenuWidget::OnConfirmationCancelled()
{
	if (ConfirmationMenu)
	{
		ConfirmationMenu->SetVisibility(ESlateVisibility::Collapsed);
	}

	// Return to save slot menu
	if (SaveSlotMenu)
	{
		SaveSlotMenu->SetVisibility(ESlateVisibility::Visible);
	}

	PendingSlotId.Empty();
}

