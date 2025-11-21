// Reusable stat bar widget with terminal UI theme
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StatBarWidget.generated.h"

/**
 * Reusable stat bar widget displaying a progress bar with terminal UI theme.
 * Features black rectangle with white border, inset colored fill, and label text.
 * Used for hunger, stamina, and other stat displays.
 */
UCLASS()
class UNKNOWN_API UStatBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Set the current and max values (handles "100+" display when current >= max)
	UFUNCTION(BlueprintCallable, Category="StatBar")
	void SetValue(float Current, float Max);

	// Set the abbreviation label (e.g., "HGR", "STM")
	UFUNCTION(BlueprintCallable, Category="StatBar")
	void SetLabel(const FString& InLabel);

	// Set the fill color (default brown for hunger)
	UFUNCTION(BlueprintCallable, Category="StatBar")
	void SetFillColor(const FLinearColor& InColor);

	// Toggle visibility
	UFUNCTION(BlueprintCallable, Category="StatBar")
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
	float CurrentValue = 0.0f;
	float MaxValue = 100.0f;
	FString Label = TEXT("STAT");
	FLinearColor FillColor = FLinearColor(0.6f, 0.4f, 0.2f, 1.0f); // Brown default
	bool bVisible = true;

	// Appearance
	UPROPERTY(EditAnywhere, Category="Appearance")
	float Width = 120.f;
	UPROPERTY(EditAnywhere, Category="Appearance")
	float Height = 24.f; // Taller bar
	UPROPERTY(EditAnywhere, Category="Appearance")
	float BorderThickness = 2.f;
	UPROPERTY(EditAnywhere, Category="Appearance")
	float FillInset = 2.f; // Padding around colored fill to show black border
	UPROPERTY(EditAnywhere, Category="Appearance")
	int32 FontSize = 12;
};

