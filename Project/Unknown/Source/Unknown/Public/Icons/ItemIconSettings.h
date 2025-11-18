#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "ItemIconSettings.generated.h"

/**
 * Editor-visible settings for the item icon system
 */
UCLASS(config=Game, defaultconfig)
class UNKNOWN_API UItemIconSettings : public UDeveloperSettings
{
    GENERATED_BODY()
public:
    UItemIconSettings(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    // UDeveloperSettings
    virtual FName GetCategoryName() const override { return TEXT("Game"); }
    virtual FText GetSectionText() const override { return NSLOCTEXT("ItemIcons", "Section", "Item Icons"); }

    // Defaults
    UPROPERTY(EditAnywhere, Config, Category="Item Icons", meta=(ClampMin=64, ClampMax=1024))
    int32 DefaultResolution = 256;

    UPROPERTY(EditAnywhere, Config, Category="Item Icons", meta=(ClampMin=1, ClampMax=16))
    int32 BatchPerTick = 2;

    UPROPERTY(EditAnywhere, Config, Category="Item Icons", meta=(ClampMin=1.0, ClampMax=1024.0))
    float MemoryBudgetMB = 64.0f;

    UPROPERTY(EditAnywhere, Config, Category="Item Icons")
    bool bTransparentBackground = true;

    UPROPERTY(EditAnywhere, Config, Category="Item Icons")
    FLinearColor SolidBackgroundColor = FLinearColor(0.125f, 0.125f, 0.125f, 1.0f);

    UPROPERTY(EditAnywhere, Config, Category="Item Icons", meta=(ClampMin=1, ClampMax=1000000))
    int32 StyleVersion = 1;

    static const UItemIconSettings* Get();
};
