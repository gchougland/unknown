#include "UI/InteractHighlightWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"

void UInteractHighlightWidget::SetHighlightRect(const FVector2D& InTopLeft, const FVector2D& InBottomRight)
{
    TopLeft = InTopLeft;
    BottomRight = InBottomRight;
    Invalidate(EInvalidateWidget::Paint);
}

void UInteractHighlightWidget::SetVisible(bool bInVisible)
{
    if (bHighlightVisible != bInVisible)
    {
        bHighlightVisible = bInVisible;
        // Do not change the widget's Slate visibility here; this only toggles the highlight rectangle.
        Invalidate(EInvalidateWidget::Paint);
    }
}

int32 UInteractHighlightWidget::NativePaint(const FPaintArgs& Args,
    const FGeometry& AllottedGeometry,
    const FSlateRect& MyCullingRect,
    FSlateWindowElementList& OutDrawElements,
    int32 LayerId,
    const FWidgetStyle& InWidgetStyle,
    bool bParentEnabled) const
{
    int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

    // Always draw crosshair if enabled
    if (bShowCrosshair)
    {
        const FVector2D Size = AllottedGeometry.GetLocalSize();
        const FVector2D Center(Size.X * 0.5f, Size.Y * 0.5f);

        // Draw a small filled square centered at the viewport center
        const FVector2D HalfSize(CrosshairSize, CrosshairSize);
        const FVector2D TopLeftOfBox = Center - HalfSize;
        const FVector2D BoxSize = HalfSize * 2.f;

        // Use a local white color brush and tint it with CrosshairColor
        static const FSlateColorBrush SquareBrush(FLinearColor::White);
        FSlateDrawElement::MakeBox(
            OutDrawElements,
            CurrentLayer + 1,
            AllottedGeometry.ToPaintGeometry(TopLeftOfBox, BoxSize),
            &SquareBrush,
            ESlateDrawEffect::None,
            CrosshairColor
        );
        CurrentLayer += 1;
    }

    // Draw highlight rectangle if requested
    if (bHighlightVisible)
    {
        const FVector2D TL = FVector2D(FMath::Min(TopLeft.X, BottomRight.X), FMath::Min(TopLeft.Y, BottomRight.Y));
        const FVector2D BR = FVector2D(FMath::Max(TopLeft.X, BottomRight.X), FMath::Max(TopLeft.Y, BottomRight.Y));
        const FVector2D TR(BR.X, TL.Y);
        const FVector2D BL(TL.X, BR.Y);

        TArray<FVector2D> LinePoints;
        LinePoints.Reserve(8);
        LinePoints.Add(TL); LinePoints.Add(TR);
        LinePoints.Add(TR); LinePoints.Add(BR);
        LinePoints.Add(BR); LinePoints.Add(BL);
        LinePoints.Add(BL); LinePoints.Add(TL);

        FSlateDrawElement::MakeLines(
            OutDrawElements,
            CurrentLayer + 1,
            AllottedGeometry.ToPaintGeometry(),
            LinePoints,
            ESlateDrawEffect::None,
            BorderColor,
            true,
            Thickness
        );
        CurrentLayer += 1;
    }

    return CurrentLayer;
}
