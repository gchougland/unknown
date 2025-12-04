// Terminal-styled button widget for main menu with blinking cursor on hover
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/Overlay.h"
#include "MainMenuButtonWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnMainMenuButtonClicked);

UCLASS()
class UNKNOWN_API UMainMenuButtonWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuButtonWidget(const FObjectInitializer& ObjectInitializer);

	// Set the button text
	UFUNCTION(BlueprintCallable, Category="MainMenuButton")
	void SetButtonText(const FString& Text);

	// Delegate for button click
	UPROPERTY(BlueprintAssignable, Category="MainMenuButton")
	FOnMainMenuButtonClicked OnClicked;

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
	TObjectPtr<UBorder> OuterBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> InnerBorder;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ButtonText;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> CursorBorder;

	UPROPERTY(Transient)
	TObjectPtr<UOverlay> ContentOverlay;

private:
	// Blinking cursor state
	bool bIsHovered = false;
	bool bCursorVisible = true;
	float CursorBlinkTimer = 0.0f;
	static constexpr float CursorBlinkInterval = 0.5f;

	// Press state
	bool bIsPressed = false;
	FLinearColor NormalBackgroundColor;
	FLinearColor PressedBackgroundColor;

	// Update cursor visibility based on blink timer
	void UpdateCursorBlink(float DeltaTime);
};

