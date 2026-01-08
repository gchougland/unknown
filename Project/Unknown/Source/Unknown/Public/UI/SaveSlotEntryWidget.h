// Save slot entry widget for the save slot menu list
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveSlotEntryWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveSlotEntryClicked, const FString&, SlotId);

UCLASS()
class UNKNOWN_API USaveSlotEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USaveSlotEntryWidget(const FObjectInitializer& ObjectInitializer);

	// Set the save slot data
	UFUNCTION(BlueprintCallable, Category="SaveSlotEntry")
	void SetSlotData(const FString& SlotId, const FString& SlotName, const FString& Timestamp, bool bIsEmpty);

	// Set selection state
	UFUNCTION(BlueprintCallable, Category="SaveSlotEntry")
	void SetSelected(bool bSelected);

	// Get selection state
	UFUNCTION(BlueprintPure, Category="SaveSlotEntry")
	bool IsSelected() const { return bIsSelected; }

	// Delegate for slot click
	UPROPERTY(BlueprintAssignable, Category="SaveSlotEntry")
	FOnSaveSlotEntryClicked OnSlotClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// Widget tree references
	UPROPERTY(Transient)
	TObjectPtr<class UBorder> OuterBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> InnerBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> SlotNameText;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TimestampText;

	UPROPERTY(Transient)
	TObjectPtr<class UBorder> CursorBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UOverlay> ContentOverlay;

private:
	// Slot data
	FString SlotId;
	FString SlotName;
	FString Timestamp;
	bool bIsEmptySlot;
	bool bIsSelected = false;

	// Blinking cursor state
	bool bIsHovered = false;
	bool bCursorVisible = true;
	float CursorBlinkTimer = 0.0f;
	static constexpr float CursorBlinkInterval = 0.5f;

	// Press state
	bool bIsPressed = false;

	// Update cursor visibility based on blink timer
	void UpdateCursorBlink(float DeltaTime);
};

