#include "UI/HotbarSlotBuilder.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryUIConstants.h"
#include "Blueprint/WidgetTree.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Widgets/SBoxPanel.h"

namespace HotbarSlotBuilder
{
    void BuildSlotWidget(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootBox,
        int32 SlotIndex,
        const FLinearColor& BorderColor,
        const FLinearColor& SlotBackground,
        const FLinearColor& TextColor,
        const FVector2D& SlotSize,
        FSlotWidgets& OutWidgets)
    {
        if (!WidgetTree || !RootBox)
        {
            return;
        }

        // Create terminal-styled bordered panel (outer white border, inner black background)
        OutWidgets.OuterBorder = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
            WidgetTree,
            BorderColor,
            SlotBackground,
            FMargin(InventoryUIConstants::Padding_Outer),
            FMargin(0.f),
            *FString::Printf(TEXT("SlotOuter_%d"), SlotIndex),
            *FString::Printf(TEXT("SlotBorder_%d"), SlotIndex)
        );

        UVerticalBoxSlot* VBSlot = RootBox->AddChildToVerticalBox(OutWidgets.OuterBorder);
        if (VBSlot)
        {
            // No external spacing between slots and do not stretch to full width — keep square
            VBSlot->SetPadding(FMargin(0.f));
            VBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBSlot->SetHorizontalAlignment(HAlign_Left);
        }

        // Get the inner border (created by CreateTerminalBorderedPanel) for highlighting active slot
        OutWidgets.InnerBorder = Cast<UBorder>(OutWidgets.OuterBorder->GetContent());
        if (!OutWidgets.InnerBorder)
        {
            UE_LOG(LogTemp, Error, TEXT("[HotbarSlotBuilder] Failed to get inner border from CreateTerminalBorderedPanel"));
            return;
        }

        // Size box to enforce slot geometry even without textures
        OutWidgets.SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("SizeBox_%d"), SlotIndex));
        OutWidgets.SizeBox->SetWidthOverride(SlotSize.X);
        OutWidgets.SizeBox->SetHeightOverride(SlotSize.Y);
        OutWidgets.InnerBorder->SetContent(OutWidgets.SizeBox);

        // Inner overlay: icon with quantity text overlaid
        OutWidgets.Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Inner_%d"), SlotIndex));
        OutWidgets.SizeBox->SetContent(OutWidgets.Overlay);

        OutWidgets.Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Icon_%d"), SlotIndex));
        // Ensure the icon fills the square slot
        {
            FSlateBrush IconBrush;
            IconBrush.DrawAs = ESlateBrushDrawType::Image;
            IconBrush.Tiling = ESlateBrushTileType::NoTile;
            IconBrush.ImageSize = SlotSize;
            OutWidgets.Icon->SetBrush(IconBrush);
        }
        UOverlaySlot* IconSlot = OutWidgets.Overlay->AddChildToOverlay(OutWidgets.Icon);
        if (IconSlot)
        {
            IconSlot->SetHorizontalAlignment(HAlign_Fill);
            IconSlot->SetVerticalAlignment(VAlign_Fill);
            IconSlot->SetPadding(FMargin(0.f));
        }

        // Hotkey label (always visible) at top-left
        OutWidgets.Hotkey = TerminalWidgetHelpers::CreateTerminalTextBlock(
            WidgetTree,
            FString::FromInt((SlotIndex + 1) % 10 == 0 ? 0 : (SlotIndex + 1)),
            InventoryUIConstants::FontSize_Hotkey,
            TextColor,
            *FString::Printf(TEXT("Hotkey_%d"), SlotIndex)
        );
        OutWidgets.Hotkey->SetShadowOffset(InventoryUIConstants::ShadowOffset);
        OutWidgets.Hotkey->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, InventoryUIConstants::ShadowOpacity));
        UOverlaySlot* HotkeySlot = OutWidgets.Overlay->AddChildToOverlay(OutWidgets.Hotkey);
        if (HotkeySlot)
        {
            HotkeySlot->SetHorizontalAlignment(HAlign_Left);
            HotkeySlot->SetVerticalAlignment(VAlign_Top);
            HotkeySlot->SetPadding(FMargin(InventoryUIConstants::Padding_Hotkey, InventoryUIConstants::Padding_Hotkey, 0.f, 0.f));
        }

        // Quantity text; ensure it renders above the hotkey so it can't be occluded
        OutWidgets.Quantity = TerminalWidgetHelpers::CreateTerminalTextBlock(
            WidgetTree,
            TEXT("0"),
            InventoryUIConstants::FontSize_Quantity,
            TextColor,
            *FString::Printf(TEXT("Quantity_%d"), SlotIndex)
        );
        OutWidgets.Quantity->SetShadowOffset(InventoryUIConstants::ShadowOffset);
        OutWidgets.Quantity->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, InventoryUIConstants::ShadowOpacity));
        OutWidgets.Quantity->SetAutoWrapText(false);
        // Guard against zero-width measurement by ensuring a minimal desired width
        OutWidgets.Quantity->SetMinDesiredWidth(InventoryUIConstants::FontSize_InfoDesc);
        // Start hidden until we compute a quantity > 1
        OutWidgets.Quantity->SetVisibility(ESlateVisibility::Collapsed);
        UOverlaySlot* QuantitySlot = OutWidgets.Overlay->AddChildToOverlay(OutWidgets.Quantity);
        if (QuantitySlot)
        {
            // Right-aligned as requested
            QuantitySlot->SetHorizontalAlignment(HAlign_Right);
            QuantitySlot->SetVerticalAlignment(VAlign_Top);
            QuantitySlot->SetPadding(FMargin(0.f, InventoryUIConstants::Padding_Quantity, InventoryUIConstants::Padding_Quantity, 0.f));
        }
    }
}

