#include "UI/SaveSlotMenuWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "UI/MainMenuButtonWidget.h"
#include "UI/SaveSlotEntryWidget.h"
#include "Save/SaveSystemSubsystem.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/ScrollBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

USaveSlotMenuWidget::USaveSlotMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MenuMode(ESaveSlotMenuMode::LoadGame)
	, SelectedSlotId(TEXT(""))
{
}

TSharedRef<SWidget> USaveSlotMenuWidget::RebuildWidget()
{
	// Build the widget tree before Slate is generated
	if (WidgetTree)
	{
		// Create terminal-styled container
		RootBorder = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
			WidgetTree,
			ProjectStyle::GetTerminalWhite(),
			ProjectStyle::GetTerminalBlack(),
			FMargin(2.f),
			FMargin(24.f, 20.f)
		);

		if (RootBorder)
		{
			WidgetTree->RootWidget = RootBorder;

			// Create vertical box for content
			UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
			if (VBox)
			{
				// Get inner border and set VBox as content
				UBorder* InnerBorder = Cast<UBorder>(RootBorder->GetContent());
				if (InnerBorder)
				{
					InnerBorder->SetContent(VBox);
				}

				// Create title text
				TitleText = TerminalWidgetHelpers::CreateTerminalTextBlock(
					WidgetTree,
					TEXT("Select Save Slot"),
					20,
					ProjectStyle::GetTerminalWhite(),
					TEXT("TitleText")
				);

				if (TitleText && VBox)
				{
					if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText))
					{
						TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
					}
				}

				// Create scroll box wrapper manually
				UBorder* ScrollBoxWrapper = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
					WidgetTree,
					ProjectStyle::GetTerminalWhite(),
					ProjectStyle::GetTerminalBlack(),
					FMargin(2.f),
					FMargin(8.f),
					TEXT("ScrollBoxWrapper")
				);

				if (ScrollBoxWrapper && VBox)
				{
					// Create scroll box
					ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("SaveSlotScrollBox"));
					if (ScrollBox)
					{
						UBorder* ScrollInner = Cast<UBorder>(ScrollBoxWrapper->GetContent());
						if (ScrollInner)
						{
							ScrollInner->SetContent(ScrollBox);
						}
					}

					if (UVerticalBoxSlot* ScrollSlot = VBox->AddChildToVerticalBox(ScrollBoxWrapper))
					{
						FSlateChildSize FillSize;
						FillSize.SizeRule = ESlateSizeRule::Fill;
						ScrollSlot->SetSize(FillSize);
						ScrollSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
					}
				}

				// Create horizontal box for action buttons
				UHorizontalBox* ActionButtonHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ActionButtonHBox"));
				if (ActionButtonHBox && VBox)
				{
					// Save button (Save mode only, to the left of New Slot)
					SaveButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("SaveButton"));
					if (SaveButton)
					{
						SaveButton->OnClicked.AddDynamic(this, &USaveSlotMenuWidget::OnSaveButtonClickedHandler);
						if (UHorizontalBoxSlot* ButtonSlot = ActionButtonHBox->AddChildToHorizontalBox(SaveButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
							ButtonSlot->SetSize(ESlateSizeRule::Fill);
						}
					}

					// Load button (Load mode only, to the left of Delete Slot)
					LoadButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("LoadButton"));
					if (LoadButton)
					{
						LoadButton->OnClicked.AddDynamic(this, &USaveSlotMenuWidget::OnLoadButtonClickedHandler);
						if (UHorizontalBoxSlot* ButtonSlot = ActionButtonHBox->AddChildToHorizontalBox(LoadButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
							ButtonSlot->SetSize(ESlateSizeRule::Fill);
						}
					}

					// Create New Slot button (Save mode only)
					CreateNewSlotButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("CreateNewSlotButton"));
					if (CreateNewSlotButton)
					{
						CreateNewSlotButton->OnClicked.AddDynamic(this, &USaveSlotMenuWidget::OnCreateNewSlotButtonClicked);
						if (UHorizontalBoxSlot* ButtonSlot = ActionButtonHBox->AddChildToHorizontalBox(CreateNewSlotButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
							ButtonSlot->SetSize(ESlateSizeRule::Fill);
						}
					}

					// Delete Slot button (Save and Load modes)
					DeleteSlotButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("DeleteSlotButton"));
					if (DeleteSlotButton)
					{
						DeleteSlotButton->OnClicked.AddDynamic(this, &USaveSlotMenuWidget::OnDeleteSlotButtonClickedHandler);
						if (UHorizontalBoxSlot* ButtonSlot = ActionButtonHBox->AddChildToHorizontalBox(DeleteSlotButton))
						{
							ButtonSlot->SetPadding(FMargin(0.f));
							ButtonSlot->SetSize(ESlateSizeRule::Fill);
						}
					}

					if (UVerticalBoxSlot* HBoxSlot = VBox->AddChildToVerticalBox(ActionButtonHBox))
					{
						HBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
					}
				}

				// Create back button
				BackButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("BackButton"));
				if (BackButton && VBox)
				{
					BackButton->OnClicked.AddDynamic(this, &USaveSlotMenuWidget::OnBackButtonClicked);
					if (UVerticalBoxSlot* BackSlot = VBox->AddChildToVerticalBox(BackButton))
					{
						BackSlot->SetPadding(FMargin(0.f));
					}
				}
			}
		}
	}

	return Super::RebuildWidget();
}

void USaveSlotMenuWidget::NativeConstruct()
{
	
	Super::NativeConstruct();

	// Set button text
	if (BackButton)
	{
		BackButton->SetButtonText(TEXT("Back"));
	}
	if (SaveButton)
	{
		SaveButton->SetButtonText(TEXT("Save"));
	}
	if (LoadButton)
	{
		LoadButton->SetButtonText(TEXT("Load"));
	}
	if (CreateNewSlotButton)
	{
		CreateNewSlotButton->SetButtonText(TEXT("New"));
	}
	if (DeleteSlotButton)
	{
		DeleteSlotButton->SetButtonText(TEXT("Delete"));
	}

	// Update title based on mode
	UpdateTitle();
	
	// Update button visibility and state
	UpdateButtonVisibility();
	UpdateDeleteButtonState();
	UpdateActionButtonState();
}

void USaveSlotMenuWidget::SetMode(ESaveSlotMenuMode InMode)
{
	MenuMode = InMode;
	SelectedSlotId = TEXT(""); // Clear selection when mode changes
	UpdateTitle();
	UpdateButtonVisibility();
	UpdateDeleteButtonState();
	UpdateActionButtonState();
	RefreshSaveList();
	
	// Re-apply anchors after RefreshSaveList() in case it triggered a rebuild
	// NOTE: SetPositionInViewport can reset anchors, so set anchors AFTER position
	SetDesiredSizeInViewport(FVector2D(400.f, 500.f));
	SetAlignmentInViewport(FVector2D(0.5f, 0.5f));
	SetPositionInViewport(FVector2D(0.f, 0.f));
	// Set anchors LAST - SetPositionInViewport can reset them, so we set anchors after
	SetAnchorsInViewport(FAnchors(0.5f, 0.5f, 0.5f, 0.5f));
}

void USaveSlotMenuWidget::RefreshSaveList()
{
	if (!WidgetTree || !ScrollBox)
	{
		return;
	}

	// Clear existing entries
	ScrollBox->ClearChildren();

	// Get save system subsystem
	UGameInstance* GameInstance = GetGameInstance();
	if (!GameInstance)
	{
		return;
	}

	USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>();
	if (!SaveSystem)
	{
		return;
	}

	// Get all existing save slots
	TArray<FSaveSlotInfo> SaveSlots = SaveSystem->GetAllSaveSlots();

	// Only show existing saves (no empty slots)
	for (const FSaveSlotInfo& SlotInfo : SaveSlots)
	{
		if (SlotInfo.bExists)
		{
			USaveSlotEntryWidget* Entry = WidgetTree->ConstructWidget<USaveSlotEntryWidget>(USaveSlotEntryWidget::StaticClass());
			if (Entry)
			{
				// Add to scroll box first to ensure widget is constructed
				ScrollBox->AddChild(Entry);
				Entry->OnSlotClicked.AddDynamic(this, &USaveSlotMenuWidget::OnSlotEntryClicked);
				
				// Set slot data after adding to scroll box (widget should be constructed now)
				Entry->SetSlotData(SlotInfo.SlotId, SlotInfo.SaveName, SlotInfo.Timestamp, false);
				
				// Set selection after setting data
				Entry->SetSelected(SlotInfo.SlotId == SelectedSlotId);
			}
		}
	}
}

void USaveSlotMenuWidget::OnBackButtonClicked()
{
	OnBackClicked.Broadcast();
}

void USaveSlotMenuWidget::OnSlotEntryClicked(const FString& SlotId)
{
	// Update selection only (don't trigger action)
	SetSelectedSlot(SlotId);
}

void USaveSlotMenuWidget::SetSelectedSlot(const FString& SlotId)
{
	SelectedSlotId = SlotId;
	
	// Update visual state of all entries
	if (ScrollBox)
	{
		for (int32 i = 0; i < ScrollBox->GetChildrenCount(); ++i)
		{
			if (USaveSlotEntryWidget* Entry = Cast<USaveSlotEntryWidget>(ScrollBox->GetChildAt(i)))
			{
				// Get slot ID from entry (we need to store it or get it from the entry)
				// For now, we'll refresh the list to update selection states
			}
		}
	}
	
	// Refresh to update selection visuals
	RefreshSaveList();
	UpdateDeleteButtonState();
	UpdateActionButtonState();
}

void USaveSlotMenuWidget::OnCreateNewSlotButtonClicked()
{
	OnCreateNewSlotClicked.Broadcast();
}

void USaveSlotMenuWidget::OnDeleteSlotButtonClickedHandler()
{
	if (!SelectedSlotId.IsEmpty())
	{
		OnDeleteSlotButtonClicked.Broadcast();
	}
}

void USaveSlotMenuWidget::UpdateTitle()
{
	if (TitleText)
	{
		if (MenuMode == ESaveSlotMenuMode::NewGame)
		{
			TitleText->SetText(FText::FromString(TEXT("New Game - Select Slot")));
		}
		else if (MenuMode == ESaveSlotMenuMode::LoadGame)
		{
			TitleText->SetText(FText::FromString(TEXT("Load Game")));
		}
		else if (MenuMode == ESaveSlotMenuMode::Save)
		{
			TitleText->SetText(FText::FromString(TEXT("Save Game")));
		}
	}
}

void USaveSlotMenuWidget::UpdateButtonVisibility()
{
	// Save mode: Show Save button and New Slot button
	bool bIsSaveMode = (MenuMode == ESaveSlotMenuMode::Save);
	
	// Load mode: Show Load button and Delete Slot button
	bool bIsLoadMode = (MenuMode == ESaveSlotMenuMode::LoadGame);
	
	// NewGame mode: Show Create New Slot button (no Save/Load buttons)
	bool bIsNewGameMode = (MenuMode == ESaveSlotMenuMode::NewGame);
	
	if (SaveButton)
	{
		SaveButton->SetVisibility(bIsSaveMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (LoadButton)
	{
		LoadButton->SetVisibility(bIsLoadMode ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (CreateNewSlotButton)
	{
		CreateNewSlotButton->SetVisibility((bIsSaveMode || bIsNewGameMode) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (DeleteSlotButton)
	{
		DeleteSlotButton->SetVisibility((bIsSaveMode || bIsLoadMode) ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	
	// Update enabled states
	UpdateDeleteButtonState();
	UpdateActionButtonState();
}

void USaveSlotMenuWidget::UpdateDeleteButtonState()
{
	if (DeleteSlotButton)
	{
		// Enable delete button only if a slot is selected
		bool bCanDelete = !SelectedSlotId.IsEmpty();
		DeleteSlotButton->SetIsEnabled(bCanDelete);
	}
}

void USaveSlotMenuWidget::UpdateActionButtonState()
{
	// Enable save/load buttons only if a slot is selected
	bool bHasSelection = !SelectedSlotId.IsEmpty();
	
	if (SaveButton)
	{
		SaveButton->SetIsEnabled(bHasSelection);
	}
	if (LoadButton)
	{
		LoadButton->SetIsEnabled(bHasSelection);
	}
}

void USaveSlotMenuWidget::OnSaveButtonClickedHandler()
{
	// Only allow save if a slot is selected
	if (!SelectedSlotId.IsEmpty())
	{
		OnSaveButtonClicked.Broadcast();
	}
}

void USaveSlotMenuWidget::OnLoadButtonClickedHandler()
{
	// Only allow load if a slot is selected
	if (!SelectedSlotId.IsEmpty())
	{
		OnLoadButtonClicked.Broadcast();
	}
}

