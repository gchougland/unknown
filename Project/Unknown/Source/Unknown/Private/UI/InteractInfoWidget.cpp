#include "UI/InteractInfoWidget.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/FoodItemData.h"
#include "UI/ProjectStyle.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWidget.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

void UInteractInfoWidget::SetInteractableItem(AItemPickup* ItemPickup)
{
	if (ItemPickup)
	{
		CurrentItemDef = ItemPickup->GetItemDef();
		CurrentCustomData = ItemPickup->GetItemEntry().CustomData;
	}
	else
	{
		CurrentItemDef = nullptr;
		CurrentCustomData.Empty();
	}
	Invalidate(EInvalidateWidget::PaintAndVolatility);
}

void UInteractInfoWidget::SetPosition(const FVector2D& InHighlightTopLeft, const FVector2D& InHighlightBottomRight)
{
	HighlightTopLeft = InHighlightTopLeft;
	HighlightBottomRight = InHighlightBottomRight;
	Invalidate(EInvalidateWidget::PaintAndVolatility);
}

void UInteractInfoWidget::SetKeybindings(const FString& InInteractKeyName, const FString& InPickupKeyName)
{
	InteractKeyName = InInteractKeyName;
	PickupKeyName = InPickupKeyName;
	Invalidate(EInvalidateWidget::PaintAndVolatility);
}

void UInteractInfoWidget::SetVisible(bool bInVisible)
{
	if (bWidgetVisible != bInVisible)
	{
		bWidgetVisible = bInVisible;
		SetVisibility(bInVisible ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		Invalidate(EInvalidateWidget::PaintAndVolatility);
	}
}

void UInteractInfoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	
	// Recalculate position and size each frame in case viewport size changes
	if (bWidgetVisible && CurrentItemDef)
	{
		CalculateWidgetSize(MyGeometry, WidgetSize);
		CalculateWidgetPosition(MyGeometry, WidgetPosition, bPositionOnRight);
	}
}

int32 UInteractInfoWidget::NativePaint(const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bWidgetVisible || !CurrentItemDef)
	{
		return CurrentLayer;
	}

	// Calculate widget size and position
	FVector2D Size;
	FVector2D Position;
	bool bPosOnRight;
	CalculateWidgetSize(AllottedGeometry, Size);
	CalculateWidgetPosition(AllottedGeometry, Position, bPosOnRight);

	// Draw black background rectangle
	const FSlateBrush* BlackBrush = ProjectStyle::GetTerminalBlackBrush();
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer + 1,
		AllottedGeometry.ToPaintGeometry(Position, Size),
		BlackBrush,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalBlack()
	);
	CurrentLayer += 1;

	// Draw white border (outer rectangle)
	const FSlateBrush* WhiteBrush = ProjectStyle::GetTerminalWhiteBrush();
	const FVector2D BorderSize = Size + FVector2D(BorderThickness * 2.f, BorderThickness * 2.f);
	const FVector2D BorderPosition = Position - FVector2D(BorderThickness, BorderThickness);
	
	// Draw border as lines (top, right, bottom, left)
	const FVector2D TL = BorderPosition;
	const FVector2D TR(BorderPosition.X + BorderSize.X, BorderPosition.Y);
	const FVector2D BR(BorderPosition.X + BorderSize.X, BorderPosition.Y + BorderSize.Y);
	const FVector2D BL(BorderPosition.X, BorderPosition.Y + BorderSize.Y);

	TArray<FVector2D> BorderPoints;
	BorderPoints.Reserve(8);
	BorderPoints.Add(TL); BorderPoints.Add(TR);
	BorderPoints.Add(TR); BorderPoints.Add(BR);
	BorderPoints.Add(BR); BorderPoints.Add(BL);
	BorderPoints.Add(BL); BorderPoints.Add(TL);

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		CurrentLayer + 1,
		AllottedGeometry.ToPaintGeometry(),
		BorderPoints,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalWhite(),
		true,
		BorderThickness
	);
	CurrentLayer += 1;

	// Prepare text content
	const FString ItemName = GetItemName();
	const FString ItemInfo = GetItemSpecificInfo();
	const TArray<FString> Keybindings = GetKeybindings();

	// Get font
	const FSlateFontInfo Font = ProjectStyle::GetMonoFont(FontSize);
	const FLinearColor TextColor = ProjectStyle::GetTerminalWhite();

	// Calculate text positions
	FVector2D TextPosition = Position + FVector2D(Padding, Padding);
	float CurrentY = TextPosition.Y;

	// Draw item name
	if (!ItemName.IsEmpty())
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(TextPosition, Size - FVector2D(Padding * 2.f, 0.f)),
			FText::FromString(ItemName),
			Font,
			ESlateDrawEffect::None,
			TextColor
		);
		
		// Measure text height
		FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ItemName, 0, ItemName.Len(), Font);
		CurrentY += TextSize.Y + SectionSpacing;
	}

	// Draw divider line
	if (!ItemName.IsEmpty() && (!ItemInfo.IsEmpty() || Keybindings.Num() > 0))
	{
		const float DividerY = CurrentY - SectionSpacing + (SectionSpacing - DividerThickness) * 0.5f;
		const FVector2D DividerStart(Position.X + Padding, DividerY);
		const FVector2D DividerEnd(Position.X + Size.X - Padding, DividerY);
		
		TArray<FVector2D> DividerPoints;
		DividerPoints.Add(DividerStart);
		DividerPoints.Add(DividerEnd);
		
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			DividerPoints,
			ESlateDrawEffect::None,
			TextColor,
			false,
			DividerThickness
		);
		CurrentLayer += 1;
		CurrentY += SectionSpacing;
	}

	// Draw item-specific info
	if (!ItemInfo.IsEmpty())
	{
		FSlateDrawElement::MakeText(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(TextPosition.X, CurrentY), Size - FVector2D(Padding * 2.f, 0.f)),
			FText::FromString(ItemInfo),
			Font,
			ESlateDrawEffect::None,
			TextColor
		);
		
		FVector2D TextSize;
		TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ItemInfo, 0, ItemInfo.Len(), Font);
		CurrentY += TextSize.Y + SectionSpacing;
	}

	// Draw divider line before keybindings
	if (!ItemInfo.IsEmpty() && Keybindings.Num() > 0)
	{
		const float DividerY = CurrentY - SectionSpacing + (SectionSpacing - DividerThickness) * 0.5f;
		const FVector2D DividerStart(Position.X + Padding, DividerY);
		const FVector2D DividerEnd(Position.X + Size.X - Padding, DividerY);
		
		TArray<FVector2D> DividerPoints;
		DividerPoints.Add(DividerStart);
		DividerPoints.Add(DividerEnd);
		
		FSlateDrawElement::MakeLines(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(),
			DividerPoints,
			ESlateDrawEffect::None,
			TextColor,
			false,
			DividerThickness
		);
		CurrentLayer += 1;
		CurrentY += SectionSpacing;
	}

	// Draw keybindings
	for (int32 i = 0; i < Keybindings.Num(); ++i)
	{
		const FString& Keybinding = Keybindings[i];
		FSlateDrawElement::MakeText(
			OutDrawElements,
			CurrentLayer + 1,
			AllottedGeometry.ToPaintGeometry(FVector2D(TextPosition.X, CurrentY), Size - FVector2D(Padding * 2.f, 0.f)),
			FText::FromString(Keybinding),
			Font,
			ESlateDrawEffect::None,
			TextColor
		);
		
		FVector2D TextSize;
		TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Keybinding, 0, Keybinding.Len(), Font);
		// Only add spacing if not the last item
		if (i < Keybindings.Num() - 1)
		{
			CurrentY += TextSize.Y + TextLineSpacing;
		}
		else
		{
			CurrentY += TextSize.Y;
		}
	}

	return CurrentLayer + 1;
}

void UInteractInfoWidget::CalculateWidgetSize(const FGeometry& AllottedGeometry, FVector2D& OutSize) const
{
	if (!CurrentItemDef)
	{
		OutSize = FVector2D::ZeroVector;
		return;
	}

	const FSlateFontInfo Font = ProjectStyle::GetMonoFont(FontSize);
	const float MaxWidth = 300.f; // Maximum widget width
	float MaxTextWidth = 0.f;
	float TotalHeight = Padding * 2.f; // Top and bottom padding

	// Measure item name
	const FString ItemName = GetItemName();
	if (!ItemName.IsEmpty())
	{
		FVector2D TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ItemName, 0, ItemName.Len(), Font);
		MaxTextWidth = FMath::Max(MaxTextWidth, FMath::Min(TextSize.X, MaxWidth));
		TotalHeight += TextSize.Y;
	}

	// Add divider if we have more content (matches drawing code which adds SectionSpacing twice)
	const FString ItemInfo = GetItemSpecificInfo();
	const TArray<FString> Keybindings = GetKeybindings();
	if (!ItemName.IsEmpty() && (!ItemInfo.IsEmpty() || Keybindings.Num() > 0))
	{
		// Drawing code does: CurrentY += TextSize.Y + SectionSpacing, then draws divider, then CurrentY += SectionSpacing
		// So we need to add SectionSpacing twice to match
		TotalHeight += SectionSpacing * 2.f;
	}

	// Measure item-specific info
	if (!ItemInfo.IsEmpty())
	{
		FVector2D TextSize;
		TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(ItemInfo, 0, ItemInfo.Len(), Font);
		MaxTextWidth = FMath::Max(MaxTextWidth, FMath::Min(TextSize.X, MaxWidth));
		TotalHeight += TextSize.Y;
	}

	// Add divider before keybindings (matches drawing code which adds SectionSpacing twice)
	if (!ItemInfo.IsEmpty() && Keybindings.Num() > 0)
	{
		// Drawing code does: CurrentY += TextSize.Y + SectionSpacing, then draws divider, then CurrentY += SectionSpacing
		// So we need to add SectionSpacing twice to match
		TotalHeight += SectionSpacing * 2.f;
	}

	// Measure keybindings
	for (const FString& Keybinding : Keybindings)
	{
		FVector2D TextSize;
		TextSize = FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Keybinding, 0, Keybinding.Len(), Font);
		MaxTextWidth = FMath::Max(MaxTextWidth, FMath::Min(TextSize.X, MaxWidth));
		TotalHeight += TextSize.Y + TextLineSpacing;
	}

	// Remove extra spacing after last keybinding
	if (Keybindings.Num() > 0)
	{
		TotalHeight -= TextLineSpacing;
	}

	// Ensure minimum width
	MaxTextWidth = FMath::Max(MaxTextWidth, 150.f);
	
	// Add extra safety margin at the bottom to prevent text from being cut off
	// This accounts for potential rounding errors in text measurement and line height variations
	const float BottomSafetyMargin = 4.f;
	TotalHeight += BottomSafetyMargin;
	
	OutSize = FVector2D(MaxTextWidth + Padding * 2.f, TotalHeight);
}

void UInteractInfoWidget::CalculateWidgetPosition(const FGeometry& AllottedGeometry, FVector2D& OutPosition, bool& bOutPositionOnRight) const
{
	const FVector2D ViewportSize = AllottedGeometry.GetLocalSize();
	const FVector2D HighlightSize = HighlightBottomRight - HighlightTopLeft;
	
	FVector2D Size;
	CalculateWidgetSize(AllottedGeometry, Size);

	// Calculate available space on left and right
	const float SpaceOnLeft = HighlightTopLeft.X;
	const float SpaceOnRight = ViewportSize.X - HighlightBottomRight.X;

	// Position on side with more space
	bOutPositionOnRight = SpaceOnRight >= SpaceOnLeft;

	// Calculate X position (directly adjacent, accounting for border thickness)
	// The border extends BorderThickness pixels outward from Position
	float XPos;
	if (bOutPositionOnRight)
	{
		// Position on right: border should start at highlight right edge
		// So Position.X = HighlightBottomRight.X + BorderThickness
		XPos = HighlightBottomRight.X + BorderThickness;
	}
	else
	{
		// Position on left: border should end at highlight left edge
		// So Position.X + Size.X + BorderThickness = HighlightTopLeft.X
		// Therefore Position.X = HighlightTopLeft.X - Size.X - BorderThickness
		XPos = HighlightTopLeft.X - Size.X - BorderThickness;
	}

	// Calculate Y position (align with highlight rect, accounting for border thickness)
	const float HighlightTop = HighlightTopLeft.Y;
	const float HighlightBottom = HighlightBottomRight.Y;
	
	// Border is drawn at Position - BorderThickness, so to align border top with highlight top:
	// Position.Y - BorderThickness = HighlightTop, therefore Position.Y = HighlightTop + BorderThickness
	float YPos = HighlightTop + BorderThickness;
	
	// Check if widget would go off bottom (accounting for border)
	if (YPos - BorderThickness + Size.Y + BorderThickness * 2.f > ViewportSize.Y)
	{
		// Widget would go off bottom, align bottom border with highlight bottom instead
		// Bottom border is at Position.Y + Size.Y + BorderThickness
		// We want: Position.Y + Size.Y + BorderThickness = HighlightBottom + BorderThickness
		// Therefore: Position.Y = HighlightBottom - Size.Y
		YPos = FMath::Max(BorderThickness, HighlightBottom - Size.Y);
	}
	
	// Ensure widget doesn't go off top (border extends upward by BorderThickness)
	YPos = FMath::Max(BorderThickness, YPos);

	OutPosition = FVector2D(XPos, YPos);
}

FString UInteractInfoWidget::GetItemName() const
{
	if (CurrentItemDef)
	{
		return CurrentItemDef->DisplayName.ToString();
	}
	return FString();
}

FString UInteractInfoWidget::GetItemSpecificInfo() const
{
	if (!CurrentItemDef || !CurrentItemDef->FoodData)
	{
		return FString();
	}

	int32 MaxUses = CurrentItemDef->FoodData->MaxUses;
	if (MaxUses <= 0)
	{
		return FString();
	}

	// Check for uses remaining in custom data, default to MaxUses if not found
	int32 UsesRemaining = MaxUses;
	const FString* UsesStr = CurrentCustomData.Find(TEXT("UsesRemaining"));
	if (UsesStr && !UsesStr->IsEmpty())
	{
		UsesRemaining = FCString::Atoi(**UsesStr);
		// Clamp to valid range
		UsesRemaining = FMath::Clamp(UsesRemaining, 0, MaxUses);
	}

	return FString::Printf(TEXT("Uses Remaining: %d / %d"), UsesRemaining, MaxUses);
}

TArray<FString> UInteractInfoWidget::GetKeybindings() const
{
	TArray<FString> Keybindings;
	
	if (!CurrentItemDef)
	{
		return Keybindings;
	}
	
	if (!InteractKeyName.IsEmpty())
	{
		Keybindings.Add(FString::Printf(TEXT("%s to Grab"), *InteractKeyName));
		Keybindings.Add(FString::Printf(TEXT("Hold %s to Use"), *InteractKeyName));
	}
	
	// Only show pickup/hold keybindings if item can be stored or held
	if (!PickupKeyName.IsEmpty() && (CurrentItemDef->bCanBeStored || CurrentItemDef->bCanBeHeld))
	{
		if (CurrentItemDef->bCanBeStored)
		{
			Keybindings.Add(FString::Printf(TEXT("%s to Pickup"), *PickupKeyName));
		}
		if (CurrentItemDef->bCanBeHeld)
		{
			Keybindings.Add(FString::Printf(TEXT("Hold %s to Hold"), *PickupKeyName));
		}
	}
	
	return Keybindings;
}

