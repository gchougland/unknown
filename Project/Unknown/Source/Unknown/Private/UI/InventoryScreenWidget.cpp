#include "UI/InventoryScreenWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/EquipmentComponent.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Blueprint/WidgetTree.h"
#include "UI/InventoryListWidget.h"
#include "UI/EquipmentPanelWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "UI/ProjectStyle.h"
#include "UI/MessageLogSubsystem.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Templates/Function.h"
#include "Widgets/Input/SMenuAnchor.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Layout/SBox.h"
#include "Containers/Ticker.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Components/PrimitiveComponent.h"
#include "Player/FirstPersonCharacter.h"
#include "Inventory/InventoryComponent.h"
// Runtime item icon subsystem
#include "Icons/ItemIconSubsystem.h"
#include "Icons/ItemIconSettings.h"
#include "Icons/ItemIconTypes.h"

TSharedRef<SWidget> UInventoryScreenWidget::RebuildWidget()
{
    // Build the widget tree here so SObjectWidget doesn’t fall back to SSpacer
    if (WidgetTree)
    {
        RebuildUI();
    }
    return Super::RebuildWidget();
}

void UInventoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] NativeConstruct (this=%s)"), *GetName());
    SetVisibility(ESlateVisibility::Collapsed);
    SetIsFocusable(true);
    UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Constructed; RootWidget=%s Visible=%d"), *GetNameSafe(WidgetTree ? WidgetTree->RootWidget : nullptr), GetVisibility() == ESlateVisibility::Visible);
}

void UInventoryScreenWidget::NativeDestruct()
{
    // Proactively unbind any dynamic delegates we added to avoid duplicates if the widget is reconstructed
    if (InventoryList)
    {
        InventoryList->OnRowContextRequested.RemoveAll(this);
        InventoryList->OnRowHovered.RemoveAll(this);
        InventoryList->OnRowUnhovered.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UInventoryScreenWidget::SetTerminalStyle(const FLinearColor& InBackground, const FLinearColor& InBorder, const FLinearColor& InText)
{
    Background = InBackground;
    Border = InBorder;
    Text = InText;
    Refresh();
}

void UInventoryScreenWidget::RefreshInventoryView()
{
    Refresh();
}

void UInventoryScreenWidget::RebuildUI()
{
    if (!WidgetTree)
    {
        return;
    }

    // Terminal-style: outer white border, inner opaque black panel
    UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("OuterBorder"));
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Box;
        Brush.TintColor = Border;
        Brush.Margin = FMargin(0.f);
        Outer->SetBrush(Brush);
    }
    Outer->SetPadding(FMargin(2.f));
    WidgetTree->RootWidget = Outer;

    RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RootBorder"));
    {
        FSlateBrush InnerBrush;
        InnerBrush.DrawAs = ESlateBrushDrawType::Box;
        InnerBrush.TintColor = Background;
        InnerBrush.Margin = FMargin(0.f);
        RootBorder->SetBrush(InnerBrush);
    }
    RootBorder->SetPadding(FMargin(16.f));
    Outer->SetContent(RootBorder);

    // Ensure we always have some minimum size in case anchors are not applied yet
    USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinSize"));
    SizeBox->SetWidthOverride(800.f);
    SizeBox->SetHeightOverride(480.f);
    RootBorder->SetContent(SizeBox);

    RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVBox"));
    SizeBox->SetContent(RootVBox);

    // Top bar: Title (left) + Volume readout (right)
    UHorizontalBox* TopBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBar"));
    if (UVerticalBoxSlot* TopBarSlot = RootVBox->AddChildToVerticalBox(TopBar))
    {
        TopBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
    }

    // Title on the left
    TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
    TitleText->SetText(FText::FromString(TEXT("INVENTORY")));
    TitleText->SetColorAndOpacity(Text);
    TitleText->SetFont(ProjectStyle::GetMonoFont(28));
    TitleText->SetShadowOffset(FVector2D(1.f, 1.f));
    TitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
    if (UHorizontalBoxSlot* LeftSlot = TopBar->AddChildToHorizontalBox(TitleText))
    {
        LeftSlot->SetPadding(FMargin(0.f));
        LeftSlot->SetHorizontalAlignment(HAlign_Left);
        LeftSlot->SetVerticalAlignment(VAlign_Center);
    }

    // Volume readout on the right
    VolumeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("VolumeText"));
    VolumeText->SetText(FText::FromString(TEXT("Volume: 0 / 0")));
    VolumeText->SetColorAndOpacity(Text);
    VolumeText->SetFont(ProjectStyle::GetMonoFont(14));
    if (UHorizontalBoxSlot* RightSlot = TopBar->AddChildToHorizontalBox(VolumeText))
    {
        RightSlot->SetPadding(FMargin(0.f));
        RightSlot->SetHorizontalAlignment(HAlign_Right);
        RightSlot->SetVerticalAlignment(VAlign_Center);
        FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; RightSlot->SetSize(FillSize);
    }

    // Ensure the freshly constructed readout shows current values
    UpdateVolumeReadout();

    // After the top bar, create a body HBox: Left = Inventory column (header+list+info), Right = Equipment panel
    UHorizontalBox* BodyHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BodyHBox"));
    if (UVerticalBoxSlot* BodySlot = RootVBox->AddChildToVerticalBox(BodyHBox))
    {
        BodySlot->SetPadding(FMargin(0.f));
        BodySlot->SetHorizontalAlignment(HAlign_Fill);
        BodySlot->SetVerticalAlignment(VAlign_Fill);
        FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; BodySlot->SetSize(FillSize);
    }

    // Left inventory column
    UVerticalBox* LeftColumnVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryColumnVBox"));
    if (UHorizontalBoxSlot* LeftSlot = BodyHBox->AddChildToHorizontalBox(LeftColumnVBox))
    {
        LeftSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
        LeftSlot->SetHorizontalAlignment(HAlign_Fill);
        LeftSlot->SetVerticalAlignment(VAlign_Fill);
        FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; LeftSlot->SetSize(FillSize);
    }

    // Right equipment panel container
    UBorder* EquipOuter = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EquipOuter"));
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Border; Brush.Margin = FMargin(0.f);
        EquipOuter->SetBrush(Brush);
        EquipOuter->SetPadding(FMargin(1.f));
    }
    if (UHorizontalBoxSlot* RightSlot = BodyHBox->AddChildToHorizontalBox(EquipOuter))
    {
        RightSlot->SetPadding(FMargin(0.f));
        RightSlot->SetHorizontalAlignment(HAlign_Fill);
        RightSlot->SetVerticalAlignment(VAlign_Fill);
        RightSlot->SetSize(ESlateSizeRule::Automatic);
    }
    UBorder* EquipInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EquipInner"));
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Background; Brush.Margin = FMargin(0.f);
        EquipInner->SetBrush(Brush);
        EquipInner->SetPadding(FMargin(6.f));
    }
    EquipOuter->SetContent(EquipInner);
    // Create the panel
    EquipmentPanel = WidgetTree->ConstructWidget<UEquipmentPanelWidget>(UEquipmentPanelWidget::StaticClass(), TEXT("EquipmentPanel"));
    EquipInner->SetContent(EquipmentPanel);
    if (EquipmentPanel)
    {
        EquipmentPanel->SetEquipmentComponent(Equipment);
    }

    // Header row with column titles goes into left column
    UBorder* HeaderOuter = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HeaderOuter"));
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Border; Brush.Margin = FMargin(0.f);
        HeaderOuter->SetBrush(Brush);
        HeaderOuter->SetPadding(FMargin(1.f));
    }
    if (UVerticalBoxSlot* HeaderSlot = LeftColumnVBox->AddChildToVerticalBox(HeaderOuter))
    {
        HeaderSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
    }

    UBorder* HeaderInner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("HeaderInner"));
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Background; Brush.Margin = FMargin(0.f);
        HeaderInner->SetBrush(Brush);
        HeaderInner->SetPadding(FMargin(0.f));
    }
    HeaderOuter->SetContent(HeaderInner);

    UHorizontalBox* HeaderHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderHBox"));
    HeaderInner->SetContent(HeaderHBox);

    auto MakeHeaderCell = [&](float Width, const TCHAR* Label)
    {
        UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        FSlateBrush B; B.DrawAs = ESlateBrushDrawType::Box; B.TintColor = Border; B.Margin = FMargin(0.f);
        CellBorder->SetBrush(B);
        CellBorder->SetPadding(FMargin(0.f));
        if (UHorizontalBoxSlot* Slot = HeaderHBox->AddChildToHorizontalBox(CellBorder))
        {
            Slot->SetPadding(FMargin(1.f, 0.f));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
        USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
        Size->SetWidthOverride(Width);
        Size->SetMinDesiredHeight(28.f);
        UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
        FSlateBrush IB; IB.DrawAs = ESlateBrushDrawType::Box; IB.TintColor = Background; IB.Margin = FMargin(0.f);
        Inner->SetBrush(IB);
        Inner->SetPadding(FMargin(6.f, 2.f));
        Inner->SetHorizontalAlignment(HAlign_Fill);
        Inner->SetVerticalAlignment(VAlign_Center);
        UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
        TextBlock->SetText(FText::FromString(Label));
        TextBlock->SetColorAndOpacity(Text);
        TextBlock->SetFont(ProjectStyle::GetMonoFont(12));
        Inner->SetContent(TextBlock);
        Size->SetContent(Inner);
        CellBorder->SetContent(Size);
    };

    // Columns: Icon, Name, Qty, Mass, Volume (match widths used by row widgets)
    MakeHeaderCell(56.f, TEXT(" "));
    MakeHeaderCell(420.f, TEXT("Name"));
    MakeHeaderCell(80.f, TEXT("Qty"));
    MakeHeaderCell(100.f, TEXT("Mass"));
    MakeHeaderCell(100.f, TEXT("Volume"));

    // Aggregated inventory list below the header (in left column)
    if (!InventoryList)
    {
        InventoryList = WidgetTree->ConstructWidget<UInventoryListWidget>(UInventoryListWidget::StaticClass(), TEXT("InventoryList"));
    }
    if (InventoryList)
    {
        LeftColumnVBox->AddChildToVerticalBox(InventoryList);
        // Subscribe to row context requests (RMB) — ensure we don't double-bind across rebuilds
        InventoryList->OnRowContextRequested.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleRowContextRequested);
        InventoryList->OnRowHovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowHovered);
        InventoryList->OnRowUnhovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowUnhovered);
    }

    // Info section below the inventory list (in left column)
    {
        // Outer white outline
        InfoOuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InfoOuter"));
        {
            FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Border; Brush.Margin = FMargin(0.f);
            InfoOuterBorder->SetBrush(Brush);
            InfoOuterBorder->SetPadding(FMargin(1.f));
        }
        if (UVerticalBoxSlot* InfoOuterSlot = LeftColumnVBox->AddChildToVerticalBox(InfoOuterBorder))
        {
            InfoOuterSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
            InfoOuterSlot->SetHorizontalAlignment(HAlign_Fill);
        }

        // Inner solid black panel
        InfoInnerBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InfoInner"));
        {
            FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Background; Brush.Margin = FMargin(0.f);
            InfoInnerBorder->SetBrush(Brush);
            InfoInnerBorder->SetPadding(FMargin(8.f));
        }
        InfoOuterBorder->SetContent(InfoInnerBorder);

        // Layout: Icon (left) + Name/Description (right)
        UHorizontalBox* InfoHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InfoHBox"));
        InfoInnerBorder->SetContent(InfoHBox);

        // Icon image with fixed size
        {
            UBorder* IconBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("InfoIconBorder"));
            FSlateBrush IB; IB.DrawAs = ESlateBrushDrawType::Box; IB.TintColor = Border; IB.Margin = FMargin(0.f);
            IconBorder->SetBrush(IB);
            IconBorder->SetPadding(FMargin(1.f));

            if (UHorizontalBoxSlot* IconSlot = InfoHBox->AddChildToHorizontalBox(IconBorder))
            {
                IconSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
                IconSlot->SetVerticalAlignment(VAlign_Top);
                IconSlot->SetHorizontalAlignment(HAlign_Left);
                IconSlot->SetSize(ESlateSizeRule::Automatic);
            }

            USizeBox* IconSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("InfoIconSize"));
            IconSizeBox->SetWidthOverride(64.f);
            IconSizeBox->SetHeightOverride(64.f);

            InfoIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("InfoIcon"));
            IconSizeBox->SetContent(InfoIcon);
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

            InfoNameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoName"));
            InfoNameText->SetFont(ProjectStyle::GetMonoFont(18));
            InfoNameText->SetColorAndOpacity(Text);
            if (UVerticalBoxSlot* NameSlot = InfoTexts->AddChildToVerticalBox(InfoNameText))
            {
                NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
            }

            InfoDescText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("InfoDesc"));
            InfoDescText->SetFont(ProjectStyle::GetMonoFont(12));
            InfoDescText->SetColorAndOpacity(Text);
            InfoDescText->SetAutoWrapText(true);
            InfoTexts->AddChildToVerticalBox(InfoDescText);
        }

        // Hidden until something is hovered
        if (InfoOuterBorder)
        {
            InfoOuterBorder->SetVisibility(ESlateVisibility::Collapsed);
        }
        ClearInfoPanel();
    }
}

void UInventoryScreenWidget::HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition)
{
    OpenContextMenu(ItemType, ScreenPosition);
}

UItemDefinition* UInventoryScreenWidget::GetSelectedItemType() const
{
    return InventoryList ? InventoryList->GetSelectedType() : nullptr;
}

void UInventoryScreenWidget::Refresh()
{
    if (RootBorder)
    {
        RootBorder->SetBrushColor(Background);
    }
    if (TitleText)
    {
        TitleText->SetColorAndOpacity(Text);
    }
    if (VolumeText)
    {
        VolumeText->SetColorAndOpacity(Text);
    }
    // Keep the list up to date
    if (InventoryList)
    {
        InventoryList->SetInventory(Inventory);
        InventoryList->Refresh();
    }
    UpdateVolumeReadout();
    // Re-apply style to info panel
    if (InfoInnerBorder)
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Background; Brush.Margin = FMargin(0.f);
        InfoInnerBorder->SetBrush(Brush);
    }
    if (InfoOuterBorder)
    {
        FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Box; Brush.TintColor = Border; Brush.Margin = FMargin(0.f);
        InfoOuterBorder->SetBrush(Brush);
    }
    if (InfoNameText) { InfoNameText->SetColorAndOpacity(Text); }
    if (InfoDescText) { InfoDescText->SetColorAndOpacity(Text); }
}

void UInventoryScreenWidget::HandleListRowHovered(UItemDefinition* ItemType)
{
    UpdateInfoPanelForDef(ItemType);
}

void UInventoryScreenWidget::HandleListRowUnhovered()
{
    ClearInfoPanel();
}

void UInventoryScreenWidget::UpdateInfoPanelForDef(UItemDefinition* Def)
{
    CurrentHoverDef = Def;
    if (!Def)
    {
        ClearInfoPanel();
        return;
    }
    if (InfoOuterBorder)
    {
        InfoOuterBorder->SetVisibility(ESlateVisibility::Visible);
    }
    if (InfoNameText)
    {
        const FText Name = Def->DisplayName.IsEmpty() ? FText::FromString(Def->GetName()) : Def->DisplayName;
        InfoNameText->SetText(Name);
    }
    if (InfoDescText)
    {
        InfoDescText->SetText(Def->Description);
    }

    if (InfoIcon)
    {
        const FVector2D IconSize(64.f, 64.f);
        if (Def->IconOverride)
        {
            FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(Def->IconOverride); Brush.ImageSize = IconSize;
            InfoIcon->SetBrush(Brush);
            InfoIcon->SetOpacity(1.f);
            InfoIcon->SetVisibility(ESlateVisibility::Visible);
        }
        else if (UItemIconSubsystem* IconSys = UItemIconSubsystem::Get())
        {
            FItemIconStyle Style;
            if (const UItemIconSettings* Settings = UItemIconSettings::Get())
            {
                Style.Resolution = Settings->DefaultResolution;
                Style.Background = Settings->bTransparentBackground ? EItemIconBackground::Transparent : EItemIconBackground::SolidColor;
                Style.SolidColor = Settings->SolidBackgroundColor;
                Style.StyleVersion = Settings->StyleVersion;
            }

            if (UTexture2D* Cached = IconSys->GetIconIfReady(Def, Style))
            {
                FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(Cached); Brush.ImageSize = IconSize;
                InfoIcon->SetBrush(Brush);
                InfoIcon->SetOpacity(1.f);
                InfoIcon->SetVisibility(ESlateVisibility::Visible);
            }
            else
            {
                InfoIcon->SetBrush(FSlateBrush());
                InfoIcon->SetOpacity(0.f);
                InfoIcon->SetVisibility(ESlateVisibility::Visible);

                TWeakObjectPtr<UInventoryScreenWidget> WeakThis(this);
                TWeakObjectPtr<UImage> WeakImage(InfoIcon);
                UItemDefinition* RequestedDef = Def;
                FOnItemIconReady Callback = FOnItemIconReady::CreateLambda([WeakThis, WeakImage, RequestedDef, IconSize](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
                {
                    if (!WeakThis.IsValid() || !WeakImage.IsValid() || ReadyTex == nullptr)
                    {
                        return;
                    }
                    if (WeakThis->CurrentHoverDef != RequestedDef)
                    {
                        return;
                    }
                    FSlateBrush Brush; Brush.DrawAs = ESlateBrushDrawType::Image; Brush.SetResourceObject(ReadyTex); Brush.ImageSize = IconSize;
                    WeakImage->SetBrush(Brush);
                    WeakImage->SetOpacity(1.f);
                    WeakImage->SetVisibility(ESlateVisibility::Visible);
                });
                IconSys->RequestIcon(Def, Style, MoveTemp(Callback));
            }
        }
        else
        {
            InfoIcon->SetBrush(FSlateBrush());
            InfoIcon->SetOpacity(0.f);
            InfoIcon->SetVisibility(ESlateVisibility::Visible);
        }
    }
}

void UInventoryScreenWidget::ClearInfoPanel()
{
    CurrentHoverDef = nullptr;
    if (InfoNameText)
    {
        InfoNameText->SetText(FText());
    }
    if (InfoDescText)
    {
        InfoDescText->SetText(FText());
    }
    if (InfoIcon)
    {
        InfoIcon->SetBrush(FSlateBrush());
        InfoIcon->SetOpacity(0.f);
        InfoIcon->SetVisibility(ESlateVisibility::Visible);
    }
    if (InfoOuterBorder)
    {
        InfoOuterBorder->SetVisibility(ESlateVisibility::Collapsed);
    }
}

// React to equipment changes — keep inventory list, equipment panel, and volume label in sync
void UInventoryScreenWidget::HandleEquipmentEquipped(EEquipmentSlot InEquipSlot, const FItemEntry& Item)
{
    // Refresh the inventory list and details
    Refresh();
    // Ensure the equipment panel shows the new state
    if (EquipmentPanel)
    {
        EquipmentPanel->Refresh();
    }
    // Update the volume readout to reflect any capacity changes (e.g., backpack)
    UpdateVolumeReadout();
}

void UInventoryScreenWidget::HandleEquipmentUnequipped(EEquipmentSlot InEquipSlot, const FItemEntry& Item)
{
    // Refresh the inventory list and details
    Refresh();
    // Ensure the equipment panel shows the new state
    if (EquipmentPanel)
    {
        EquipmentPanel->Refresh();
    }
    // Update the volume readout to reflect any capacity changes
    UpdateVolumeReadout();
}

void UInventoryScreenWidget::Open(UInventoryComponent* InInventory, UStorageComponent* InStorage)
{
    // Prefer the explicitly provided inventory; otherwise, try to auto-resolve from the owning pawn
    Inventory = InInventory;
    if (!Inventory)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
            }
        }
    }
    Storage = InStorage;

    // Resolve Equipment component from the owning pawn (if available)
    Equipment = nullptr;
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            // Prefer direct find to avoid circular header coupling
            Equipment = Pawn->FindComponentByClass<UEquipmentComponent>();
        }
    }
    // Bind to equipment events so the screen updates immediately on equip/unequip
    if (Equipment)
    {
        Equipment->OnItemEquipped.RemoveAll(this);
        Equipment->OnItemUnequipped.RemoveAll(this);
        Equipment->OnItemEquipped.AddDynamic(this, &UInventoryScreenWidget::HandleEquipmentEquipped);
        Equipment->OnItemUnequipped.AddDynamic(this, &UInventoryScreenWidget::HandleEquipmentUnequipped);
    }
    bOpen = true;
    SetVisibility(ESlateVisibility::Visible);

    // Removed temporary HUD message on inventory open

    // If an equipment panel already exists from a prior build, rebind it to the newly resolved Equipment now
    if (EquipmentPanel)
    {
        EquipmentPanel->SetEquipmentComponent(Equipment);
    }

    // Wire the list to the provided inventory and refresh the UI
    if (InventoryList)
    {
        InventoryList->SetInventory(Inventory);
        InventoryList->Refresh();
    }

    Refresh();
    if (EquipmentPanel)
    {
        EquipmentPanel->Refresh();
    }
    UpdateVolumeReadout();

    // Prewarm runtime item icons for visible/known items to minimize pop-in
    if (UItemIconSubsystem* IconSys = UItemIconSubsystem::Get())
    {
        FItemIconStyle Style;
        if (const UItemIconSettings* Settings = UItemIconSettings::Get())
        {
            Style.Resolution = Settings->DefaultResolution;
            Style.Background = Settings->bTransparentBackground ? EItemIconBackground::Transparent : EItemIconBackground::SolidColor;
            Style.SolidColor = Settings->SolidBackgroundColor;
            Style.StyleVersion = Settings->StyleVersion;
        }
        // Collect unique item definitions from inventory and from currently equipped items
        TArray<const UItemDefinition*> UniqueDefs;
        TSet<const UItemDefinition*> Seen;
        if (Inventory)
        {
            for (const FItemEntry& E : Inventory->GetEntries())
            {
                if (E.Def && !Seen.Contains(E.Def))
                {
                    Seen.Add(E.Def);
                    UniqueDefs.Add(E.Def);
                }
            }
        }
        if (Equipment)
        {
            // Iterate all slots
            const EEquipmentSlot Slots[] = { EEquipmentSlot::Head, EEquipmentSlot::Chest, EEquipmentSlot::Hands, EEquipmentSlot::Back, EEquipmentSlot::Primary, EEquipmentSlot::Secondary, EEquipmentSlot::Utility, EEquipmentSlot::Gadget };
            for (EEquipmentSlot S : Slots)
            {
                FItemEntry EquippedEntry;
                if (Equipment->GetEquipped(S, EquippedEntry) && EquippedEntry.Def && !Seen.Contains(EquippedEntry.Def))
                {
                    Seen.Add(EquippedEntry.Def);
                    UniqueDefs.Add(EquippedEntry.Def);
                }
            }
        }
        if (UniqueDefs.Num() > 0)
        {
            IconSys->Prewarm(UniqueDefs, Style);
        }
    }

    // Ensure keyboard focus so we can receive Tab/I/Escape
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    SetKeyboardFocus();
}

void UInventoryScreenWidget::Close()
{
    bOpen = false;
    SetVisibility(ESlateVisibility::Collapsed);
}

FReply UInventoryScreenWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HandleCloseKey(InKeyEvent.GetKey()))
    {
        return FReply::Handled();
    }
    return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

// --- Helpers ---
void UInventoryScreenWidget::UpdateVolumeReadout()
{
    // Always try to resolve the currently visible VolumeText from the widget tree first
    UTextBlock* TargetText = nullptr;
    if (WidgetTree)
    {
        TargetText = Cast<UTextBlock>(WidgetTree->FindWidget(TEXT("VolumeText")));
    }
    // Fallback to cached pointer if not found (e.g., early during construct)
    if (!TargetText)
    {
        TargetText = VolumeText;
    }
    if (!TargetText)
    {
        return;
    }
    // Ensure we have a source inventory; if not, try to auto-resolve one from the owning pawn
    if (!Inventory)
    {
        if (APlayerController* PC = GetOwningPlayer())
        {
            if (APawn* Pawn = PC->GetPawn())
            {
                Inventory = Pawn->FindComponentByClass<UInventoryComponent>();
            }
        }
    }

    float Used = 0.f;
    float Max = 0.f;
    if (Inventory)
    {
        Used = FMath::Max(0.f, Inventory->GetUsedVolume());
        Max = FMath::Max(0.f, Inventory->MaxVolume);
    }
    const FString Str = FString::Printf(TEXT("Volume: %.1f / %.1f"), Used, Max);
    TargetText->SetText(FText::FromString(Str));
    // Cache for next call if we didn't have it
    VolumeText = TargetText;
}

FReply UInventoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HandleCloseKey(InKeyEvent.GetKey()))
    {
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

bool UInventoryScreenWidget::HandleCloseKey(const FKey& Key)
{
    if (Key == EKeys::Tab || Key == EKeys::I || Key == EKeys::Escape)
    {
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Close key pressed: %s"), *Key.ToString());
        OnRequestClose.Broadcast();
        return true;
    }
    return false;
}

void UInventoryScreenWidget::OpenContextMenu(UItemDefinition* ItemType, const FVector2D& ScreenPosition)
{
    if (!ItemType)
    {
        return;
    }

    // Enforce pure black background and pure white borders/text (no transparency)
    const FLinearColor PureBlack(0.f, 0.f, 0.f, 1.f);
    const FLinearColor PureWhite(1.f, 1.f, 1.f, 1.f);

    // Solid box brushes for borders and fills
    static FSlateBrush WhiteBoxBrush;
    static FSlateBrush BlackBoxBrush;
    static bool bInit = false;
    if (!bInit)
    {
        WhiteBoxBrush = FSlateBrush();
        WhiteBoxBrush.DrawAs = ESlateBrushDrawType::Box;
        WhiteBoxBrush.TintColor = PureWhite;
        WhiteBoxBrush.Margin = FMargin(0.f);

        BlackBoxBrush = FSlateBrush();
        BlackBoxBrush.DrawAs = ESlateBrushDrawType::Box;
        BlackBoxBrush.TintColor = PureBlack;
        BlackBoxBrush.Margin = FMargin(0.f);
        bInit = true;
    }

    // Button style that stays solid black even on hover/press
    static FButtonStyle SolidBlackButtonStyle;
    static bool bBtnInit = false;
    if (!bBtnInit)
    {
        SolidBlackButtonStyle = FButtonStyle()
            .SetNormal(BlackBoxBrush)
            .SetHovered(BlackBoxBrush)
            .SetPressed(BlackBoxBrush)
            .SetNormalForeground(PureWhite)
            .SetHoveredForeground(PureWhite)
            .SetPressedForeground(PureWhite)
            .SetDisabledForeground(PureWhite);
        bBtnInit = true;
    }

    // Remember self so we can restore focus after menu clicks
    TWeakObjectPtr<UInventoryScreenWidget> Self = this;

    auto MakeActionRow = [&](const FString& Label, TFunction<void()> OnClick) -> TSharedRef<SWidget>
    {
        TSharedPtr<SButton> Button;
        return SNew(SBorder)
            .BorderImage(&WhiteBoxBrush)
            .BorderBackgroundColor(PureWhite)
            .Padding(0.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SAssignNew(Button, SButton)
                    .ButtonStyle(&SolidBlackButtonStyle)
                    .IsFocusable(false)
                    .ContentPadding(FMargin(8.f, 4.f))
                    .OnClicked_Lambda([OnClick, Self]()
                    {
                        if (OnClick) { OnClick(); }
                        FSlateApplication::Get().DismissAllMenus();
                        if (Self.IsValid()) { UWidgetBlueprintLibrary::SetFocusToGameViewport(); Self->SetKeyboardFocus(); }
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Label))
                        .ColorAndOpacity(PureWhite)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .BorderImage(&WhiteBoxBrush)
                    .BorderBackgroundColor(PureWhite)
                    .Padding(0.f)
                    .Visibility_Lambda([Button]() -> EVisibility { return (Button.IsValid() && Button->IsHovered()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                    [
                        SNew(SBox)
                        .WidthOverride(2.f)
                    ]
                ]
            ];
    };

    // Shared state to reference the Assign submenu anchor safely from multiple rows and control closing
    struct FLocalMenuState 
    { 
        TWeakPtr<SMenuAnchor> AssignAnchor; 
        TWeakPtr<SButton> AssignOpener; 
        TWeakPtr<SWidget> MainMenuRoot; 
        FDelegateHandle TickerHandle; 
    };
    TSharedRef<FLocalMenuState> MenuState = MakeShared<FLocalMenuState>();

    // Helper to wrap a row in our black/white styled border with hover accent
    auto MakeStyledRow = [&](const FString& Label, TFunction<void()> OnClick) -> TSharedRef<SWidget>
    {
        TSharedPtr<SButton> Button;
        return SNew(SBorder)
            .BorderImage(&WhiteBoxBrush)
            .BorderBackgroundColor(PureWhite)
            .Padding(0.f)
            [
                SNew(SOverlay)
                + SOverlay::Slot()
                [
                    SAssignNew(Button, SButton)
                    .ButtonStyle(&SolidBlackButtonStyle)
                    .IsFocusable(false)
                    .ContentPadding(FMargin(8.f, 4.f))
                    .OnHovered_Lambda([State = MenuState]()
                    {
                        // Do NOT auto-close the Assign submenu when hovering other rows.
                        // Previously we called A->SetIsOpen(false) here which closed the fly-out as the mouse moved,
                        // preventing clicks from reaching the submenu. Keep it open; closing is handled by clicks/dismiss.
                        if (TSharedPtr<SMenuAnchor> A = State->AssignAnchor.Pin())
                        {
                            UE_LOG(LogTemp, VeryVerbose, TEXT("[InventoryScreen] Row hover: leaving Assign submenu open"));
                        }
                    })
                    .OnClicked_Lambda([OnClick, Self]()
                    {
                        if (OnClick) { OnClick(); }
                        FSlateApplication::Get().DismissAllMenus();
                        if (Self.IsValid()) { UWidgetBlueprintLibrary::SetFocusToGameViewport(); Self->SetKeyboardFocus(); }
                        return FReply::Handled();
                    })
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(Label))
                        .ColorAndOpacity(PureWhite)
                    ]
                ]
                + SOverlay::Slot()
                .HAlign(HAlign_Left)
                .VAlign(VAlign_Fill)
                [
                    SNew(SBorder)
                    .BorderImage(&WhiteBoxBrush)
                    .BorderBackgroundColor(PureWhite)
                    .Padding(0.f)
                    .Visibility_Lambda([Button]() -> EVisibility { return (Button.IsValid() && Button->IsHovered()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                    [
                        SNew(SBox)
                        .WidthOverride(2.f)
                    ]
                ]
            ];
    };

    // Build a custom-styled fly-out submenu using SMenuAnchor (no engine submenu chrome or MultiBox). We will not use FMenuBuilder at all.
    auto MakeAssignSubmenu = [ItemType, PureWhite, PureBlack, Self]() -> TSharedRef<SWidget>
    {
        // Build submenu rows 1–9 with identical styling
        auto MakeSlotRow = [&](int32 SlotIndex) -> TSharedRef<SWidget>
        {
            TSharedPtr<SButton> Button;
            return SNew(SBorder)
                .BorderImage(&WhiteBoxBrush)
                .BorderBackgroundColor(PureWhite)
                .Padding(0.f)
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SAssignNew(Button, SButton)
                        .ButtonStyle(&SolidBlackButtonStyle)
                        .IsFocusable(false)
                        .ContentPadding(FMargin(8.f, 4.f))
                        .ClickMethod(EButtonClickMethod::MouseDown)
                        .OnPressed_Lambda([ItemType, SlotIndex, Self]()
                        {
                            // Fire on mouse-down to beat SMenuAnchor's dismiss-on-click, ensuring our logic runs.
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign press: ItemType=%s SlotIndex(UI)=%d"), *GetNameSafe(ItemType), SlotIndex);

                            if (!Self.IsValid())
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Self widget invalid (GCed?)"));
                                return;
                            }

                            APlayerController* PC = Self->GetOwningPlayer();
                            if (!PC)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Owning PlayerController is null"));
                                return;
                            }
                            APawn* Pawn = PC->GetPawn();
                            if (!Pawn)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: PC->GetPawn() returned null"));
                                return;
                            }

                            AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
                            if (!Character)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Pawn %s is not AFirstPersonCharacter"), *GetNameSafe(Pawn));
                                return;
                            }

                            UHotbarComponent* HB = Character->GetHotbar();
                            if (!HB)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: Character->GetHotbar() is null"));
                                return;
                            }

                            if (!ItemType)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign press abort: ItemType is null"));
                                return;
                            }

                            const int32 ZeroBased = FMath::Clamp(SlotIndex - 1, 0, 8);
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign press: attempting AssignSlot Index=%d Type=%s"), ZeroBased, *GetNameSafe(ItemType));
                            const bool bOk = HB->AssignSlot(ZeroBased, ItemType);
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign result (press): %s for slot %d (Type=%s)"), bOk ? TEXT("Success") : TEXT("Failure"), ZeroBased, *GetNameSafe(ItemType));

                            // Close menu and restore focus to game viewport
                            FSlateApplication::Get().DismissAllMenus();
                            UWidgetBlueprintLibrary::SetFocusToGameViewport();
                            Self->SetKeyboardFocus();
                        })
                        .OnClicked_Lambda([ItemType, SlotIndex, Self]()
                        {
                            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Assign click: ItemType=%s SlotIndex(UI)=%d"), *GetNameSafe(ItemType), SlotIndex);

                            // Validate captured widget self
                            if (!Self.IsValid())
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Self widget invalid (GCed?)"));
                                return FReply::Handled();
                            }

                            // Resolve PC and Pawn
                            APlayerController* PC = Self->GetOwningPlayer();
                            if (!PC)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Owning PlayerController is null"));
                                return FReply::Handled();
                            }
                            APawn* Pawn = PC->GetPawn();
                            if (!Pawn)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: PC->GetPawn() returned null"));
                                return FReply::Handled();
                            }

                            // Character cast
                            AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
                            if (!Character)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Pawn %s is not AFirstPersonCharacter"), *GetNameSafe(Pawn));
                                return FReply::Handled();
                            }

                            // Hotbar resolution
                            UHotbarComponent* HB = Character->GetHotbar();
                            if (!HB)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: Character->GetHotbar() is null"));
                                return FReply::Handled();
                            }

                            if (!ItemType)
                            {
                                UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Assign click abort: ItemType is null"));
                                return FReply::Handled();
                            }

                            const int32 ZeroBased = FMath::Clamp(SlotIndex - 1, 0, 8);
                            const bool bOk = HB->AssignSlot(ZeroBased, ItemType);
                            
                            // Close menu and restore focus to game viewport
                            FSlateApplication::Get().DismissAllMenus();
                            UWidgetBlueprintLibrary::SetFocusToGameViewport();
                            Self->SetKeyboardFocus();
                            return FReply::Handled();
                        })
                        [
                            SNew(STextBlock)
                            .Text(FText::FromString(FString::Printf(TEXT("Slot %d"), SlotIndex)))
                            .ColorAndOpacity(PureWhite)
                        ]
                    ]
                    + SOverlay::Slot()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Fill)
                    [
                        SNew(SBorder)
                        .BorderImage(&WhiteBoxBrush)
                        .BorderBackgroundColor(PureWhite)
                        .Padding(0.f)
                        .Visibility_Lambda([Button]() -> EVisibility { return (Button.IsValid() && Button->IsHovered()) ? EVisibility::HitTestInvisible : EVisibility::Collapsed; })
                        [
                            SNew(SBox)
                            .WidthOverride(2.f)
                        ]
                    ]
                ];
        };

        TSharedRef<SVerticalBox> SubVBox = SNew(SVerticalBox);
        for (int32 SlotIndex = 1; SlotIndex <= 9; ++SlotIndex)
        {
            SubVBox->AddSlot().AutoHeight().Padding(2.f)
            [ MakeSlotRow(SlotIndex) ];
        }

        return SNew(SBorder)
            .BorderImage(&WhiteBoxBrush)
            .BorderBackgroundColor(PureWhite)
            .Padding(2.f)
            [
                SNew(SBorder)
                .BorderImage(&BlackBoxBrush)
                .BorderBackgroundColor(PureBlack)
                .Padding(6.f)
                [
                    SNew(SBox)
                    .MinDesiredWidth(160.f)
                    [ SubVBox ]
                ]
            ];
    };

    auto MakeAssignRow = [&]() -> TSharedRef<SWidget>
    {
        TSharedPtr<SButton> OpenerButton;
        TSharedPtr<SMenuAnchor> LocalAnchor;

        TSharedRef<SWidget> Row = SNew(SBorder)
            .BorderImage(&WhiteBoxBrush)
            .BorderBackgroundColor(PureWhite)
            .Padding(0.f)
            [
                SAssignNew(LocalAnchor, SMenuAnchor)
                .UseApplicationMenuStack(false) // render inline so our styling is preserved, no engine chrome
                .Method(EPopupMethod::UseCurrentWindow)
                .Placement(MenuPlacement_MenuRight)
                .OnGetMenuContent_Lambda([MakeAssignSubmenu]() -> TSharedRef<SWidget>
                {
                    return MakeAssignSubmenu();
                })
                [
                    SNew(SOverlay)
                    + SOverlay::Slot()
                    [
                        SAssignNew(OpenerButton, SButton)
                        .ButtonStyle(&SolidBlackButtonStyle)
                        .IsFocusable(false)
                        .ContentPadding(FMargin(8.f, 4.f))
                        .OnHovered_Lambda([State = MenuState]()
                        {
                            if (TSharedPtr<SMenuAnchor> A = State->AssignAnchor.Pin())
                            {
                                const bool bWasOpen = A->IsOpen();
                                if (!bWasOpen)
                                {
                                    A->SetIsOpen(true, false);
                                }
                            }
                        })
                        .OnClicked_Lambda([State = MenuState]()
                        {
                            if (TSharedPtr<SMenuAnchor> A = State->AssignAnchor.Pin())
                            {
                                const bool bNew = !A->IsOpen();
                                A->SetIsOpen(bNew, false);
                            }
                            return FReply::Handled();
                        })
                        [
                            SNew(SHorizontalBox)
                            + SHorizontalBox::Slot().AutoWidth()
                            [ SNew(STextBlock).Text(FText::FromString(TEXT("Assign to Hotbar \u25B6"))).ColorAndOpacity(PureWhite) ]
                        ]
                    ]
                    + SOverlay::Slot()
                    .HAlign(HAlign_Left)
                    .VAlign(VAlign_Fill)
                    [
                        SNew(SBorder)
                        .BorderImage(&WhiteBoxBrush)
                        .BorderBackgroundColor(PureWhite)
                        .Padding(0.f)
                        .Visibility_Lambda([OpenerButton, State = MenuState]() -> EVisibility
                        {
                            const bool bHover = (OpenerButton.IsValid() && OpenerButton->IsHovered());
                            const bool bOpen = (State->AssignAnchor.IsValid() && State->AssignAnchor.Pin()->IsOpen());
                            return (bHover || bOpen) ? EVisibility::HitTestInvisible : EVisibility::Collapsed;
                        })
                        [ SNew(SBox).WidthOverride(2.f) ]
                    ]
                ]
            ];

        // Share weak reference so other rows can close it on hover
        MenuState->AssignAnchor = LocalAnchor;
        return Row;
    };

    // Build our menu vertically (no FMenuBuilder / SMultiBoxWidget)
    TSharedRef<SVerticalBox> MenuVBox = SNew(SVerticalBox);

    // Top-level actions
    MenuVBox->AddSlot().AutoHeight().Padding(2.f)[ MakeStyledRow(TEXT("Use"), [ItemType]()
    {
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Use requested: %s"), *GetNameSafe(ItemType));
    }) ];

    MenuVBox->AddSlot().AutoHeight().Padding(2.f)[ MakeStyledRow(TEXT("Hold"), [this, ItemType]()
    {
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Hold requested: %s"), *GetNameSafe(ItemType));
        if (!ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: ItemType is null"));
            return;
        }
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: Inventory is null"));
            return;
        }

        // Get the character
        APlayerController* PC = GetOwningPlayer();
        if (!PC)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: PlayerController is null"));
            return;
        }

        APawn* Pawn = PC->GetPawn();
        AFirstPersonCharacter* Character = Cast<AFirstPersonCharacter>(Pawn);
        if (!Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: Pawn is not a FirstPersonCharacter"));
            return;
        }

        // Find the first item entry of this type in the inventory
        FItemEntry ItemToHold;
        bool bFound = false;
        for (const FItemEntry& Entry : Inventory->GetEntries())
        {
            if (Entry.Def == ItemType)
            {
                ItemToHold = Entry;
                bFound = true;
                break;
            }
        }

        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold aborted: No item of type %s found in inventory"), *GetNameSafe(ItemType));
            return;
        }

        // Hold the item (this will spawn the actor and attach it to the socket)
        if (Character->HoldItem(ItemToHold))
        {
            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Successfully holding %s"), *GetNameSafe(ItemType));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Hold failed: HoldItem returned false"));
        }
    }) ];

    // EQUIP (only for equippable items and only from the player's inventory panel)
    if (ItemType && ItemType->bEquippable)
    {
        MenuVBox->AddSlot().AutoHeight().Padding(2.f)[ MakeStyledRow(TEXT("Equip"), [this, ItemType]()
        {
            UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Equip requested: %s"), *GetNameSafe(ItemType));
            if (!ItemType || !Inventory)
            {
                UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: missing ItemType or Inventory"));
                return;
            }
            // Resolve Equipment (prefer cached; else attempt to find on pawn)
            UEquipmentComponent* EquipComp = Equipment;
            if (!EquipComp)
            {
                if (APlayerController* PC = GetOwningPlayer())
                {
                    if (APawn* Pawn = PC->GetPawn())
                    {
                        EquipComp = Pawn->FindComponentByClass<UEquipmentComponent>();
                    }
                }
            }
            if (!EquipComp)
            {
                UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: EquipmentComponent not found on pawn"));
                return;
            }

            // Find one entry of this type to equip
            FGuid ItemIdToEquip;
            for (const FItemEntry& E : Inventory->GetEntries())
            {
                if (E.Def == ItemType)
                {
                    ItemIdToEquip = E.ItemId;
                    break;
                }
            }
            if (!ItemIdToEquip.IsValid())
            {
                UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip aborted: no entry of type %s in inventory"), *GetNameSafe(ItemType));
                return;
            }

            FText Error;
            if (EquipComp->EquipFromInventory(ItemIdToEquip, Error))
            {
                UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Equipped %s"), *GetNameSafe(ItemType));
                // Refresh inventory view and volume readout; equipment panel will also need refresh when present
                Refresh();
                UpdateVolumeReadout();
                if (EquipmentPanel)
                {
                    EquipmentPanel->Refresh();
                }
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Equip failed: %s"), *Error.ToString());
            }
        }) ];
    }

    MenuVBox->AddSlot().AutoHeight().Padding(2.f)[ MakeStyledRow(TEXT("Drop"), [this, ItemType]()
    {
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Drop requested: %s"), *GetNameSafe(ItemType));
        if (!ItemType)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: ItemType is null"));
            return;
        }
        if (!Inventory)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: Inventory is null"));
            return;
        }

        // Find one entry of this type to remove
        FItemEntry EntryToDrop;
        bool bFound = false;
        for (const FItemEntry& E : Inventory->GetEntries())
        {
            if (E.Def == ItemType)
            {
                EntryToDrop = E;
                bFound = true;
                break;
            }
        }
        if (!bFound)
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop aborted: No entry of type %s in inventory"), *GetNameSafe(ItemType));
            return;
        }

        // Remove from inventory first
        if (!Inventory->RemoveById(EntryToDrop.ItemId))
        {
            UE_LOG(LogTemp, Warning, TEXT("[InventoryScreen] Drop failed: RemoveById(%s) returned false"), *EntryToDrop.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
            return;
        }

        UWorld* World = GetWorld();
        if (!World)
        {
            UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Drop error: World is null; restoring item to inventory"));
            Inventory->TryAdd(EntryToDrop);
            Refresh();
            return;
        }

        APlayerController* PC = GetOwningPlayer();
        APawn* Pawn = PC ? PC->GetPawn() : nullptr;

        FActorSpawnParameters Params;
        // Do not spawn if colliding; we will roll back the inventory removal to avoid item loss
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

        const AActor* Context = Pawn ? static_cast<AActor*>(Pawn) : static_cast<AActor*>(PC);
        AItemPickup* Pickup = AItemPickup::SpawnDropped(World, Context, ItemType, Params);
        if (!Pickup)
        {
            UE_LOG(LogTemp, Error, TEXT("[InventoryScreen] Failed to spawn AItemPickup at safe location; restoring item to inventory"));
            Inventory->TryAdd(EntryToDrop);
            Refresh();
            return;
        }
        UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] Dropped one %s at %s"), *GetNameSafe(ItemType), *Pickup->GetActorLocation().ToCompactString());

        // Update UI
        Refresh();
    }) ];

    // Note: "Equip" action is added earlier only when the item is equippable (ItemType->bEquippable).
    // Do not add a generic Equip here to avoid duplicate menu entries.

    // Assign to Hotbar (hover to open submenu)
    MenuVBox->AddSlot().AutoHeight().Padding(2.f)[ MakeAssignRow() ];

    // Wrap menu vbox with our black/white frame
    TSharedRef<SWidget> MenuWidget = SNew(SBorder)
        .BorderImage(&WhiteBoxBrush)
        .BorderBackgroundColor(PureWhite)
        .Padding(1.f)
        [
            SNew(SBorder)
            .BorderImage(&BlackBoxBrush)
            .BorderBackgroundColor(PureBlack)
            .Padding(6.f)
            [ MenuVBox ]
        ];

    if (TSharedPtr<SWidget> Cached = GetCachedWidget())
    {
        FSlateApplication::Get().PushMenu(
            Cached.ToSharedRef(),
            FWidgetPath(),
            MenuWidget,
            ScreenPosition,
            FPopupTransitionEffect(FPopupTransitionEffect::ContextMenu)
        );
    }
}
