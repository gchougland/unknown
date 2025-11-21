#include "UI/InventoryScreenWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/EquipmentComponent.h"
#include "UI/StorageWindowWidget.h"
#include "UI/StorageListWidget.h"
#include "UI/MessageLogSubsystem.h"
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
#include "Engine/Engine.h"

TSharedRef<SWidget> UInventoryScreenWidget::RebuildWidget()
{
    // Build the widget tree here so SObjectWidget doesn't fall back to SSpacer
    if (WidgetTree && !bUIBuilt)
    {
        RebuildUI();
        bUIBuilt = true;
    }
    return Super::RebuildWidget();
}

void UInventoryScreenWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] NativeConstruct (this=%s)"), *GetName());
    SetVisibility(ESlateVisibility::Collapsed);
    SetIsFocusable(true);
}

void UInventoryScreenWidget::NativeDestruct()
{
    Super::NativeDestruct();
}

void UInventoryScreenWidget::RebuildUI()
{
    if (!WidgetTree)
    {
        return;
    }

    // Build UI structure using builder functions - only called once during initial construction
    InventoryScreenWidgetBuilder::FWidgetReferences Refs;
    
    // Build root layout
    InventoryScreenWidgetBuilder::BuildRootLayout(WidgetTree, Border, Background, Refs);
    RootBorder = Refs.RootBorder;
    RootVBox = Refs.RootVBox;

    // Build top bar
    InventoryScreenWidgetBuilder::BuildTopBar(WidgetTree, RootVBox, Text, Refs);
    TitleText = Refs.TitleText;
    VolumeText = Refs.VolumeText;

    // Build body layout
    InventoryScreenWidgetBuilder::BuildBodyLayout(WidgetTree, RootVBox, Refs);
    UHorizontalBox* BodyHBox = Refs.BodyHBox;
    
    // Build inventory column (left side, takes up available space)
    UVerticalBox* LeftColumnVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("InventoryColumnVBox"));
    InventoryColumnVBox = LeftColumnVBox;
    if (UHorizontalBoxSlot* InventorySlot = BodyHBox->AddChildToHorizontalBox(LeftColumnVBox))
    {
        InventorySlot->SetPadding(FMargin(0.f, 0.f, InventoryUIConstants::Padding_InnerSmall, 0.f));
        InventorySlot->SetHorizontalAlignment(HAlign_Fill);
        InventorySlot->SetVerticalAlignment(VAlign_Fill);
        FSlateChildSize FillSize;
        FillSize.SizeRule = ESlateSizeRule::Fill;
        FillSize.Value = 1.0f; // Takes up remaining space (equipment panel will be auto width)
        InventorySlot->SetSize(FillSize);
    }

    // Build inventory column (header, list, info)
    InventoryScreenWidgetBuilder::BuildInventoryColumn(WidgetTree, LeftColumnVBox, Border, Background, Text, Refs);
    InventoryList = Refs.InventoryList;
    if (InventoryList)
    {
        // Ensure the widget is fully constructed
        InventoryList->TakeWidget();
    }
    InfoOuterBorder = Refs.InfoOuterBorder;
    InfoInnerBorder = Refs.InfoInnerBorder;
    InfoIcon = Refs.InfoIcon;
    InfoNameText = Refs.InfoNameText;
    InfoDescText = Refs.InfoDescText;
    ClearInfoPanel();

    // Build equipment panel (right side, always last)
    InventoryScreenWidgetBuilder::BuildEquipmentPanel(WidgetTree, BodyHBox, Border, Background, Equipment, Refs);
    EquipmentPanel = Refs.EquipmentPanel;
}

void UInventoryScreenWidget::UpdateStorageWindowVisibility()
{
    // Storage window is managed separately by FirstPersonPlayerController
    // This function is kept for compatibility but doesn't need to do anything
    // The storage window should be shown/hidden by the player controller
}

void UInventoryScreenWidget::HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition)
{
    OpenContextMenu(ItemType, ScreenPosition);
}

UItemDefinition* UInventoryScreenWidget::GetSelectedItemType() const
{
    return InventoryList ? InventoryList->GetSelectedType() : nullptr;
}

void UInventoryScreenWidget::RefreshInventoryView()
{
    Refresh();
}

void UInventoryScreenWidget::Refresh()
{
    if (InventoryList)
    {
        InventoryList->Refresh();
    }
    UpdateVolumeReadout();
    if (EquipmentPanel)
    {
        EquipmentPanel->Refresh();
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
    
    // Build UI only if not already built
    if (!bUIBuilt)
    {
        RebuildUI();
        bUIBuilt = true;
    }
    
    // Update storage window visibility based on current storage state
    UpdateStorageWindowVisibility();
    
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
    
    UE_LOG(LogTemp, Display, TEXT("[InventoryScreen] ResolveComponents: Inventory=%p, Storage=%p"), Inventory ? Inventory.Get() : nullptr, Storage ? Storage.Get() : nullptr);

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

    // Bind to inventory list events
    if (InventoryList)
    {
        InventoryList->OnRowContextRequested.RemoveAll(this);
        InventoryList->OnRowLeftClicked.RemoveAll(this);
        InventoryList->OnRowHovered.RemoveAll(this);
        InventoryList->OnRowUnhovered.RemoveAll(this);
        
        InventoryList->OnRowContextRequested.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleRowContextRequested);
        InventoryList->OnRowLeftClicked.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleInventoryRowLeftClicked);
        InventoryList->OnRowHovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowHovered);
        InventoryList->OnRowUnhovered.AddUniqueDynamic(this, &UInventoryScreenWidget::HandleListRowUnhovered);
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

    // Storage window is managed separately by FirstPersonPlayerController
    // It will be shown/hidden and opened by the player controller

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
        // Iterate through all equipment slots manually
        TArray<EEquipmentSlot> AllSlots = {
            EEquipmentSlot::Head, EEquipmentSlot::Chest, EEquipmentSlot::Hands, EEquipmentSlot::Back,
            EEquipmentSlot::Primary, EEquipmentSlot::Secondary, EEquipmentSlot::Utility, EEquipmentSlot::Gadget
        };
        for (EEquipmentSlot S : AllSlots)
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
    
    // Storage window is managed separately by FirstPersonPlayerController
    // Clear storage reference when closing
    Storage = nullptr;
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
    if (!TargetText)
    {
        TargetText = VolumeText;
    }
    if (!TargetText || !Inventory)
    {
        return;
    }
    const float Used = Inventory->GetUsedVolume();
    const float Max = Inventory->MaxVolume;
    const FString Str = FString::Printf(TEXT("Volume: %.1f / %.1f"), Used, Max);
    TargetText->SetText(FText::FromString(Str));
    if (VolumeText)
    {
        VolumeText->SetColorAndOpacity(Text);
    }
    // Note: Do NOT refresh InventoryList here - that causes infinite loops
    // The list will be refreshed separately when needed
}

void UInventoryScreenWidget::UpdateInfoPanelForDef(UItemDefinition* Def)
{
    if (!Def)
    {
        ClearInfoPanel();
        return;
    }
    CurrentHoverDef = Def;
    UpdateInfoText(Def);
    UpdateInfoIcon(Def);
    if (InfoOuterBorder)
    {
        InfoOuterBorder->SetVisibility(ESlateVisibility::Visible);
    }
}

void UInventoryScreenWidget::UpdateInfoText(UItemDefinition* Def)
{
    if (!Def)
    {
        return;
    }
    if (InfoNameText)
    {
        InfoNameText->SetText(Def->DisplayName.IsEmpty() ? FText::FromString(Def->GetName()) : Def->DisplayName);
    }
    if (InfoDescText)
    {
        InfoDescText->SetText(Def->Description);
    }
}

void UInventoryScreenWidget::UpdateInfoIcon(UItemDefinition* Def)
{
    if (!Def || !InfoIcon)
    {
        return;
    }
    // Load icon asynchronously
    ItemIconHelper::LoadIconAsync(Def, ItemIconHelper::CreateIconStyle(), FOnItemIconReady::CreateLambda([this](const UItemDefinition* IconDef, UTexture2D* Texture)
    {
        if (InfoIcon && Texture && IconDef == CurrentHoverDef)
        {
            // Apply icon with default size
            ItemIconHelper::ApplyIconToImage(InfoIcon, Texture, FVector2D(64.f, 64.f));
        }
    }));
}

void UInventoryScreenWidget::HandleListRowHovered(UItemDefinition* ItemType)
{
    UpdateInfoPanelForDef(ItemType);
}

void UInventoryScreenWidget::HandleListRowUnhovered()
{
    ClearInfoPanel();
}

void UInventoryScreenWidget::OpenContextMenu(UItemDefinition* ItemType, const FVector2D& ScreenPosition)
{
    if (!ItemType)
    {
        return;
    }
    InventoryContextMenuBuilder::BuildContextMenu(this, ItemType, ScreenPosition);
}

void UInventoryScreenWidget::HandleInventoryRowLeftClicked(UItemDefinition* ItemType)
{
	if (!Storage || !Inventory || !ItemType)
	{
		return;
	}

	// Find the first item entry with this definition in inventory
	const TArray<FItemEntry>& InventoryEntries = Inventory->GetEntries();
	const FItemEntry* FoundEntry = InventoryEntries.FindByPredicate([&](const FItemEntry& E) { return E.Def == ItemType; });
	
	if (!FoundEntry || !FoundEntry->IsValid() || !FoundEntry->ItemId.IsValid())
	{
		return;
	}

	// Check if item can be stored
	if (!FoundEntry->Def->bCanBeStored)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Transfer] Item %s cannot be stored"), *GetNameSafe(FoundEntry->Def));
		return;
	}

	// Try to add to storage
	FItemEntry EntryToTransfer = *FoundEntry;
	if (Storage->CanAdd(EntryToTransfer))
	{
		// Remove from inventory first
		if (Inventory->RemoveById(FoundEntry->ItemId))
		{
			// Try to add to storage
			if (Storage->TryAdd(EntryToTransfer))
			{
				// Success - refresh both widgets
				if (InventoryList)
				{
					InventoryList->Refresh();
				}
				UpdateVolumeReadout();
				// Storage window refresh is handled by FirstPersonPlayerController
				UE_LOG(LogTemp, Display, TEXT("[Transfer] Moved %s from inventory to storage"), *GetNameSafe(EntryToTransfer.Def));
			}
			else
			{
				// Failed to add to storage, put it back in inventory
				Inventory->TryAdd(EntryToTransfer);
				UE_LOG(LogTemp, Warning, TEXT("[Transfer] Failed to add %s to Storage, restored to Inventory"), *GetNameSafe(EntryToTransfer.Def));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Transfer] Storage full, cannot add %s"), *GetNameSafe(EntryToTransfer.Def));
		
		// Push message to message log
		if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
		{
			MsgLog->PushMessage(FText::FromString(TEXT("Not enough space in container")));
		}
	}
}

void UInventoryScreenWidget::HandleStorageItemLeftClick(const FGuid& ItemId)
{
	if (!Storage || !Inventory || !ItemId.IsValid())
	{
		return;
	}

	// Find the entry in storage
	const TArray<FItemEntry>& StorageEntries = Storage->GetEntries();
	const FItemEntry* FoundEntry = StorageEntries.FindByPredicate([&](const FItemEntry& E) { return E.ItemId == ItemId; });
	
	if (!FoundEntry || !FoundEntry->IsValid())
	{
		return;
	}

	// Check if item can be stored
	if (!FoundEntry->Def->bCanBeStored)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Transfer] Item %s cannot be stored"), *GetNameSafe(FoundEntry->Def));
		return;
	}

	// Try to add to inventory
	FItemEntry EntryToTransfer = *FoundEntry;
	if (Inventory->CanAdd(EntryToTransfer))
	{
		// Remove from storage first
		if (Storage->RemoveById(ItemId))
		{
			// Try to add to inventory
			if (Inventory->TryAdd(EntryToTransfer))
			{
			// Success - refresh both widgets
			if (InventoryList)
			{
				InventoryList->Refresh();
			}
			UpdateVolumeReadout();
			// Storage window refresh is handled by FirstPersonPlayerController via OnStorageChanged delegate
			UE_LOG(LogTemp, Display, TEXT("[Transfer] Moved %s from storage to inventory"), *GetNameSafe(EntryToTransfer.Def));
			}
			else
			{
				// Failed to add to inventory, put it back in storage
				Storage->TryAdd(EntryToTransfer);
				UE_LOG(LogTemp, Warning, TEXT("[Transfer] Failed to add %s to Inventory, restored to Storage"), *GetNameSafe(EntryToTransfer.Def));
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Transfer] Inventory full, cannot add %s"), *GetNameSafe(EntryToTransfer.Def));
	}
}

bool UInventoryScreenWidget::HandleCloseKey(const FKey& Key)
{
    if (Key == EKeys::Tab || Key == EKeys::I || Key == EKeys::Escape)
    {
        OnRequestClose.Broadcast();
        return true;
    }
    return false;
}

void UInventoryScreenWidget::SetTerminalStyle(const FLinearColor& InBorder, const FLinearColor& InBackground, const FLinearColor& InText)
{
    Border = InBorder;
    Background = InBackground;
    Text = InText;
    // If UI is already built, we'd need to rebuild to apply new style, but for now just update the colors
    // The style will be applied on next RebuildUI() call
}

FReply UInventoryScreenWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
    if (HandleCloseKey(InKeyEvent.GetKey()))
    {
        return FReply::Handled();
    }
    return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}
