#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryUIConstants.h"
#include "UI/ProjectStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"

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

    UScrollBox* CreateTerminalScrollBox(
        UWidgetTree* WidgetTree,
        const FMargin& OuterPadding,
        const FMargin& InnerPadding,
        const FName& Name)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        // Create scroll box first
        UScrollBox* ScrollBox = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), Name);
        if (!ScrollBox)
        {
            return nullptr;
        }

        // Create terminal-styled border panel to wrap the scroll box
        UBorder* OuterBorder = CreateTerminalBorderedPanel(
            WidgetTree,
            ProjectStyle::GetTerminalWhite(),
            ProjectStyle::GetTerminalBlack(),
            OuterPadding,
            InnerPadding,
            Name.IsNone() ? FName(TEXT("ScrollBoxOuter")) : FName(*(Name.ToString() + TEXT("_Outer"))),
            Name.IsNone() ? FName(TEXT("ScrollBoxInner")) : FName(*(Name.ToString() + TEXT("_Inner")))
        );

        if (OuterBorder)
        {
            // Get the inner border from the panel and set scroll box as content
            UBorder* InnerBorder = Cast<UBorder>(OuterBorder->GetContent());
            if (InnerBorder)
            {
                InnerBorder->SetContent(ScrollBox);
            }
        }

        return ScrollBox;
    }

    UEditableTextBox* CreateTerminalEditableTextBox(
        UWidgetTree* WidgetTree,
        const FString& PlaceholderText,
        int32 FontSize,
        float Width,
        float Height,
        const FName& Name)
    {
        if (!WidgetTree)
        {
            return nullptr;
        }

        // Create terminal-styled border panel for the text box
        UBorder* OuterBorder = CreateTerminalBorderedPanel(
            WidgetTree,
            ProjectStyle::GetTerminalWhite(),
            ProjectStyle::GetTerminalBlack(),
            FMargin(2.f),
            FMargin(8.f, 4.f),
            Name.IsNone() ? FName(TEXT("TextBoxOuter")) : FName(*(Name.ToString() + TEXT("_Outer"))),
            Name.IsNone() ? FName(TEXT("TextBoxInner")) : FName(*(Name.ToString() + TEXT("_Inner")))
        );

        if (!OuterBorder)
        {
            return nullptr;
        }

        // Create editable text box
        UEditableTextBox* TextBox = WidgetTree->ConstructWidget<UEditableTextBox>(UEditableTextBox::StaticClass(), Name);
        if (TextBox)
        {
            // Set terminal styling
            FEditableTextBoxStyle TextBoxStyle;
            FSlateBrush NormalBrush;
            NormalBrush.DrawAs = ESlateBrushDrawType::Box;
            NormalBrush.TintColor = ProjectStyle::GetTerminalBlack();
            NormalBrush.Margin = FMargin(0.f);
            TextBoxStyle.BackgroundImageNormal = NormalBrush;
            TextBoxStyle.BackgroundImageHovered = NormalBrush;
            TextBoxStyle.BackgroundImageFocused = NormalBrush;
            TextBoxStyle.BackgroundImageReadOnly = NormalBrush;

            TextBoxStyle.TextStyle.SetFont(ProjectStyle::GetMonoFont(FontSize));
            TextBoxStyle.TextStyle.SetColorAndOpacity(ProjectStyle::GetTerminalWhite());
            TextBoxStyle.Padding = FMargin(4.f, 2.f);

            // Set style and hint text using properties
            TextBox->WidgetStyle = TextBoxStyle;
            TextBox->SetHintText(FText::FromString(PlaceholderText));

            // Set size if specified
            if (Width > 0.f || Height > 0.f)
            {
                USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
                if (SizeBox)
                {
                    if (Width > 0.f)
                    {
                        SizeBox->SetWidthOverride(Width);
                    }
                    if (Height > 0.f)
                    {
                        SizeBox->SetHeightOverride(Height);
                    }
                    SizeBox->SetContent(TextBox);

                    // Get the inner border from the panel
                    UBorder* InnerBorder = Cast<UBorder>(OuterBorder->GetContent());
                    if (InnerBorder)
                    {
                        InnerBorder->SetContent(SizeBox);
                    }
                }
            }
            else
            {
                // Get the inner border from the panel
                UBorder* InnerBorder = Cast<UBorder>(OuterBorder->GetContent());
                if (InnerBorder)
                {
                    InnerBorder->SetContent(TextBox);
                }
            }
        }

        return TextBox;
    }

    UBorder* CreateTerminalModalDialog(
        UWidgetTree* WidgetTree,
        UWidget* Content,
        float Width,
        float Height,
        const FMargin& OuterPadding,
        const FMargin& InnerPadding,
        const FName& Name)
    {
        if (!WidgetTree || !Content)
        {
            return nullptr;
        }

        // Create terminal-styled border panel
        UBorder* OuterBorder = CreateTerminalBorderedPanel(
            WidgetTree,
            ProjectStyle::GetTerminalWhite(),
            ProjectStyle::GetTerminalBlack(),
            OuterPadding,
            InnerPadding,
            Name.IsNone() ? FName(TEXT("ModalDialogOuter")) : FName(*(Name.ToString() + TEXT("_Outer"))),
            Name.IsNone() ? FName(TEXT("ModalDialogInner")) : FName(*(Name.ToString() + TEXT("_Inner")))
        );

        if (!OuterBorder)
        {
            return nullptr;
        }

        // Wrap content in size box if dimensions specified
        if (Width > 0.f || Height > 0.f)
        {
            USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
            if (SizeBox)
            {
                if (Width > 0.f)
                {
                    SizeBox->SetWidthOverride(Width);
                }
                if (Height > 0.f)
                {
                    SizeBox->SetHeightOverride(Height);
                }
                SizeBox->SetContent(Content);

                // Get the inner border from the panel
                UBorder* InnerBorder = Cast<UBorder>(OuterBorder->GetContent());
                if (InnerBorder)
                {
                    InnerBorder->SetContent(SizeBox);
                }
            }
        }
        else
        {
            // Get the inner border from the panel
            UBorder* InnerBorder = Cast<UBorder>(OuterBorder->GetContent());
            if (InnerBorder)
            {
                InnerBorder->SetContent(Content);
            }
        }

        return OuterBorder;
    }
}

