#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudMessageLogWidget.generated.h"

class SVerticalBox;
class SBorder;

/** Always-on HUD widget that shows stacking messages in the top-right corner. */
UCLASS()
class UNKNOWN_API UHudMessageLogWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    // Add a message to the log with a given lifetime (seconds). Will fade-out and remove.
    void AddMessage(const FText& Message, float DurationSeconds);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;

private:
    struct FMsgEntry
    {
        TSharedPtr<SBorder> Row;
        float Opacity = 1.0f;
        // Animation state
        float EnterElapsed = 0.0f;
        float EnterDuration = 0.0f;
        float ExitElapsed = 0.0f;
        float ExitDuration = 0.0f;
        bool bFadingOut = false;
        FTimerHandle FadeTimer;
        FTimerHandle EnterTimer;
        FTimerHandle RemoveTimer;
    };

    // Root slate container (aligned to top-right)
    TSharedPtr<SVerticalBox> RootVBox;

    // Active messages
    TArray<TSharedPtr<FMsgEntry>> Messages;

    void BeginEnter(TSharedPtr<FMsgEntry> Entry, float EnterDuration);
    void TickEnter(TSharedPtr<FMsgEntry> Entry, float Step);
    void BeginFade(TSharedPtr<FMsgEntry> Entry, float FadeDuration);
    void TickFade(TSharedPtr<FMsgEntry> Entry, float Step, float TargetDuration);
    void RemoveEntry(TSharedPtr<FMsgEntry> Entry);
};
