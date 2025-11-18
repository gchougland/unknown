#pragma once

#include "CoreMinimal.h"
#include "Subsystems/EngineSubsystem.h"
#include "Icons/ItemIconTypes.h"
#include "ItemIconSubsystem.generated.h"

class UTexture2D;
class UItemDefinition;
struct FItemIconStyle;
struct FItemIconKey;
class UWorld;

DECLARE_DELEGATE_TwoParams(FOnItemIconReady, const UItemDefinition* /*Def*/, UTexture2D* /*Texture*/);

UCLASS()
class UNKNOWN_API UItemIconSubsystem : public UEngineSubsystem
{
    GENERATED_BODY()
public:
    // UEngineSubsystem
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    // API (C++ only for now)
    UTexture2D* GetIconIfReady(const UItemDefinition* Def, const FItemIconStyle& Style);

    void RequestIcon(const UItemDefinition* Def, const FItemIconStyle& Style, FOnItemIconReady OnReady);

    void Prewarm(const TArray<const UItemDefinition*>& Defs, const FItemIconStyle& Style);

    void InvalidateAll(int32 NewStyleVersion);

    void SetMemoryBudgetMB(float MB);

    float GetMemoryBudgetMB() const { return MemoryBudgetMB; }

    static UItemIconSubsystem* Get();

private:
    // Per-frame processing of request queue
    bool ProcessTick(float DeltaTime);

    struct FPendingRequest
    {
        TArray<FOnItemIconReady> Callbacks;
        const UItemDefinition* Def = nullptr;
        FItemIconStyle Style;
    };

    // Minimal skeleton cache
    TMap<FItemIconKey, TWeakObjectPtr<UTexture2D>> MemoryCache;
    TMap<FItemIconKey, FPendingRequest> Pending;
    TQueue<FItemIconKey> Queue;

    float MemoryBudgetMB = 64.0f;
    int32 BatchPerTick = 2;

    // Helpers
    FItemIconKey MakeKey(const UItemDefinition* Def, const FItemIconStyle& Style) const;

    // Hidden capture rig
    class FItemIconPreviewScene* PreviewScene = nullptr;

    // World resolver for capture
    UWorld* GetAnyGameWorld() const;
};
