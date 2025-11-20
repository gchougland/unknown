#include "UI/InventoryScreenWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
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
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "UI/InventoryListWidget.h"
#include "UI/EquipmentPanelWidget.h"
#include "UI/ProjectStyle.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryContextMenu/InventoryContextMenuBuilder.h"
#include "UI/ItemIconHelper.h"
#include "UI/ComponentResolver.h"
#include "UI/InventoryUIConstants.h"
#include "UI/InventoryScreenWidgetBuilder.h"
#include "Icons/ItemIconSubsystem.h"

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

    // Build UI structure using builder functions
    InventoryScreenWidgetBuilder::FWidgetReferences Refs;
    
    // Build root layout
    InventoryScreenWidgetBuilder::BuildRootLayout(WidgetTree, Border, Background, Refs);
    RootBorder = Refs.RootBorder;
    RootVBox = Refs.RootVBox;

    // Build top bar
    InventoryScreenWidgetBuilder::BuildTopBar(WidgetTree, RootVBox, Text, Refs);
    TitleText = Refs.TitleText;
    VolumeText = Refs.VolumeText;
    UpdateVolumeReadout();

    // Build body layout
    InventoryScreenWidgetBuilder::BuildBodyLayout(WidgetTree, RootVBox, Refs);
    
    // Left inventory column
    UVerticalBox* LeftColumnVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryColumnVBox"));
    InventoryColumnVBox = LeftColumnVBox;
    if (UHorizontalBoxSlot* LeftSlot = Refs.BodyHBox->AddChildToHorizontalBox(LeftColumnVBox))
    {
        LeftSlot->SetPadding(FMargin(0.f, 0.f, InventoryUIConstants::Padding_InnerSmall, 0.f));
        LeftSlot->SetHorizontalAlignment(HAlign_Fill);
        LeftSlot->SetVerticalAlignment(VAlign_Fill);
        FSlateChildSize FillSize; FillSize.SizeRule = ESlateSizeRule::Fill; LeftSlot->SetSize(FillSize);
    }

    // Build inventory column (header, list, info)
    InventoryScreenWidgetBuilder::BuildInventoryColumn(WidgetTree, LeftColumnVBox, Border, Background, Text, Refs);
    InventoryList = Refs.InventoryList;
    if (InventoryList)
    {
        // Subscribe to row context requests (RMB) — ensure we don't double-bind across rebuilds
        InventoryList->OnRowContextRequested.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleRowContextRequested);
        InventoryList->OnRowHovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowHovered);
        InventoryList->OnRowUnhovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowUnhovered);
    }
    InfoOuterBorder = Refs.InfoOuterBorder;
    InfoInnerBorder = Refs.InfoInnerBorder;
    InfoIcon = Refs.InfoIcon;
    InfoNameText = Refs.InfoNameText;
    InfoDescText = Refs.InfoDescText;
    ClearInfoPanel();

    // Build equipment panel
    InventoryScreenWidgetBuilder::BuildEquipmentPanel(WidgetTree, Refs.BodyHBox, Border, Background, Equipment, Refs);
    EquipmentPanel = Refs.EquipmentPanel;
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
    UpdateInfoText(Def);
    UpdateInfoIcon(Def);
}

void UInventoryScreenWidget::UpdateInfoText(UItemDefinition* Def)
{
    if (!Def)
    {
        return;
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
}

void UInventoryScreenWidget::UpdateInfoIcon(UItemDefinition* Def)
{
    if (!InfoIcon || !Def)
    {
        return;
    }

    const FVector2D IconSize = InventoryUIConstants::IconSize_InfoPanel_Vec;
    const FItemIconStyle Style = ItemIconHelper::CreateIconStyle();
    
    // Try synchronous load first
    UTexture2D* IconTex = ItemIconHelper::LoadIconSync(Def, Style);
    if (IconTex)
    {
        ItemIconHelper::ApplyIconToImage(InfoIcon, IconTex, IconSize);
    }
    else
    {
        // Clear icon while waiting
        ItemIconHelper::ApplyIconToImage(InfoIcon, nullptr, IconSize);
        
        // Request async load
        TWeakObjectPtr<UInventoryScreenWidget> WeakThis(this);
        TWeakObjectPtr<UImage> WeakImage(InfoIcon);
        UItemDefinition* RequestedDef = Def;
        FOnItemIconReady Callback = FOnItemIconReady::CreateLambda([WeakThis, WeakImage, RequestedDef, IconSize](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
        {
            if (!WeakThis.IsValid() || !WeakImage.IsValid() || ReadyTex == nullptr)
            {
                return;
            }
            // Ensure we're still showing the same item
            if (WeakThis->CurrentHoverDef != RequestedDef)
            {
                return;
            }
            ItemIconHelper::ApplyIconToImage(WeakImage.Get(), ReadyTex, IconSize);
        });
        ItemIconHelper::LoadIconAsync(Def, Style, MoveTemp(Callback));
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
    ResolveComponents(InInventory, InStorage);
    BindEvents();
    InitializeUI();
    PrewarmIcons();

    // Ensure keyboard focus so we can receive Tab/I/Escape
    UWidgetBlueprintLibrary::SetFocusToGameViewport();
    SetKeyboardFocus();
}

void UInventoryScreenWidget::ResolveComponents(UInventoryComponent* InInventory, UStorageComponent* InStorage)
{
    // Prefer the explicitly provided inventory; otherwise, try to auto-resolve from the owning pawn
    Inventory = InInventory;
    if (!Inventory)
    {
        Inventory = ComponentResolver::ResolveInventoryComponent(GetOwningPlayer());
    }
    Storage = InStorage;

    // Resolve Equipment component from the owning pawn (if available)
    Equipment = ComponentResolver::ResolveEquipmentComponent(GetOwningPlayer());
}

void UInventoryScreenWidget::BindEvents()
{
    // Bind to equipment events so the screen updates immediately on equip/unequip
    if (Equipment)
    {
        Equipment->OnItemEquipped.RemoveAll(this);
        Equipment->OnItemUnequipped.RemoveAll(this);
        Equipment->OnItemEquipped.AddDynamic(this, &UInventoryScreenWidget::HandleEquipmentEquipped);
        Equipment->OnItemUnequipped.AddDynamic(this, &UInventoryScreenWidget::HandleEquipmentUnequipped);
    }
}

void UInventoryScreenWidget::InitializeUI()
{
    bOpen = true;
    SetVisibility(ESlateVisibility::Visible);

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
}

void UInventoryScreenWidget::PrewarmIcons()
{
    // Prewarm runtime item icons for visible/known items to minimize pop-in
    const FItemIconStyle Style = ItemIconHelper::CreateIconStyle();
    
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
        ItemIconHelper::PrewarmIcons(UniqueDefs, Style);
    }
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
        Inventory = ComponentResolver::ResolveInventoryComponent(GetOwningPlayer());
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
    InventoryContextMenuBuilder::BuildContextMenu(this, ItemType, ScreenPosition);
}
