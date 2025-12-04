#include "UI/MainMenuButtonWidget.h"
#include "UI/ProjectStyle.h"
#include "UI/TerminalWidgetHelpers.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

UMainMenuButtonWidget::UMainMenuButtonWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NormalBackgroundColor = ProjectStyle::GetTerminalBlack();
	PressedBackgroundColor = FLinearColor(0.2f, 0.2f, 0.2f, 1.0f); // Slightly brighter for press effect
}

TSharedRef<SWidget> UMainMenuButtonWidget::RebuildWidget()
{
	// Build the widget tree before Slate is generated
	if (WidgetTree)
	{
		// Create outer border (white outline)
		OuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OuterBorder"));
		if (OuterBorder)
		{
			FSlateBrush WhiteBrush;
			WhiteBrush.DrawAs = ESlateBrushDrawType::Box;
			WhiteBrush.TintColor = ProjectStyle::GetTerminalWhite();
			WhiteBrush.Margin = FMargin(0.f);
			OuterBorder->SetBrush(WhiteBrush);
			OuterBorder->SetPadding(FMargin(2.f));
			WidgetTree->RootWidget = OuterBorder;
		}

		// Create inner border (black background)
		InnerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InnerBorder"));
		if (InnerBorder && OuterBorder)
		{
			FSlateBrush BlackBrush;
			BlackBrush.DrawAs = ESlateBrushDrawType::Box;
			BlackBrush.TintColor = NormalBackgroundColor;
			BlackBrush.Margin = FMargin(0.f);
			InnerBorder->SetBrush(BlackBrush);
			InnerBorder->SetPadding(FMargin(16.f, 10.f)); // Reduced padding between text and button edge
			InnerBorder->SetHorizontalAlignment(HAlign_Fill);
			InnerBorder->SetVerticalAlignment(VAlign_Fill);
			OuterBorder->SetContent(InnerBorder);
		}

		// Create overlay for content (to layer cursor over text)
		ContentOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ContentOverlay"));
		if (ContentOverlay && InnerBorder)
		{
			InnerBorder->SetContent(ContentOverlay);
		}

		// Create horizontal box for cursor and text layout
		UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox"));
		if (HBox && ContentOverlay)
		{
			// Add to overlay, centered
			UPanelSlot* HBoxSlot = ContentOverlay->AddChild(HBox);
			if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HBoxSlot))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Center);
				OverlaySlot->SetVerticalAlignment(VAlign_Center);
			}

			// Create cursor border (left side, initially hidden)
			USizeBox* CursorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CursorSizeBox"));
			if (CursorSizeBox)
			{
				CursorSizeBox->SetWidthOverride(2.f);
				
				CursorBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CursorBorder"));
				if (CursorBorder)
				{
					FSlateBrush CursorBrush;
					CursorBrush.DrawAs = ESlateBrushDrawType::Box;
					CursorBrush.TintColor = ProjectStyle::GetTerminalWhite();
					CursorBrush.Margin = FMargin(0.f);
					CursorBorder->SetBrush(CursorBrush);
					CursorBorder->SetVisibility(ESlateVisibility::Collapsed);
					CursorSizeBox->SetContent(CursorBorder);
				}
				
				// Add cursor to horizontal box
				if (UHorizontalBoxSlot* CursorSlot = HBox->AddChildToHorizontalBox(CursorSizeBox))
				{
					CursorSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f)); // Right padding to separate from text
					CursorSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			// Create text block
			ButtonText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ButtonText"));
			if (ButtonText)
			{
				ButtonText->SetFont(ProjectStyle::GetMonoFont(18));
				ButtonText->SetColorAndOpacity(ProjectStyle::GetTerminalWhite());
				ButtonText->SetJustification(ETextJustify::Center);
				ButtonText->SetText(FText::FromString(TEXT("Button"))); // Default text
				
				// Add text to horizontal box
				if (UHorizontalBoxSlot* TextSlot = HBox->AddChildToHorizontalBox(ButtonText))
				{
					TextSlot->SetVerticalAlignment(VAlign_Center);
					TextSlot->SetSize(ESlateSizeRule::Fill);
				}
			}
		}
	}

	return Super::RebuildWidget();
}

void UMainMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UMainMenuButtonWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsHovered)
	{
		UpdateCursorBlink(InDeltaTime);
	}
}

void UMainMenuButtonWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	bIsHovered = true;
	bCursorVisible = true;
	CursorBlinkTimer = 0.0f;
	if (CursorBorder)
	{
		CursorBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UMainMenuButtonWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsHovered = false;
	if (CursorBorder)
	{
		CursorBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FReply UMainMenuButtonWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPressed = true;
		if (InnerBorder)
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = PressedBackgroundColor;
			Brush.Margin = FMargin(0.f);
			InnerBorder->SetBrush(Brush);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMainMenuButtonWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
	{
		bIsPressed = false;
		if (InnerBorder)
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::Box;
			Brush.TintColor = NormalBackgroundColor;
			Brush.Margin = FMargin(0.f);
			InnerBorder->SetBrush(Brush);
		}
		
		// Trigger click delegate
		OnClicked.Broadcast();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMainMenuButtonWidget::SetButtonText(const FString& Text)
{
	if (ButtonText)
	{
		ButtonText->SetText(FText::FromString(Text));
	}
}

void UMainMenuButtonWidget::UpdateCursorBlink(float DeltaTime)
{
	CursorBlinkTimer += DeltaTime;
	if (CursorBlinkTimer >= CursorBlinkInterval)
	{
		CursorBlinkTimer = 0.0f;
		bCursorVisible = !bCursorVisible;
		if (CursorBorder)
		{
			CursorBorder->SetVisibility(bCursorVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}
}

