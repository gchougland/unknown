#include "UI/HudMessageLogWidget.h"
#include "UI/ProjectStyle.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"
#include "Engine/World.h"
#include "TimerManager.h"

// Local log for HUD message output mirroring
DEFINE_LOG_CATEGORY_STATIC(LogHudMessage, Log, All);

TSharedRef<SWidget> UHudMessageLogWidget::RebuildWidget()
{
    // Root overlay to allow top-right anchoring
    TSharedRef<SOverlay> Root = SNew(SOverlay)
        .Visibility(EVisibility::HitTestInvisible); // Do not intercept any mouse/keyboard input
    RootVBox = SNew(SVerticalBox)
        .Visibility(EVisibility::HitTestInvisible);

    // Top-right stacked messages
    Root->AddSlot()
    .HAlign(HAlign_Right)
    .VAlign(VAlign_Top)
    [
        RootVBox.ToSharedRef()
    ];

    return Root;
}

void UHudMessageLogWidget::AddMessage(const FText& Message, float DurationSeconds)
{
    // Mirror on-screen toasts to the log for debugging/CI visibility
    UE_LOG(LogHudMessage, Display, TEXT("[HUDMessage] %s (Duration=%.2fs)"), *Message.ToString(), DurationSeconds);

    if (!RootVBox.IsValid())
    {
        TakeWidget();
        if (!RootVBox.IsValid())
        {
            return;
        }
    }

    // Visual row: semi-transparent black background, white text with project font
    TSharedPtr<SBorder> RowBorder;
    TSharedRef<SWidget> Row =
        SAssignNew(RowBorder, SBorder)
        .Visibility(EVisibility::HitTestInvisible) // Belt-and-suspenders: children also ignore hit tests
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.7f))
        .Padding(FMargin(8.f, 4.f))
        [
            SNew(STextBlock)
            .Text(Message)
            .Font(ProjectStyle::GetMonoFont(14))
            .ColorAndOpacity(FLinearColor::White)
            .ShadowOffset(FVector2D(1.f,1.f))
            .ShadowColorAndOpacity(FLinearColor(0.f,0.f,0.f,0.85f))
        ];

    RootVBox->AddSlot()
    .AutoHeight()
    .Padding(FMargin(0.f, 2.f))
    [ Row ];

    // Initialize animation state
    TSharedPtr<FMsgEntry> Entry = MakeShared<FMsgEntry>();
    Entry->Row = RowBorder;
    if (Entry->Row.IsValid())
    {
        Entry->Row->SetRenderOpacity(0.f);
        Entry->Row->SetRenderTransformPivot(FVector2D(1.f, 0.f)); // top-right pivot
        Entry->Row->SetRenderTransform(FSlateRenderTransform(FScale2D(0.9f), FVector2D(20.f, 0.f)));
    }
    Messages.Add(Entry);

    // Enter animation (pop-in)
    BeginEnter(Entry, 0.22f);

    // Schedule exit animation and removal
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }
    const float ExitDuration = 0.25f;
    const float NowToFade = FMath::Max(0.0f, DurationSeconds - ExitDuration);
    // Start fade near the end of lifetime
    World->GetTimerManager().SetTimer(Entry->FadeTimer, FTimerDelegate::CreateUObject(this, &UHudMessageLogWidget::BeginFade, Entry, ExitDuration), NowToFade, false);
    // Remove after fade fully completes
    World->GetTimerManager().SetTimer(Entry->RemoveTimer, FTimerDelegate::CreateUObject(this, &UHudMessageLogWidget::RemoveEntry, Entry), DurationSeconds + 0.01f, false);
}

void UHudMessageLogWidget::RemoveEntry(TSharedPtr<FMsgEntry> Entry)
{
    if (!Entry.IsValid() || !RootVBox.IsValid())
    {
        return;
    }
    // Stop any running timers for this entry
    if (UWorld* World = GetWorld())
    {
        FTimerManager& TM = World->GetTimerManager();
        TM.ClearTimer(Entry->EnterTimer);
        TM.ClearTimer(Entry->FadeTimer);
        TM.ClearTimer(Entry->RemoveTimer);
    }
    // Remove the slot containing this row
    // Note: SVerticalBox does not provide direct child removal by widget in this API; rebuild root by clearing and re-adding
    TArray<TSharedPtr<FMsgEntry>> Remaining;
    for (TSharedPtr<FMsgEntry>& E : Messages)
    {
        if (E != Entry)
        {
            Remaining.Add(E);
        }
    }
    Messages = Remaining;

    // Rebuild the VBox children
    RootVBox->ClearChildren();
    for (TSharedPtr<FMsgEntry>& E : Messages)
    {
        if (E.IsValid() && E->Row.IsValid())
        {
            RootVBox->AddSlot().AutoHeight().Padding(FMargin(0.f,2.f))[ E->Row.ToSharedRef() ];
        }
    }
}

// Simple easing for pop-in: ease-out with slight overshoot (Back easing)
static float EaseOutBack(float T, float Overshoot)
{
    // From Robert Penner's easing with customizable overshoot
    const float S = Overshoot;
    T -= 1.f;
    return (T * T * ((S + 1.f) * T + S) + 1.f);
}

void UHudMessageLogWidget::BeginEnter(TSharedPtr<FMsgEntry> Entry, float EnterDuration)
{
    if (!Entry.IsValid() || !Entry->Row.IsValid())
    {
        return;
    }
    Entry->EnterElapsed = 0.f;
    Entry->EnterDuration = FMath::Max(0.01f, EnterDuration);

    if (UWorld* World = GetWorld())
    {
        // Tick at ~60 FPS
        const float Step = 1.f / 60.f;
        World->GetTimerManager().SetTimer(Entry->EnterTimer,
            FTimerDelegate::CreateUObject(this, &UHudMessageLogWidget::TickEnter, Entry, Step),
            Step, true);
    }
}

void UHudMessageLogWidget::TickEnter(TSharedPtr<FMsgEntry> Entry, float Step)
{
    if (!Entry.IsValid() || !Entry->Row.IsValid())
    {
        return;
    }
    Entry->EnterElapsed += Step;
    const float T = FMath::Clamp(Entry->EnterElapsed / Entry->EnterDuration, 0.f, 1.f);
    const float Opacity = T;
    const float Scale = 0.9f + 0.2f * EaseOutBack(T, 0.9f); // 0.9 -> ~1.1 then settle
    const float OffsetX = FMath::Lerp(20.f, 0.f, T);

    Entry->Row->SetRenderOpacity(Opacity);
    Entry->Row->SetRenderTransform(FSlateRenderTransform(FScale2D(Scale), FVector2D(OffsetX, 0.f)));

    if (T >= 1.f)
    {
        // Snap to final
        Entry->Row->SetRenderOpacity(1.f);
        Entry->Row->SetRenderTransform(FSlateRenderTransform(FScale2D(1.f), FVector2D(0.f, 0.f)));
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(Entry->EnterTimer);
        }
    }
}

void UHudMessageLogWidget::BeginFade(TSharedPtr<FMsgEntry> Entry, float FadeDuration)
{
    if (!Entry.IsValid() || !Entry->Row.IsValid())
    {
        return;
    }
    if (Entry->bFadingOut)
    {
        return;
    }
    Entry->bFadingOut = true;
    Entry->ExitElapsed = 0.f;
    Entry->ExitDuration = FMath::Max(0.01f, FadeDuration);

    if (UWorld* World = GetWorld())
    {
        const float Step = 1.f / 60.f;
        World->GetTimerManager().SetTimer(Entry->FadeTimer,
            FTimerDelegate::CreateUObject(this, &UHudMessageLogWidget::TickFade, Entry, Step, Entry->ExitDuration),
            Step, true);
    }
}

void UHudMessageLogWidget::TickFade(TSharedPtr<FMsgEntry> Entry, float Step, float TargetDuration)
{
    if (!Entry.IsValid() || !Entry->Row.IsValid())
    {
        return;
    }
    Entry->ExitElapsed += Step;
    const float T = FMath::Clamp(Entry->ExitElapsed / TargetDuration, 0.f, 1.f);
    const float Opacity = 1.f - T;
    const float OffsetX = FMath::Lerp(0.f, 20.f, T);
    const float Scale = FMath::Lerp(1.f, 0.98f, T);
    Entry->Row->SetRenderOpacity(Opacity);
    Entry->Row->SetRenderTransform(FSlateRenderTransform(FScale2D(Scale), FVector2D(OffsetX, 0.f)));

    if (T >= 1.f)
    {
        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().ClearTimer(Entry->FadeTimer);
        }
    }
}
