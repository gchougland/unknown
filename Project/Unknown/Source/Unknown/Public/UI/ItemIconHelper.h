#pragma once

#include "CoreMinimal.h"
#include "Icons/ItemIconTypes.h"
#include "Icons/ItemIconSubsystem.h"

class UItemDefinition;
class UTexture2D;
class UImage;
struct FSlateBrush;

// Delegate for async icon loading callbacks
DECLARE_DELEGATE_TwoParams(FOnIconLoaded, UTexture2D*, const FVector2D&);

namespace ItemIconHelper
{
    /**
     * Creates an FItemIconStyle from project settings with optional overrides
     * @param OverrideResolution Optional resolution override (0 = use settings default)
     * @param OverrideBackground Optional background override (nullptr = use settings default)
     * @return Configured icon style
     */
    UNKNOWN_API FItemIconStyle CreateIconStyle(int32 OverrideResolution = 0, EItemIconBackground* OverrideBackground = nullptr);

    /**
     * Loads an icon synchronously if it's already ready
     * @param Def Item definition to load icon for
     * @param Style Icon style to use
     * @return Texture if ready, nullptr otherwise
     */
    UNKNOWN_API UTexture2D* LoadIconSync(const UItemDefinition* Def, const FItemIconStyle& Style);

    /**
     * Loads an icon asynchronously, calling the callback when ready
     * @param Def Item definition to load icon for
     * @param Style Icon style to use
     * @param OnReady Callback invoked when icon is loaded
     */
    UNKNOWN_API void LoadIconAsync(const UItemDefinition* Def, const FItemIconStyle& Style, FOnItemIconReady OnReady);

    /**
     * Applies an icon texture to a UImage widget with the specified size
     * @param ImageWidget The image widget to update
     * @param Texture The texture to apply (nullptr clears the icon)
     * @param IconSize The size of the icon
     */
    UNKNOWN_API void ApplyIconToImage(UImage* ImageWidget, UTexture2D* Texture, const FVector2D& IconSize);

    /**
     * Prewarms icons for a list of item definitions
     * @param Defs Array of item definitions to prewarm
     * @param Style Icon style to use
     */
    UNKNOWN_API void PrewarmIcons(const TArray<const UItemDefinition*>& Defs, const FItemIconStyle& Style);
}

