#include "UI/SaveSlotEntryWidget.h"
#include "UI/ProjectStyle.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

USaveSlotEntryWidget::USaveSlotEntryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsEmptySlot(true)
{
}

TSharedRef<SWidget> USaveSlotEntryWidget::RebuildWidget()
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
			BlackBrush.TintColor = ProjectStyle::GetTerminalBlack();
			BlackBrush.Margin = FMargin(0.f);
			InnerBorder->SetBrush(BlackBrush);
			InnerBorder->SetPadding(FMargin(12.f, 8.f));
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

		// Create horizontal box for cursor and content layout
		UHorizontalBox* HBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HBox"));
		if (HBox && ContentOverlay)
		{
			// Add to overlay, fill horizontally, center vertically
			UPanelSlot* HBoxSlot = ContentOverlay->AddChild(HBox);
			if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(HBoxSlot))
			{
				OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
				OverlaySlot->SetVerticalAlignment(VAlign_Center);
			}

			// Create cursor border (left side, initially hidden)
			USizeBox* CursorSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CursorSizeBox"));
			if (CursorSizeBox)
			{
				CursorSizeBox->SetWidthOverride(2.f);
				CursorSizeBox->SetHeightOverride(18.f);
				
				CursorBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CursorBorder"));
				if (CursorBorder)
				{
					FSlateBrush CursorBrush;
					CursorBrush.DrawAs = ESlateBrushDrawType::Box;
					CursorBrush.TintColor = ProjectStyle::GetTerminalWhite();
					CursorBrush.Margin = FMargin(0.f);
					CursorBorder->SetBrush(CursorBrush);
					CursorBorder->SetVisibility(ESlateVisibility::Collapsed);
					CursorBorder->SetVerticalAlignment(VAlign_Fill);
					CursorSizeBox->SetContent(CursorBorder);
				}
				
				// Add cursor to horizontal box
				if (UHorizontalBoxSlot* CursorSlot = HBox->AddChildToHorizontalBox(CursorSizeBox))
				{
					CursorSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
					CursorSlot->SetVerticalAlignment(VAlign_Center);
				}
			}

			// Create overlay for text content to center it vertically
			UOverlay* TextOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("TextOverlay"));
			if (TextOverlay)
			{
				// Create vertical box for slot name and timestamp
				UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
				if (VBox)
				{
					// Slot name text
					SlotNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("SlotNameText"));
					if (SlotNameText)
					{
						SlotNameText->SetFont(ProjectStyle::GetMonoFont(18));
						SlotNameText->SetColorAndOpacity(ProjectStyle::GetTerminalWhite());
						SlotNameText->SetJustification(ETextJustify::Left);
						SlotNameText->SetText(FText::FromString(TEXT("Empty Slot")));
						
						if (UVerticalBoxSlot* NameSlot = VBox->AddChildToVerticalBox(SlotNameText))
						{
							NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
						}
					}

					// Timestamp text
					TimestampText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TimestampText"));
					if (TimestampText)
					{
						TimestampText->SetFont(ProjectStyle::GetMonoFont(12));
						TimestampText->SetColorAndOpacity(ProjectStyle::GetTerminalWhite());
						TimestampText->SetJustification(ETextJustify::Left);
						TimestampText->SetText(FText::GetEmpty());
						
						if (UVerticalBoxSlot* TimestampSlot = VBox->AddChildToVerticalBox(TimestampText))
						{
							TimestampSlot->SetPadding(FMargin(0.f));
						}
					}

					// Add VBox to overlay, centered
					UPanelSlot* VBoxSlot = TextOverlay->AddChild(VBox);
					if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(VBoxSlot))
					{
						OverlaySlot->SetHorizontalAlignment(HAlign_Left);
						OverlaySlot->SetVerticalAlignment(VAlign_Center);
					}
				}

				// Add text overlay to horizontal box
				if (UHorizontalBoxSlot* TextSlot = HBox->AddChildToHorizontalBox(TextOverlay))
				{
					TextSlot->SetVerticalAlignment(VAlign_Center);
					TextSlot->SetSize(ESlateSizeRule::Fill);
				}
			}
		}
	}

	return Super::RebuildWidget();
}

void USaveSlotEntryWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Apply stored slot data after widget is constructed
	// This ensures SlotNameText and TimestampText exist when we set their values
	if (SlotNameText)
	{
		if (bIsEmptySlot)
		{
			SlotNameText->SetText(FText::FromString(TEXT("Empty Slot")));
		}
		else
		{
			SlotNameText->SetText(FText::FromString(SlotName));
		}
	}
	
	if (TimestampText)
	{
		if (bIsEmptySlot)
		{
			TimestampText->SetText(FText::GetEmpty());
		}
		else
		{
			TimestampText->SetText(FText::FromString(Timestamp));
		}
	}
}

void USaveSlotEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsHovered)
	{
		UpdateCursorBlink(InDeltaTime);
	}
}

void USaveSlotEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

void USaveSlotEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	bIsHovered = false;
	if (CursorBorder)
	{
		CursorBorder->SetVisibility(ESlateVisibility::Collapsed);
	}
}

FReply USaveSlotEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bIsPressed = true;
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply USaveSlotEntryWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsPressed)
	{
		bIsPressed = false;
		
		// Trigger click delegate
		OnSlotClicked.Broadcast(SlotId);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void USaveSlotEntryWidget::SetSlotData(const FString& InSlotId, const FString& InSlotName, const FString& InTimestamp, bool bInIsEmpty)
{
	SlotId = InSlotId;
	SlotName = InSlotName;
	Timestamp = InTimestamp;
	bIsEmptySlot = bInIsEmpty;

	if (SlotNameText)
	{
		if (bIsEmptySlot)
		{
			SlotNameText->SetText(FText::FromString(TEXT("Empty Slot")));
		}
		else
		{
			SlotNameText->SetText(FText::FromString(SlotName));
		}
	}

	if (TimestampText)
	{
		if (bIsEmptySlot)
		{
			TimestampText->SetText(FText::GetEmpty());
		}
		else
		{
			TimestampText->SetText(FText::FromString(Timestamp));
		}
	}
}

void USaveSlotEntryWidget::SetSelected(bool bSelected)
{
	bIsSelected = bSelected;
	
	// Update background color to indicate selection
	if (InnerBorder)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.Margin = FMargin(0.f);
		
		if (bSelected)
		{
			// Selected: slightly lighter background
			Brush.TintColor = FLinearColor(0.15f, 0.15f, 0.15f, 1.0f);
		}
		else
		{
			// Normal: black background
			Brush.TintColor = ProjectStyle::GetTerminalBlack();
		}
		
		InnerBorder->SetBrush(Brush);
	}
}

void USaveSlotEntryWidget::UpdateCursorBlink(float DeltaTime)
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

