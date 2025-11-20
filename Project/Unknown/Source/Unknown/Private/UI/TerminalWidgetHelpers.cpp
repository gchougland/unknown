#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryUIConstants.h"
#include "UI/ProjectStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Styling/SlateBrush.h"

namespace TerminalWidgetHelpers
{
    UBorder* CreateTerminalBorderedPanel(
        UWidgetTree* WidgetTree,
        const FLinearColor& OuterBorderColor,
        const FLinearColor& InnerBackgroundColor,
        const FMargin& OuterPadding,
        const FMargin& InnerPadding,
        const FName& OuterName,
        const FName& InnerName)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        // Outer border
        UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), OuterName);
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = OuterBorderColor;
            Brush.Margin = FMargin(0.f);
            Outer->SetBrush(Brush);
        }
        Outer->SetPadding(OuterPadding);

        // Inner border
        UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), InnerName);
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = InnerBackgroundColor;
            Brush.Margin = FMargin(0.f);
            Inner->SetBrush(Brush);
        }
        Inner->SetPadding(InnerPadding);

        Outer->SetContent(Inner);
        return Outer;
    }

    UTextBlock* CreateTerminalTextBlock(
        UWidgetTree* WidgetTree,
        const FString& Text,
        int32 FontSize,
        const FLinearColor& TextColor,
        const FName& Name)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
        TextBlock->SetText(FText::FromString(Text));
        TextBlock->SetColorAndOpacity(TextColor);
        TextBlock->SetFont(ProjectStyle::GetMonoFont(FontSize));
        return TextBlock;
    }

    UBorder* CreateTerminalHeaderCell(
        UWidgetTree* WidgetTree,
        UHorizontalBox* ParentHBox,
        float Width,
        const FString& Label,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor)
    {
        if (!WidgetTree || !ParentHBox)
        {
            return nullptr;
        }

        // Outer cell border
        UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = BorderColor;
            Brush.Margin = FMargin(0.f);
            CellBorder->SetBrush(Brush);
        }
        CellBorder->SetPadding(FMargin(0.f));

        if (UHorizontalBoxSlot* Slot = ParentHBox->AddChildToHorizontalBox(CellBorder))
        {
            Slot->SetPadding(FMargin(1.f, 0.f));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }

        // Size box for width constraint
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Size->SetWidthOverride(Width);
        Size->SetMinDesiredHeight(InventoryUIConstants::RowHeight_Header);

        // Inner background
        UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = BackgroundColor;
            Brush.Margin = FMargin(0.f);
            Inner->SetBrush(Brush);
        }
        Inner->SetPadding(FMargin(InventoryUIConstants::Padding_InnerTiny, InventoryUIConstants::Padding_Cell));
        Inner->SetHorizontalAlignment(HAlign_Fill);
        Inner->SetVerticalAlignment(VAlign_Center);

        // Text block
        UTextBlock* TextBlock = CreateTerminalTextBlock(WidgetTree, Label, InventoryUIConstants::FontSize_Header, TextColor);
        Inner->SetContent(TextBlock);
        Size->SetContent(Inner);
        CellBorder->SetContent(Size);

        return CellBorder;
    }
}

