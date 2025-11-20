#include "UI/ItemIconHelper.h"
#include "Inventory/ItemDefinition.h"
#include "Icons/ItemIconSubsystem.h"
#include "Icons/ItemIconSettings.h"
#include "Components/Image.h"
#include "Styling/SlateBrush.h"

namespace ItemIconHelper
{
    FItemIconStyle CreateIconStyle(int32 OverrideResolution, EItemIconBackground* OverrideBackground)
    {
        FItemIconStyle Style;
        
        if (const UItemIconSettings* Settings = UItemIconSettings::Get())
        {
            Style.Resolution = OverrideResolution > 0 ? OverrideResolution : Settings->DefaultResolution;
            Style.Background = OverrideBackground ? *OverrideBackground : 
                (Settings->bTransparentBackground ? EItemIconBackground::Transparent : EItemIconBackground::SolidColor);
            Style.SolidColor = Settings->SolidBackgroundColor;
            Style.StyleVersion = Settings->StyleVersion;
        }
        else
        {
            // Fallback defaults
            Style.Resolution = OverrideResolution > 0 ? OverrideResolution : 256;
            Style.Background = OverrideBackground ? *OverrideBackground : EItemIconBackground::Transparent;
            Style.SolidColor = FLinearColor(0.125f, 0.125f, 0.125f, 1.0f);
            Style.StyleVersion = 1;
        }
        
        return Style;
    }

    UTexture2D* LoadIconSync(const UItemDefinition* Def, const FItemIconStyle& Style)
    {
        if (!Def)
        {
            return nullptr;
        }

        // Check for override icon first
        if (Def->IconOverride)
        {
            return Def->IconOverride;
        }

        // Try to get from icon subsystem
        if (UItemIconSubsystem* IconSys = UItemIconSubsystem::Get())
        {
            return IconSys->GetIconIfReady(Def, Style);
        }

        return nullptr;
    }

    void LoadIconAsync(const UItemDefinition* Def, const FItemIconStyle& Style, FOnItemIconReady OnReady)
    {
        if (!Def || !OnReady.IsBound())
        {
            return;
        }

        // Check for override icon first
        if (Def->IconOverride)
        {
            OnReady.ExecuteIfBound(Def, Def->IconOverride);
            return;
        }

        // Request from icon subsystem
        if (UItemIconSubsystem* IconSys = UItemIconSubsystem::Get())
        {
            IconSys->RequestIcon(Def, Style, MoveTemp(OnReady));
        }
    }

    void ApplyIconToImage(UImage* ImageWidget, UTexture2D* Texture, const FVector2D& IconSize)
    {
        if (!ImageWidget)
        {
            return;
        }

        if (Texture)
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Image;
            Brush.SetResourceObject(Texture);
            Brush.ImageSize = IconSize;
            ImageWidget->SetBrush(Brush);
            ImageWidget->SetOpacity(1.f);
            ImageWidget->SetVisibility(ESlateVisibility::Visible);
        }
        else
        {
            ImageWidget->SetBrush(FSlateBrush());
            ImageWidget->SetOpacity(0.f);
            ImageWidget->SetVisibility(ESlateVisibility::Visible);
        }
    }

    void PrewarmIcons(const TArray<const UItemDefinition*>& Defs, const FItemIconStyle& Style)
    {
        if (Defs.Num() == 0)
        {
            return;
        }

        if (UItemIconSubsystem* IconSys = UItemIconSubsystem::Get())
        {
            IconSys->Prewarm(Defs, Style);
        }
    }
}

