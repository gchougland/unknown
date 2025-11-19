#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "MessageLogSubsystem.generated.h"

class UHudMessageLogWidget;

/** Engine-wide message/toast subsystem. Displays stacking messages in top-right, fades out automatically. */
UCLASS()
class UNKNOWN_API UMessageLogSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    UMessageLogSubsystem() = default;

    UFUNCTION(BlueprintCallable, Category="Messages")
    void PushMessage(const FText& Message, float DurationSeconds = 3.5f);

private:
    UPROPERTY(Transient)
    TObjectPtr<UHudMessageLogWidget> Widget;

    void EnsureWidget();
};
