#include "UI/DropProgressBarWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "UI/ProjectStyle.h"

void UDropProgressBarWidget::SetProgress(float InProgress)
{
	Progress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	Invalidate(EInvalidateWidget::Paint);
}

void UDropProgressBarWidget::SetVisible(bool bInVisible)
{
	if (bVisible != bInVisible)
	{
		bVisible = bInVisible;
		Invalidate(EInvalidateWidget::Paint);
	}
}

int32 UDropProgressBarWidget::NativePaint(const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled) const
{
	int32 CurrentLayer = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	if (!bVisible)
	{
		return CurrentLayer;
	}

	const FVector2D ViewportSize = AllottedGeometry.GetLocalSize();
	const FVector2D Center(ViewportSize.X * 0.5f, ViewportSize.Y * 0.5f);
	
	// Position below crosshair
	const FVector2D BarCenter(Center.X, Center.Y + VerticalOffset);
	const FVector2D HalfSize(Width * 0.5f, Height * 0.5f);
	const FVector2D TopLeft = BarCenter - HalfSize;
	const FVector2D BarSize(Width, Height);

	// Draw outer border (white)
	static const FSlateColorBrush WhiteBrush(ProjectStyle::GetTerminalWhite());
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer + 1,
		AllottedGeometry.ToPaintGeometry(TopLeft, BarSize),
		&WhiteBrush,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalWhite()
	);

	// Draw inner background (black)
	const FVector2D InnerTopLeft = TopLeft + FVector2D(BorderThickness, BorderThickness);
	const FVector2D InnerSize = BarSize - FVector2D(BorderThickness * 2.f, BorderThickness * 2.f);
	static const FSlateColorBrush BlackBrush(ProjectStyle::GetTerminalBlack());
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer + 2,
		AllottedGeometry.ToPaintGeometry(InnerTopLeft, InnerSize),
		&BlackBrush,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalBlack()
	);

	// Draw progress fill (white, left to right)
	if (Progress > 0.0f)
	{
		const float FillWidth = InnerSize.X * Progress;
		const FVector2D FillSize(FillWidth, InnerSize.Y);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			CurrentLayer + 3,
			AllottedGeometry.ToPaintGeometry(InnerTopLeft, FillSize),
			&WhiteBrush,
			ESlateDrawEffect::None,
			ProjectStyle::GetTerminalWhite()
		);
	}

	return CurrentLayer + 3;
}

