#include "UI/LoadingFadeWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Styling/CoreStyle.h"

ULoadingFadeWidget::ULoadingFadeWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, CurrentOpacity(0.0f)
	, TargetOpacity(0.0f)
	, FadeDuration(0.5f)
	, FadeElapsed(0.0f)
	, bIsFading(false)
	, StartOpacity(0.0f)
{
}

TSharedRef<SWidget> ULoadingFadeWidget::RebuildWidget()
{
	// Create fullscreen black overlay
	FadeOverlay = SNew(SBorder)
		.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
		.BorderBackgroundColor(FLinearColor::Black)
		.ColorAndOpacity(FLinearColor::White)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		.Visibility(EVisibility::HitTestInvisible); // Don't block input
	
	// Set initial opacity to transparent
	FadeOverlay->SetRenderOpacity(0.0f);
	CurrentOpacity = 0.0f;

	return FadeOverlay.ToSharedRef();
}

void ULoadingFadeWidget::FadeOut(float Duration)
{
	StartOpacity = CurrentOpacity;
	TargetOpacity = 1.0f;
	FadeDuration = FMath::Max(0.01f, Duration);
	FadeElapsed = 0.0f;
	bIsFading = true;

	// Start fade animation tick
	if (UWorld* World = GetWorld())
	{
		const float TickInterval = 1.0f / 60.0f; // 60 FPS
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &ULoadingFadeWidget::TickFade, TickInterval, true);
	}
}

void ULoadingFadeWidget::FadeIn(float Duration)
{
	StartOpacity = CurrentOpacity;
	TargetOpacity = 0.0f;
	FadeDuration = FMath::Max(0.01f, Duration);
	FadeElapsed = 0.0f;
	bIsFading = true;

	// Start fade animation tick
	if (UWorld* World = GetWorld())
	{
		const float TickInterval = 1.0f / 60.0f; // 60 FPS
		World->GetTimerManager().SetTimer(FadeTimerHandle, this, &ULoadingFadeWidget::TickFade, TickInterval, true);
	}
}

void ULoadingFadeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	// Ensure fade overlay exists and is initialized
	if (FadeOverlay.IsValid())
	{
		FadeOverlay->SetRenderOpacity(CurrentOpacity);
	}
}

void ULoadingFadeWidget::SetOpacity(float Opacity)
{
	CurrentOpacity = FMath::Clamp(Opacity, 0.0f, 1.0f);
	if (FadeOverlay.IsValid())
	{
		FadeOverlay->SetRenderOpacity(CurrentOpacity);
	}

	// Stop any active fade
	if (bIsFading)
	{
		bIsFading = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
	}
}

void ULoadingFadeWidget::TickFade()
{
	if (!bIsFading || !FadeOverlay.IsValid())
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		const float TickInterval = 1.0f / 60.0f; // 60 FPS
		FadeElapsed += TickInterval;
		const float Alpha = FMath::Clamp(FadeElapsed / FadeDuration, 0.0f, 1.0f);

		// Lerp from start to target
		CurrentOpacity = FMath::Lerp(StartOpacity, TargetOpacity, Alpha);
		FadeOverlay->SetRenderOpacity(CurrentOpacity);

		// Check if fade is complete
		if (FadeElapsed >= FadeDuration)
		{
			// Snap to final value
			CurrentOpacity = TargetOpacity;
			FadeOverlay->SetRenderOpacity(CurrentOpacity);
			bIsFading = false;
			World->GetTimerManager().ClearTimer(FadeTimerHandle);
		}
	}
}

