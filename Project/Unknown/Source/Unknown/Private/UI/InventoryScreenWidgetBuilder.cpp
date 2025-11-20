#include "UI/InventoryScreenWidgetBuilder.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryUIConstants.h"
#include "UI/InventoryListWidget.h"
#include "UI/EquipmentPanelWidget.h"
#include "Inventory/EquipmentComponent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Widgets/SBoxPanel.h"

namespace InventoryScreenWidgetBuilder
{
    void BuildRootLayout(
        UWidgetTree* WidgetTree,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree)
        {
            return;
        }

        // Terminal-style: outer white border, inner opaque black panel
        UBorder* Outer = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
            WidgetTree,
            BorderColor,
            BackgroundColor,
            FMargin(InventoryUIConstants::Padding_Outer),
            FMargin(InventoryUIConstants::Padding_Inner),
            TEXT("OuterBorder"),
            TEXT("RootBorder")
        );
        WidgetTree->RootWidget = Outer;
        
        // Get the inner border (RootBorder) from the helper
        OutRefs.RootBorder = Cast<UBorder>(Outer->GetContent());

        // Ensure we always have some minimum size in case anchors are not applied yet
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinSize"));
        SizeBox->SetWidthOverride(InventoryUIConstants::InventoryScreen_Width);
        SizeBox->SetHeightOverride(InventoryUIConstants::InventoryScreen_Height);
        OutRefs.RootBorder->SetContent(SizeBox);

        OutRefs.RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVBox"));
        SizeBox->SetContent(OutRefs.RootVBox);
    }

    void BuildTopBar(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootVBox,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree || !RootVBox)
        {
            return;
        }

        // Top bar: Title (left) + Volume readout (right)
        UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
        if (UVerticalBoxSlot* TopBarSlot = RootVBox->AddChildToVerticalBox(TopBar))
        {
            TopBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InventoryUIConstants::Padding_TopBar));
        }

        // Title on the left
        OutRefs.TitleText = TerminalWidgetHelpers::CreateTerminalTextBlock(
            WidgetTree,
            TEXT("INVENTORY"),
            InventoryUIConstants::FontSize_Title,
            TextColor,
            TEXT("TitleText")
        );
        OutRefs.TitleText->SetShadowOffset(InventoryUIConstants::ShadowOffset);
        OutRefs.TitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, InventoryUIConstants::ShadowOpacity));
        if (UHorizontalBoxSlot* LeftSlot = TopBar->AddChildToHorizontalBox(OutRefs.TitleText))
        {
            LeftSlot->SetPadding(FMargin(0.f));
            LeftSlot->SetHorizontalAlignment(HAlign_Left);
            LeftSlot->SetVerticalAlignment(VAlign_Center);
        }

        // Volume readout on the right
        OutRefs.VolumeText = TerminalWidgetHelpers::CreateTerminalTextBlock(
            WidgetTree,
            TEXT("Volume: 0 / 0"),
            InventoryUIConstants::FontSize_VolumeReadout,
            TextColor,
            TEXT("VolumeText")
        );
        if (UHorizontalBoxSlot* RightSlot = TopBar->AddChildToHorizontalBox(OutRefs.VolumeText))
        {
            RightSlot->SetPadding(FMargin(0.f));
            RightSlot->SetHorizontalAlignment(HAlign_Right);
            RightSlot->SetVerticalAlignment(VAlign_Center);
            FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; RightSlot->SetSize(FillSize);
        }
    }

    void BuildBodyLayout(
        UWidgetTree* WidgetTree,
        UVerticalBox* RootVBox,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree || !RootVBox)
        {
            return;
        }

        // After the top bar, create a body HBox: Left = Inventory column (header+list+info), Right = Equipment panel
        OutRefs.BodyHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyHBox"));
        if (UVerticalBoxSlot* BodySlot = RootVBox->AddChildToVerticalBox(OutRefs.BodyHBox))
        {
            BodySlot->SetPadding(FMargin(0.f));
            BodySlot->SetHorizontalAlignment(HAlign_Fill);
            BodySlot->SetVerticalAlignment(VAlign_Fill);
            FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; BodySlot->SetSize(FillSize);
        }
    }

    void BuildInventoryColumn(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree || !LeftColumnVBox)
        {
            return;
        }

        // Header row with column titles goes into left column
        BuildHeaderRow(WidgetTree, LeftColumnVBox, BorderColor, BackgroundColor, TextColor);

        // Aggregated inventory list below the header (in left column)
        if (!OutRefs.InventoryList)
        {
            OutRefs.InventoryList = WidgetTree->ConstructWidget<UInventoryListWidget>(UInventoryListWidget::StaticClass(), TEXT("InventoryList"));
        }
        if (OutRefs.InventoryList)
        {
            LeftColumnVBox->AddChildToVerticalBox(OutRefs.InventoryList);
            // Note: Delegate binding is handled by the widget itself
        }

        // Info section below the inventory list (in left column)
        BuildInfoPanel(WidgetTree, LeftColumnVBox, BorderColor, BackgroundColor, TextColor, OutRefs);
    }

    void BuildEquipmentPanel(
        UWidgetTree* WidgetTree,
        UHorizontalBox* BodyHBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        UEquipmentComponent* Equipment,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree || !BodyHBox)
        {
            return;
        }

        // Right equipment panel container
        OutRefs.EquipOuter = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
            WidgetTree,
            BorderColor,
            BackgroundColor,
            FMargin(InventoryUIConstants::Padding_Cell),
            FMargin(InventoryUIConstants::Padding_InnerTiny),
            TEXT("EquipOuter"),
            TEXT("EquipInner")
        );
        if (UHorizontalBoxSlot* RightSlot = BodyHBox->AddChildToHorizontalBox(OutRefs.EquipOuter))
        {
            RightSlot->SetPadding(FMargin(0.f));
            RightSlot->SetHorizontalAlignment(HAlign_Fill);
            RightSlot->SetVerticalAlignment(VAlign_Fill);
            RightSlot->SetSize(ESlateSizeRule::Automatic);
        }
        UBorder* EquipInner = Cast<UBorder>(OutRefs.EquipOuter->GetContent());
        
        // Create the panel
        OutRefs.EquipmentPanel = WidgetTree->ConstructWidget<UEquipmentPanelWidget>(UEquipmentPanelWidget::StaticClass(), TEXT("EquipmentPanel"));
        EquipInner->SetContent(OutRefs.EquipmentPanel);
        if (OutRefs.EquipmentPanel)
        {
            OutRefs.EquipmentPanel->SetEquipmentComponent(Equipment);
        }
    }

    void BuildHeaderRow(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor)
    {
        if (!WidgetTree || !LeftColumnVBox)
        {
            return;
        }

        // Header row with column titles goes into left column
        UBorder* HeaderOuter = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
            WidgetTree,
            BorderColor,
            BackgroundColor,
            FMargin(InventoryUIConstants::Padding_Cell),
            FMargin(0.f),
            TEXT("HeaderOuter"),
            TEXT("HeaderInner")
        );
        if (UVerticalBoxSlot* HeaderSlot = LeftColumnVBox->AddChildToVerticalBox(HeaderOuter))
        {
            HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InventoryUIConstants::Padding_Header));
        }
        UBorder* HeaderInner = Cast<UBorder>(HeaderOuter->GetContent());

        UHorizontalBox* HeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderHBox"));
        HeaderInner->SetContent(HeaderHBox);

        auto MakeHeaderCell = [&](float Width, const TCHAR* Label)
        {
            TerminalWidgetHelpers::CreateTerminalHeaderCell(
                WidgetTree,
                HeaderHBox,
                Width,
                Label,
                BorderColor,
                BackgroundColor,
                TextColor
            );
        };

        // Columns: Icon, Name, Qty, Mass, Volume (match widths used by row widgets)
        MakeHeaderCell(InventoryUIConstants::ColumnWidth_Icon, TEXT(" "));
        MakeHeaderCell(InventoryUIConstants::ColumnWidth_Name, TEXT("Name"));
        MakeHeaderCell(InventoryUIConstants::ColumnWidth_Quantity, TEXT("Qty"));
        MakeHeaderCell(InventoryUIConstants::ColumnWidth_Mass, TEXT("Mass"));
        MakeHeaderCell(InventoryUIConstants::ColumnWidth_Volume, TEXT("Volume"));
    }

    void BuildInfoPanel(
        UWidgetTree* WidgetTree,
        UVerticalBox* LeftColumnVBox,
        const FLinearColor& BorderColor,
        const FLinearColor& BackgroundColor,
        const FLinearColor& TextColor,
        FWidgetReferences& OutRefs)
    {
        if (!WidgetTree || !LeftColumnVBox)
        {
            return;
        }

        // Outer white outline
        OutRefs.InfoOuterBorder = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
            WidgetTree,
            BorderColor,
            BackgroundColor,
            FMargin(InventoryUIConstants::Padding_Cell),
            FMargin(InventoryUIConstants::Padding_InfoPanel),
            TEXT("InfoOuter"),
            TEXT("InfoInner")
        );
        if (UVerticalBoxSlot* InfoOuterSlot = LeftColumnVBox->AddChildToVerticalBox(OutRefs.InfoOuterBorder))
        {
            InfoOuterSlot->SetPadding(FMargin(0.f, InventoryUIConstants::Padding_InfoPanel, 0.f, 0.f));
            InfoOuterSlot->SetHorizontalAlignment(HAlign_Fill);
        }
        
        // Get the inner border from the helper
        OutRefs.InfoInnerBorder = Cast<UBorder>(OutRefs.InfoOuterBorder->GetContent());

        // Layout: Icon (left) + Name/Description (right)
        UHorizontalBox* InfoHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InfoHBox"));
        OutRefs.InfoInnerBorder->SetContent(InfoHBox);

        // Icon image with fixed size
        {
            UBorder* IconBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InfoIconBorder"));
            FSlateBrush IB; IB.DrawAs = ESlateBrushDrawType::Box; IB.TintColor = BorderColor; IB.Margin = FMargin(0.f);
            IconBorder->SetBrush(IB);
            IconBorder->SetPadding(FMargin(InventoryUIConstants::Padding_Cell));

            if (UHorizontalBoxSlot* IconSlot = InfoHBox->AddChildToHorizontalBox(IconBorder))
            {
                IconSlot->SetPadding(FMargin(0.f, 0.f, InventoryUIConstants::Padding_InnerSmall, 0.f));
                IconSlot->SetVerticalAlignment(VAlign_Top);
                IconSlot->SetHorizontalAlignment(HAlign_Left);
                IconSlot->SetSize(ESlateSizeRule::Automatic);
            }

            USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoIconSize"));
            IconSizeBox->SetWidthOverride(InventoryUIConstants::IconSize_InfoPanel);
            IconSizeBox->SetHeightOverride(InventoryUIConstants::IconSize_InfoPanel);

            OutRefs.InfoIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InfoIcon"));
            IconSizeBox->SetContent(OutRefs.InfoIcon);
            IconBorder->SetContent(IconSizeBox);
        }

        // Texts
        {
            UVerticalBox* InfoTexts = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InfoTexts"));
            if (UHorizontalBoxSlot* TextsSlot = InfoHBox->AddChildToHorizontalBox(InfoTexts))
            {
                TextsSlot->SetPadding(FMargin(0.f));
                TextsSlot->SetVerticalAlignment(VAlign_Fill);
                FSlateChildSize Fill; Fill.SizeRule = ESlateSizeRule::Fill; TextsSlot->SetSize(Fill);
            }

            OutRefs.InfoNameText = TerminalWidgetHelpers::CreateTerminalTextBlock(
                WidgetTree,
                TEXT(""),
                InventoryUIConstants::FontSize_InfoName,
                TextColor,
                TEXT("InfoName")
            );
            if (UVerticalBoxSlot* NameSlot = InfoTexts->AddChildToVerticalBox(OutRefs.InfoNameText))
            {
                NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InventoryUIConstants::Padding_Cell));
            }

            OutRefs.InfoDescText = TerminalWidgetHelpers::CreateTerminalTextBlock(
                WidgetTree,
                TEXT(""),
                InventoryUIConstants::FontSize_InfoDesc,
                TextColor,
                TEXT("InfoDesc")
            );
            OutRefs.InfoDescText->SetAutoWrapText(true);
            InfoTexts->AddChildToVerticalBox(OutRefs.InfoDescText);
        }

        // Hidden until something is hovered
        if (OutRefs.InfoOuterBorder)
        {
            OutRefs.InfoOuterBorder->SetVisibility(ESlateVisibility::Collapsed);
        }
    }
}

