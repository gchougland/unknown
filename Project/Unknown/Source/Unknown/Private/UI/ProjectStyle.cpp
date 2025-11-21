#include "UI/ProjectStyle.h"
#include "Engine/AssetManager.h"
#include "UObject/SoftObjectPath.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Widgets/SWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Framework/Application/SlateApplication.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Templates/Function.h"

namespace ProjectStyle
{
    static TSoftObjectPtr<UObject> GMonoFontAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono")));

    static UObject* LoadFontObject()
    {
        UObject* FontObj = GMonoFontAsset.Get();
        if (!FontObj)
        {
            if (GMonoFontAsset.IsNull())
            {
                GMonoFontAsset = TSoftObjectPtr<UObject>(FSoftObjectPath(TEXT("/Game/ThirdParty/Fonts/Font_IBMPlexMono.Font_IBMPlexMono")));
            }
            if (UAssetManager* AM = UAssetManager::GetIfInitialized())
            {
                FontObj = AM->GetStreamableManager().LoadSynchronous(GMonoFontAsset.ToSoftObjectPath(), false);
            }
            else
            {
                FontObj = GMonoFontAsset.LoadSynchronous();
            }
        }
        return FontObj;
    }

    FSlateFontInfo GetMonoFont(int32 Size)
    {
        UObject* FontObj = LoadFontObject();
        FSlateFontInfo Info;
        Info.Size = Size;
        Info.FontObject = FontObj;
        // Some fonts expose face names like "Regular"; leaving default if unknown
        return Info;
    }

    FSlateFontInfo GetMonoFontBold(int32 Size)
    {
        UObject* FontObj = LoadFontObject();
        FSlateFontInfo Info;
        Info.Size = Size;
        Info.FontObject = FontObj;
        // Specify the Bold typeface from the composite font
        Info.TypefaceFontName = FName(TEXT("Bold"));
        return Info;
    }

    // Terminal color constants
    FLinearColor GetTerminalWhite()
    {
        return FLinearColor(1.f, 1.f, 1.f, 1.f);
    }

    FLinearColor GetTerminalBlack()
    {
        return FLinearColor(0.f, 0.f, 0.f, 1.f);
    }

    // Terminal brushes (initialized on first access)
    static FSlateBrush* GetTerminalWhiteBrushImpl()
    {
        static FSlateBrush WhiteBrush;
        static bool bInitialized = false;
        if (!bInitialized)
        {
            WhiteBrush = FSlateBrush();
            WhiteBrush.DrawAs = ESlateBrushDrawType::Box;
            WhiteBrush.TintColor = GetTerminalWhite();
            WhiteBrush.Margin = FMargin(0.f);
            bInitialized = true;
        }
        return &WhiteBrush;
    }

    static FSlateBrush* GetTerminalBlackBrushImpl()
    {
        static FSlateBrush BlackBrush;
        static bool bInitialized = false;
        if (!bInitialized)
        {
            BlackBrush = FSlateBrush();
            BlackBrush.DrawAs = ESlateBrushDrawType::Box;
            BlackBrush.TintColor = GetTerminalBlack();
            BlackBrush.Margin = FMargin(0.f);
            bInitialized = true;
        }
        return &BlackBrush;
    }

    const FSlateBrush* GetTerminalWhiteBrush()
    {
        return GetTerminalWhiteBrushImpl();
    }

    const FSlateBrush* GetTerminalBlackBrush()
    {
        return GetTerminalBlackBrushImpl();
    }

    const FButtonStyle* GetTerminalButtonStyle()
    {
        static FButtonStyle ButtonStyle;
        static bool bInitialized = false;
        if (!bInitialized)
        {
            const FSlateBrush* BlackBrush = GetTerminalBlackBrush();
            const FLinearColor White = GetTerminalWhite();
            ButtonStyle = FButtonStyle()
                .SetNormal(*BlackBrush)
                .SetHovered(*BlackBrush)
                .SetPressed(*BlackBrush)
                .SetNormalForeground(White)
                .SetHoveredForeground(White)
                .SetPressedForeground(White)
                .SetDisabledForeground(White);
            bInitialized = true;
        }
        return &ButtonStyle;
    }

    TSharedRef<SWidget> MakeTerminalMenuRow(const FString& Label, TFunction<void()> OnClick)
    {
        TSharedPtr<SButton> Button;
        const FSlateBrush* WhiteBrush = GetTerminalWhiteBrush();
        const FButtonStyle* ButtonStyle = GetTerminalButtonStyle();
        const FLinearColor White = GetTerminalWhite();

        return SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(White)
            .Padding(0.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SAssignNew(Button, SButton)
                    .ButtonStyle(ButtonStyle)
                    .IsFocusable(false)
                    .ContentPadding(FMargin(8.f, 4.f))
                    .OnClicked_Lambda([OnClick]()
                    {
                        if (OnClick) { OnClick(); }
                        FSlateApplication::Get().DismissAllMenus();
                        // Note: Focus restoration should be handled by OnClick callback if needed
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Label))
                        .ColorAndOpacity(White)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .BorderImage(WhiteBrush)
                    .BorderBackgroundColor(White)
                    .Padding(0.f)
                    .Visibility_Lambda([Button]() -> EVisibility 
                    { 
                        return (Button.IsValid() && Button->IsHovered()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; 
                    })
                    [
                        SNew(SBox)
                        .WidthOverride(2.f)
                    ]
                ]
            ];
    }

    TSharedRef<SWidget> MakeTerminalMenuFrame(TSharedRef<SWidget> Content)
    {
        const FSlateBrush* WhiteBrush = GetTerminalWhiteBrush();
        const FSlateBrush* BlackBrush = GetTerminalBlackBrush();
        const FLinearColor White = GetTerminalWhite();
        const FLinearColor Black = GetTerminalBlack();

        return SNew(SBorder)
            .BorderImage(WhiteBrush)
            .BorderBackgroundColor(White)
            .Padding(1.f)
            [
                SNew(SBorder)
                .BorderImage(BlackBrush)
                .BorderBackgroundColor(Black)
                .Padding(6.f)
                [ Content ]
            ];
    }
}
