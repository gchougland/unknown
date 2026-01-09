#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "LoadingFadeWidget.generated.h"

class SBorder;

/**
 * Fullscreen fade widget for loading transitions.
 * Fades to black before loading, stays black during load, then fades back in.
 */
UCLASS()
class UNKNOWN_API ULoadingFadeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	ULoadingFadeWidget(const FObjectInitializer& ObjectInitializer);

	// Fade to black (opacity 0 -> 1)
	// Duration in seconds
	UFUNCTION(BlueprintCallable, Category="Fade")
	void FadeOut(float Duration = 0.5f);

	// Fade from black (opacity 1 -> 0)
	// Duration in seconds
	UFUNCTION(BlueprintCallable, Category="Fade")
	void FadeIn(float Duration = 0.5f);

	// Check if fade is complete
	UFUNCTION(BlueprintPure, Category="Fade")
	bool IsFading() const { return bIsFading; }

	// Set opacity directly (0 = transparent, 1 = fully opaque black)
	UFUNCTION(BlueprintCallable, Category="Fade")
	void SetOpacity(float Opacity);

	// Get current opacity
	UFUNCTION(BlueprintPure, Category="Fade")
	float GetOpacity() const { return CurrentOpacity; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

private:
	// Fullscreen black overlay (Slate widget)
	TSharedPtr<SBorder> FadeOverlay;

	// Current opacity (0 = transparent, 1 = fully opaque)
	UPROPERTY()
	float CurrentOpacity = 0.0f;

	// Animation state
	UPROPERTY()
	float TargetOpacity = 0.0f;

	UPROPERTY()
	float FadeDuration = 0.5f;

	UPROPERTY()
	float FadeElapsed = 0.0f;

	UPROPERTY()
	bool bIsFading = false;

	UPROPERTY()
	float StartOpacity = 0.0f;

	UPROPERTY()
	FTimerHandle FadeTimerHandle;

	// Tick fade animation
	void TickFade();
};

