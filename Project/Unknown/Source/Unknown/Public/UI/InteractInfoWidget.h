// Terminal-styled info widget that displays contextual information about interactables
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InteractInfoWidget.generated.h"

class AItemPickup;
class UItemDefinition;

UCLASS()
class UNKNOWN_API UInteractInfoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// Update widget content for an item pickup
	UFUNCTION(BlueprintCallable, Category="InteractInfo")
	void SetInteractableItem(AItemPickup* ItemPickup);

	// Position widget adjacent to the highlight rectangle
	UFUNCTION(BlueprintCallable, Category="InteractInfo")
	void SetPosition(const FVector2D& HighlightTopLeft, const FVector2D& HighlightBottomRight);

	// Set keybindings to display (called by PlayerController)
	UFUNCTION(BlueprintCallable, Category="InteractInfo")
	void SetKeybindings(const FString& InteractKeyName, const FString& PickupKeyName);

	// Toggle visibility
	UFUNCTION(BlueprintCallable, Category="InteractInfo")
	void SetVisible(bool bInVisible);

protected:
	virtual int32 NativePaint(const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled) const override;

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// Current item data
	UPROPERTY()
	TObjectPtr<UItemDefinition> CurrentItemDef;

	TMap<FName, FString> CurrentCustomData;

	// Position data
	FVector2D HighlightTopLeft = FVector2D::ZeroVector;
	FVector2D HighlightBottomRight = FVector2D::ZeroVector;
	bool bWidgetVisible = false;

	// Keybinding data
	FString InteractKeyName;
	FString PickupKeyName;

	// Calculated widget position and size
	mutable FVector2D WidgetPosition = FVector2D::ZeroVector;
	mutable FVector2D WidgetSize = FVector2D::ZeroVector;
	mutable bool bPositionOnRight = true;

	// Appearance constants
	static constexpr float BorderThickness = 2.f;
	static constexpr float Padding = 8.f;
	static constexpr float TextLineSpacing = 4.f;
	static constexpr float SectionSpacing = 8.f;
	static constexpr float DividerThickness = 1.f;
	static constexpr int32 FontSize = 12;

	// Helper methods
	void CalculateWidgetSize(const FGeometry& AllottedGeometry, FVector2D& OutSize) const;
	void CalculateWidgetPosition(const FGeometry& AllottedGeometry, FVector2D& OutPosition, bool& bOutPositionOnRight) const;
	FString GetItemName() const;
	FString GetItemSpecificInfo() const;
	TArray<FString> GetKeybindings() const;
};

