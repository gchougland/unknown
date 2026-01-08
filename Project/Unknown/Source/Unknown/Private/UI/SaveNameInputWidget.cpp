#include "UI/SaveNameInputWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/ProjectStyle.h"
#include "UI/MainMenuButtonWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"

USaveNameInputWidget::USaveNameInputWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

TSharedRef<SWidget> USaveNameInputWidget::RebuildWidget()
{
	// Build the widget tree before Slate is generated
	if (WidgetTree)
	{
		// Create vertical box for content
		UVerticalBox* ContentVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentVBox"));
		if (ContentVBox)
		{
			// Create title text
			TitleText = TerminalWidgetHelpers::CreateTerminalTextBlock(
				WidgetTree,
				TEXT("Enter Save Name"),
				18,
				ProjectStyle::GetTerminalWhite(),
				TEXT("TitleText")
			);

			if (TitleText && ContentVBox)
			{
				if (UVerticalBoxSlot* TitleSlot = ContentVBox->AddChildToVerticalBox(TitleText))
				{
					TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 16.f));
				}
			}

			// Create text input using helper
			NameTextBox = TerminalWidgetHelpers::CreateTerminalEditableTextBox(
				WidgetTree,
				TEXT("Enter save name..."),
				18,
				350.f,  // Width
				40.f    // Height
			);

			if (NameTextBox && ContentVBox)
			{
				// The helper creates a border wrapper, so we need to add that to the VBox
				// Actually, the helper returns the text box, but it's wrapped in a border
				// We need to get the wrapper. Let me create it manually for now.
				UBorder* TextBoxWrapper = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
					WidgetTree,
					ProjectStyle::GetTerminalWhite(),
					ProjectStyle::GetTerminalBlack(),
					FMargin(2.f),
					FMargin(8.f, 4.f),
					TEXT("TextBoxWrapper")
				);

				if (TextBoxWrapper)
				{
					UBorder* TextBoxInner = Cast<UBorder>(TextBoxWrapper->GetContent());
					if (TextBoxInner)
					{
						// Create size box for text box
						USizeBox* TextBoxSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
						if (TextBoxSizeBox)
						{
							TextBoxSizeBox->SetWidthOverride(350.f);
							TextBoxSizeBox->SetHeightOverride(40.f);
							TextBoxSizeBox->SetContent(NameTextBox);
							TextBoxInner->SetContent(TextBoxSizeBox);
						}
					}

					if (UVerticalBoxSlot* TextBoxSlot = ContentVBox->AddChildToVerticalBox(TextBoxWrapper))
					{
						TextBoxSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
					}
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
					ConfirmButton->OnClicked.AddDynamic(this, &USaveNameInputWidget::OnConfirmClicked);
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
					CancelButton->OnClicked.AddDynamic(this, &USaveNameInputWidget::OnCancelClicked);
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
				}
			}

			// Create modal dialog wrapper
			RootBorder = TerminalWidgetHelpers::CreateTerminalModalDialog(
				WidgetTree,
				ContentVBox,
				400.f,  // Width
				200.f,  // Height
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

void USaveNameInputWidget::NativeConstruct()
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

void USaveNameInputWidget::SetDefaultName(const FString& DefaultName)
{
	if (NameTextBox)
	{
		NameTextBox->SetText(FText::FromString(DefaultName));
	}
}

void USaveNameInputWidget::OnConfirmClicked()
{
	if (!NameTextBox)
	{
		return;
	}

	FString EnteredName = NameTextBox->GetText().ToString().TrimStartAndEnd();
	
	if (ValidateName(EnteredName))
	{
		OnNameEntered.Broadcast(EnteredName);
	}
	else
	{
		// Invalid name - could show error message here
		UE_LOG(LogTemp, Warning, TEXT("[SaveNameInput] Invalid save name entered"));
	}
}

void USaveNameInputWidget::OnCancelClicked()
{
	OnCancelled.Broadcast();
}

bool USaveNameInputWidget::ValidateName(const FString& Name) const
{
	// Basic validation: non-empty and reasonable length
	if (Name.IsEmpty())
	{
		return false;
	}

	// Maximum length check (e.g., 50 characters)
	if (Name.Len() > 50)
	{
		return false;
	}

	// Could add more validation here (e.g., invalid characters)

	return true;
}

