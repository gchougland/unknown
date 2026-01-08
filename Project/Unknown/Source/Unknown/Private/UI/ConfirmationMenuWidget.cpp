#include "UI/ConfirmationMenuWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "UI/MainMenuButtonWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

UConfirmationMenuWidget::UConfirmationMenuWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> UConfirmationMenuWidget::RebuildWidget()
{
	// Build the widget tree before Slate is generated
	if (WidgetTree)
	{
		// Create vertical box for content
		UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
		if (ContentVBox)
		{
			// Create message text
			MessageText = TerminalWidgetHelpers::CreateTerminalTextBlock(
				WidgetTree,
				TEXT("Are you sure?"),
				18,
				ProjectStyle::GetTerminalWhite(),
				TEXT("MessageText")
			);

			if (MessageText && ContentVBox)
			{
				if (UVerticalBoxSlot* MessageSlot = ContentVBox->AddChildToVerticalBox(MessageText))
				{
					MessageSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
					MessageSlot->SetHorizontalAlignment(HAlign_Center);
					// Enable text wrapping
					MessageText->SetAutoWrapText(true);
				}
			}

			// Create horizontal box for buttons
			UHorizontalBox* ButtonHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ButtonHBox"));
			if (ButtonHBox)
			{
				// Create confirm button
				ConfirmButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("ConfirmButton"));
				if (ConfirmButton)
				{
					ConfirmButton->OnClicked.AddDynamic(this, &UConfirmationMenuWidget::OnConfirmClicked);
					if (UHorizontalBoxSlot* ConfirmSlot = ButtonHBox->AddChildToHorizontalBox(ConfirmButton))
					{
						ConfirmSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
						FSlateChildSize FillSize;
						FillSize.SizeRule = ESlateSizeRule::Fill;
						ConfirmSlot->SetSize(FillSize);
					}
				}

				// Create cancel button
				CancelButton = WidgetTree->ConstructWidget<UMainMenuButtonWidget>(UMainMenuButtonWidget::StaticClass(), TEXT("CancelButton"));
				if (CancelButton)
				{
					CancelButton->OnClicked.AddDynamic(this, &UConfirmationMenuWidget::OnCancelClicked);
					if (UHorizontalBoxSlot* CancelSlot = ButtonHBox->AddChildToHorizontalBox(CancelButton))
					{
						CancelSlot->SetPadding(FMargin(0.f));
						FSlateChildSize FillSize;
						FillSize.SizeRule = ESlateSizeRule::Fill;
						CancelSlot->SetSize(FillSize);
					}
				}

				// Add button box to vertical box
				if (UVerticalBoxSlot* ButtonSlot = ContentVBox->AddChildToVerticalBox(ButtonHBox))
				{
					ButtonSlot->SetPadding(FMargin(0.f));
					// Size is auto by default, no need to set it
				}
			}

			// Create modal dialog wrapper
			RootBorder = TerminalWidgetHelpers::CreateTerminalModalDialog(
				WidgetTree,
				ContentVBox,
				400.f,  // Width
				0.f,    // Height - auto based on content
				FMargin(2.f),
				FMargin(20.f, 16.f)
			);

			if (RootBorder)
			{
				WidgetTree->RootWidget = RootBorder;
			}
		}
	}

	return Super::RebuildWidget();
}

void UConfirmationMenuWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// Set button text
	if (ConfirmButton)
	{
		ConfirmButton->SetButtonText(TEXT("Confirm"));
	}
	if (CancelButton)
	{
		CancelButton->SetButtonText(TEXT("Cancel"));
	}

	// Note: Anchors/positioning are set AFTER AddToViewport() in the parent widget
	// (same pattern as PauseMenuWidget - positioning set in parent, not in NativeConstruct)
}

void UConfirmationMenuWidget::SetMessage(const FString& Message)
{
	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
	}
}

void UConfirmationMenuWidget::OnConfirmClicked()
{
	OnConfirmed.Broadcast();
}

void UConfirmationMenuWidget::OnCancelClicked()
{
	OnCancelled.Broadcast();
}

