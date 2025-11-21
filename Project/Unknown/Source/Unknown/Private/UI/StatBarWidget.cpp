#include "UI/StatBarWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateColorBrush.h"
#include "UI/ProjectStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/SlateRenderer.h"
#include "Fonts/FontMeasure.h"

void UStatBarWidget::SetValue(float Current, float Max)
{
	CurrentValue = FMath::Max(0.0f, Current);
	MaxValue = FMath::Max(1.0f, Max); // Ensure max is at least 1 to avoid division by zero
	Invalidate(EInvalidateWidget::Paint);
}

void UStatBarWidget::SetLabel(const FString& InLabel)
{
	Label = InLabel;
	Invalidate(EInvalidateWidget::Paint);
}

void UStatBarWidget::SetFillColor(const FLinearColor& InColor)
{
	FillColor = InColor;
	Invalidate(EInvalidateWidget::Paint);
}

void UStatBarWidget::SetVisible(bool bInVisible)
{
	if (bVisible != bInVisible)
	{
		bVisible = bInVisible;
		Invalidate(EInvalidateWidget::Paint);
	}
}

int32 UStatBarWidget::NativePaint(const FPaintArgs& Args,
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

	const FVector2D WidgetSize = AllottedGeometry.GetLocalSize();
	const FVector2D BarSize(Width, Height);
	
	// Calculate progress (clamp to 0-1 for fill, but allow > 1 for display)
	const float Progress = FMath::Clamp(CurrentValue / MaxValue, 0.0f, 1.0f);
	
	// Position bar at top-left of widget
	const FVector2D BarTopLeft(0.f, 0.f);
	const FVector2D InnerTopLeft = BarTopLeft + FVector2D(BorderThickness, BorderThickness);
	const FVector2D InnerSize = BarSize - FVector2D(BorderThickness * 2.f, BorderThickness * 2.f);

	// Draw outer border (white)
	static const FSlateColorBrush WhiteBrush(ProjectStyle::GetTerminalWhite());
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer + 1,
		AllottedGeometry.ToPaintGeometry(BarTopLeft, BarSize),
		&WhiteBrush,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalWhite()
	);

	// Draw inner background (black)
	static const FSlateColorBrush BlackBrush(ProjectStyle::GetTerminalBlack());
	FSlateDrawElement::MakeBox(
		OutDrawElements,
		CurrentLayer + 2,
		AllottedGeometry.ToPaintGeometry(InnerTopLeft, InnerSize),
		&BlackBrush,
		ESlateDrawEffect::None,
		ProjectStyle::GetTerminalBlack()
	);

	// Draw progress fill (colored, left to right) with inset to show black border
	if (Progress > 0.0f)
	{
		const FVector2D FillTopLeft = InnerTopLeft + FVector2D(FillInset, FillInset);
		const FVector2D FillInnerSize = InnerSize - FVector2D(FillInset * 2.f, FillInset * 2.f);
		const float FillWidth = FillInnerSize.X * Progress;
		const FVector2D FillSize(FillWidth, FillInnerSize.Y);
		FSlateColorBrush FillBrush(FillColor);
		FSlateDrawElement::MakeBox(
			OutDrawElements,
			CurrentLayer + 3,
			AllottedGeometry.ToPaintGeometry(FillTopLeft, FillSize),
			&FillBrush,
			ESlateDrawEffect::None,
			FillColor
		);
	}

	// Draw label and value text inside the bar (centered) - use bold font
	FSlateFontInfo FontInfo = ProjectStyle::GetMonoFontBold(FontSize);
	const FLinearColor TextColor = ProjectStyle::GetTerminalWhite();
	
	// Format value text: show "100+" if current >= max, otherwise show current value
	FString ValueText;
	if (CurrentValue >= MaxValue)
	{
		ValueText = FString::Printf(TEXT("%.0f+"), MaxValue);
	}
	else
	{
		ValueText = FString::Printf(TEXT("%.0f"), CurrentValue);
	}
	
	// Format full text: "LABEL VALUE"
	FString FullText = FString::Printf(TEXT("%s %s"), *Label, *ValueText);
	
	// Measure text accurately using font measurement service
	TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	const FVector2D TextSize = FontMeasure->Measure(FullText, FontInfo);
	
	// Calculate text position: centered inside the inner black rectangle
	const FVector2D TextPosition(
		InnerTopLeft.X + (InnerSize.X - TextSize.X) * 0.5f, // Center horizontally
		InnerTopLeft.Y + (InnerSize.Y - TextSize.Y) * 0.5f // Center vertically
	);
	
	FSlateDrawElement::MakeText(
		OutDrawElements,
		CurrentLayer + 4,
		AllottedGeometry.ToPaintGeometry(TextPosition, TextSize),
		FullText,
		FontInfo,
		ESlateDrawEffect::None,
		TextColor
	);

	return CurrentLayer + 4;
}

