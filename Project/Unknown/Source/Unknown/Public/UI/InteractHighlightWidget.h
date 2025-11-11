// Draws a hollow selection rectangle to highlight the current interactable and a central dot crosshair
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractHighlightWidget.generated.h"

UCLASS()
class UNKNOWN_API UInteractHighlightWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="InteractHighlight")
	void SetHighlightRect(const FVector2D& InTopLeft, const FVector2D& InBottomRight);

	// Toggle only the highlight rectangle visibility; the widget itself stays visible so the crosshair can always render
	UFUNCTION(BlueprintCallable, Category="InteractHighlight")
	void SetVisible(bool bInVisible);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	FVector2D TopLeft = FVector2D::ZeroVector;
	FVector2D BottomRight = FVector2D::ZeroVector;
	bool bHighlightVisible = false;

	// Appearance - selection border
	UPROPERTY(EditAnywhere, Category="Appearance|Highlight")
	FLinearColor BorderColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, Category="Appearance|Highlight")
	float Thickness = 2.f;

	// Appearance - crosshair
	UPROPERTY(EditAnywhere, Category="Appearance|Crosshair")
	bool bShowCrosshair = true;
	UPROPERTY(EditAnywhere, Category="Appearance|Crosshair")
	FLinearColor CrosshairColor = FLinearColor::White;
	// Size in viewport pixels (DPI/viewport-relative due to widget coordinates)
	UPROPERTY(EditAnywhere, Category="Appearance|Crosshair", meta=(ClampMin="1.0"))
	float CrosshairSize = 4.f;
};
