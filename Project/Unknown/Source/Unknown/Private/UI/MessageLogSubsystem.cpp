#include "UI/MessageLogSubsystem.h"
#include "UI/HudMessageLogWidget.h"
#include "Blueprint/UserWidget.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"

DEFINE_LOG_CATEGORY_STATIC(LogMessageLogSubsystem, Log, All);

void UMessageLogSubsystem::EnsureWidget()
{
    if (Widget)
    {
        UE_LOG(LogMessageLogSubsystem, Verbose, TEXT("MessageLog widget already exists; skipping creation."));
        return;
    }
    if (!GEngine)
    {
        UE_LOG(LogMessageLogSubsystem, Warning, TEXT("GEngine is null; cannot create MessageLog widget."));
        return;
    }

    UWorld* World = nullptr;
    // Prefer the GameViewport world if available (ensures the widget is actually visible on screen)
    if (GEngine->GameViewport)
    {
        World = GEngine->GameViewport->GetWorld();
        UE_LOG(LogMessageLogSubsystem, Verbose, TEXT("Using GameViewport world: %s"), *GetNameSafe(World));
    }
    // Fallback: first valid Game/PIE world
    if (!World)
    {
        const TIndirectArray<FWorldContext>& Contexts = GEngine->GetWorldContexts();
        for (const FWorldContext& Ctx : Contexts)
        {
            if (Ctx.World() && (Ctx.WorldType == EWorldType::Game || Ctx.WorldType == EWorldType::PIE))
            {
                World = Ctx.World();
                UE_LOG(LogMessageLogSubsystem, Verbose, TEXT("Found fallback world via contexts: %s (Type=%d)"), *GetNameSafe(World), (int32)Ctx.WorldType);
                break;
            }
        }
    }
    if (!World)
    {
        UE_LOG(LogMessageLogSubsystem, Warning, TEXT("No suitable World found to host MessageLog widget."));
        return;
    }

    // Create widget and add to viewport at a high Z-order
    Widget = CreateWidget<UHudMessageLogWidget>(World, UHudMessageLogWidget::StaticClass());
    if (Widget)
    {
        Widget->AddToViewport(9999);
        // Ensure the HUD message widget does NOT intercept mouse/keyboard input
        Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
        UE_LOG(LogMessageLogSubsystem, Log, TEXT("Created and added MessageLog widget to viewport (World=%s)."), *GetNameSafe(World));
    }
    else
    {
        UE_LOG(LogMessageLogSubsystem, Error, TEXT("Failed to create UHudMessageLogWidget."));
    }
}

void UMessageLogSubsystem::PushMessage(const FText& Message, float DurationSeconds)
{
    UE_LOG(LogMessageLogSubsystem, Verbose, TEXT("PushMessage: '%s' Duration=%.2fs"), *Message.ToString(), DurationSeconds);
    EnsureWidget();
    if (Widget)
    {
        Widget->AddMessage(Message, DurationSeconds);
    }
    else
    {
        UE_LOG(LogMessageLogSubsystem, Warning, TEXT("PushMessage dropped because widget is not available."));
    }
}
