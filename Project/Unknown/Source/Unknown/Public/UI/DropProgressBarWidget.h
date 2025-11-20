// Progress bar widget for hold-to-drop action
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DropProgressBarWidget.generated.h"

UCLASS()
class UNKNOWN_API UDropProgressBarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Set progress value (0.0 to 1.0)
	UFUNCTION(BlueprintCallable, Category="DropProgress")
	void SetProgress(float InProgress);

	// Toggle visibility
	UFUNCTION(BlueprintCallable, Category="DropProgress")
	void SetVisible(bool bInVisible);

	// Check if progress bar is visible
	bool GetIsVisible() const { return bVisible; }

protected:
	virtual int32 NativePaint(const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

private:
	float Progress = 0.0f;
	bool bVisible = false;

	// Appearance
	UPROPERTY(EditAnywhere, Category="Appearance")
	float Width = 200.f;
	UPROPERTY(EditAnywhere, Category="Appearance")
	float Height = 8.f;
	UPROPERTY(EditAnywhere, Category="Appearance")
	float BorderThickness = 2.f;
	UPROPERTY(EditAnywhere, Category="Appearance")
	float VerticalOffset = 40.f; // Offset below crosshair
};

