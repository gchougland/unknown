#include "UI/HotbarWidget.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Blueprint/WidgetTree.h"
#include "Engine/Texture2D.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Inventory/InventoryComponent.h"
#include "Widgets/SBoxPanel.h" // FSlateChildSize / ESlateSizeRule
#include "Inventory/ItemTypes.h"
#include "UI/ItemIconHelper.h"
#include "UI/InventoryUIConstants.h"
#include "UI/ComponentResolver.h"
#include "UI/HotbarSlotBuilder.h"
#include "UI/ProjectStyle.h"
#include "UI/TerminalWidgetHelpers.h"

TSharedRef<SWidget> UHotbarWidget::RebuildWidget()
{
    // Build the widget tree before Slate is generated; otherwise UUserWidget returns an SSpacer
    if (WidgetTree)
    {
        RebuildSlots();
    }
    return Super::RebuildWidget();
}

void UHotbarWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] NativeConstruct (this=%s)"), *GetName());
    // Do not rebuild here — rebuilding after Slate creation can desync on-screen widgets vs. arrays
    RefreshAll();

    // Bind to inventory changes so quantities update live
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
            {
                // Avoid double-binding across reconstructs
                if (BoundInventory != Inv)
                {
                    if (BoundInventory)
                    {
                        BoundInventory->OnItemAdded.RemoveAll(this);
                        BoundInventory->OnItemRemoved.RemoveAll(this);
                    }
                    BoundInventory = Inv;
                    BoundInventory->OnItemAdded.AddUniqueDynamic(this, &UHotbarWidget::OnInventoryItemAdded);
                    BoundInventory->OnItemRemoved.AddUniqueDynamic(this, &UHotbarWidget::OnInventoryItemRemoved);
                }
            }
        }
    }
}

void UHotbarWidget::NativeDestruct()
{
    // Unbind inventory events to avoid dangling delegates
    if (BoundInventory)
    {
        BoundInventory->OnItemAdded.RemoveAll(this);
        BoundInventory->OnItemRemoved.RemoveAll(this);
        BoundInventory = nullptr;
    }
    // Unbind hotbar events as well
    if (Hotbar)
    {
        Hotbar->OnSlotAssigned.RemoveAll(this);
        Hotbar->OnActiveChanged.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UHotbarWidget::RebuildSlots()
{
    if (!WidgetTree)
    {
        return;
    }
    UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] RebuildSlots start (AssignedTypes=%d)"), AssignedTypes.Num());
    // Clear previous tree content by recreating the root
    RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
    WidgetTree->RootWidget = RootBox;
    UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] RootWidget set to RootBox: %s"), *GetNameSafe(RootBox));

	SlotBorders.Empty();
 SlotIcons.Empty();
 SlotLabels.Empty();
 SlotHotkeys.Empty();

	const int32 Num = AssignedTypes.Num() > 0 ? AssignedTypes.Num() : 9;
    for (int32 i = 0; i < Num; ++i)
    {
        HotbarSlotBuilder::FSlotWidgets SlotWidgets;
        HotbarSlotBuilder::BuildSlotWidget(
            WidgetTree,
            RootBox,
            i,
            BorderColor,
            SlotBackground,
            TextColor,
            SlotSize,
            SlotWidgets
        );

        if (SlotWidgets.InnerBorder && SlotWidgets.Icon && SlotWidgets.Quantity && SlotWidgets.Hotkey)
        {
            SlotBorders.Add(SlotWidgets.InnerBorder);
            SlotIcons.Add(SlotWidgets.Icon);
            SlotLabels.Add(SlotWidgets.Quantity);
            SlotHotkeys.Add(SlotWidgets.Hotkey);
        }
    }
    UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] RebuildSlots end (Num=%d)"), Num);
}

void UHotbarWidget::RefreshAll()
{
	const int32 Num = SlotBorders.Num();
	for (int32 i = 0; i < Num; ++i)
	{
		RefreshSlotVisual(i);
	}
}

void UHotbarWidget::RefreshSlotVisual(int32 Index)
{
    if (!SlotBorders.IsValidIndex(Index) || !SlotIcons.IsValidIndex(Index) || !SlotLabels.IsValidIndex(Index))
    {
        return;
    }
    UpdateSlotBackground(Index);
    UpdateSlotIcon(Index);
    UpdateSlotQuantity(Index);
}

void UHotbarWidget::UpdateSlotBackground(int32 Index)
{
    if (!SlotBorders.IsValidIndex(Index))
    {
        return;
    }
    UBorder* Border = SlotBorders[Index];
    // Background highlight for active slot
    Border->SetBrushColor(Index == ActiveIndex ? ActiveBackground : SlotBackground);
}

void UHotbarWidget::UpdateSlotIcon(int32 Index)
{
    if (!SlotIcons.IsValidIndex(Index))
    {
        return;
    }
    UImage* Icon = SlotIcons[Index];

    // Icon update & opacity rules
    UTexture2D* Tex = nullptr;
    if (AssignedTypes.IsValidIndex(Index))
    {
        if (UItemDefinition* Def = AssignedTypes[Index])
        {
            const FItemIconStyle Style = ItemIconHelper::CreateIconStyle();
            Tex = ItemIconHelper::LoadIconSync(Def, Style);
            
            if (!Tex)
            {
                // Request async load
                TWeakObjectPtr<UHotbarWidget> WeakThis(this);
                TWeakObjectPtr<UImage> WeakIcon(Icon);
                FOnItemIconReady Callback = FOnItemIconReady::CreateLambda([WeakThis, WeakIcon, Index](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
                {
                    if (!WeakThis.IsValid() || !WeakIcon.IsValid() || ReadyTex == nullptr)
                    {
                        return;
                    }
                    if (!WeakThis->AssignedTypes.IsValidIndex(Index) || WeakThis->AssignedTypes[Index] != ReadyDef)
                    {
                        return; // Slot changed since request
                    }
                    ItemIconHelper::ApplyIconToImage(WeakIcon.Get(), ReadyTex, WeakThis->SlotSize);
                    WeakIcon->SetBrushTintColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
                });
                ItemIconHelper::LoadIconAsync(Def, Style, MoveTemp(Callback));
            }
        }
    }
    
    if (Tex)
    {
        ItemIconHelper::ApplyIconToImage(Icon, Tex, SlotSize);
        // Ensure brush tint alpha is 1 for non-empty
        Icon->SetBrushTintColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
    }
    else
    {
        // Transparent when empty slot
        Icon->SetBrush(FSlateBrush());
        // Per spec: set brush tint alpha to 0 when empty
        Icon->SetBrushTintColor(FLinearColor(1.f, 1.f, 1.f, 0.f));
        Icon->SetOpacity(0.f);
        // Collapsed prevents any accidental draw/hit-test
        Icon->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UHotbarWidget::UpdateSlotQuantity(int32 Index)
{
    if (!SlotLabels.IsValidIndex(Index) || !SlotIcons.IsValidIndex(Index))
    {
        return;
    }
    UTextBlock* Label = SlotLabels[Index];
    UImage* Icon = SlotIcons[Index];

    // Quantity overlay: count items of this type from owning pawn's inventory
    int32 Quantity = 0;
    if (AssignedTypes.IsValidIndex(Index))
    {
        if (UItemDefinition* Def = AssignedTypes[Index])
        {
            if (UInventoryComponent* Inv = ComponentResolver::ResolveInventoryComponent(GetOwningPlayer()))
            {
                Quantity = Inv->CountByDef(Def);
            }
        }
    }
    
    // Quantity only shown when > 1; when == 0 and type is assigned, gray out icon to half transparency
    if (AssignedTypes.IsValidIndex(Index) && AssignedTypes[Index] != nullptr)
    {
        if (Quantity <= 0)
        {
            // Assigned but none in inventory: gray icon
            Icon->SetOpacity(0.5f);
            Icon->SetVisibility(ESlateVisibility::Visible);
        }
        // Show quantity only if more than 1 and ensure visibility when shown
        if (Quantity > 1)
        {
            Label->SetText(FText::AsNumber(Quantity));
            // Ensure it renders — explicitly set to Visible
            Label->SetVisibility(ESlateVisibility::Visible);
            Label->SetIsEnabled(true);
            Label->SetRenderOpacity(1.f);
            // Ensure Slate allocates horizontal space (Widget Reflector showed width=0)
            Label->SetMinDesiredWidth(InventoryUIConstants::FontSize_InfoDesc);
            // Ensure readable, fully opaque color (force A=1)
            FLinearColor OpaqueText = TextColor; OpaqueText.A = 1.f;
            Label->SetColorAndOpacity(OpaqueText);
            // Extra defensive: clear any render transform that could push it out of view
            Label->SetRenderTransform(FWidgetTransform());
            // Apply immediately
            Label->InvalidateLayoutAndVolatility();
            InvalidateLayoutAndVolatility();
            // Overlay draws children in the order added; Quantity is added after Hotkey, so it renders above it.
            UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] Show Qty slot %d => %d (Vis=%d, Opacity=%.2f, Label=%s, Parent=%s)"), Index, Quantity, (int32)Label->GetVisibility(), Label->GetRenderOpacity(), *GetNameSafe(Label), *GetNameSafe(Label->GetParent()));
        }
        else
        {
            Label->SetText(FText::GetEmpty());
            // Hide when not used
            Label->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] Hide Qty slot %d (Q=%d)"), Index, Quantity);
        }
    }
    else
    {
        // Empty slot: ensure quantity hidden
        Label->SetText(FText::GetEmpty());
        Label->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] No type for slot %d -> hide qty"), Index);
    }
}

void UHotbarWidget::SetHotbar(UHotbarComponent* InHotbar)
{
	if (Hotbar == InHotbar)
	{
		return;
	}

	// Unbind previous
	if (Hotbar)
	{
		Hotbar->OnSlotAssigned.RemoveAll(this);
		Hotbar->OnActiveChanged.RemoveAll(this);
	}

	Hotbar = InHotbar;
	AssignedTypes.Reset();
	ActiveIndex = INDEX_NONE;
	ActiveItemId.Invalidate();

	if (Hotbar)
	{
		AssignedTypes.SetNum(Hotbar->GetNumSlots());
		// Initialize with current assigned types
		for (int32 i = 0; i < Hotbar->GetNumSlots(); ++i)
		{
			AssignedTypes[i] = Hotbar->GetSlot(i).AssignedType;
		}

		// Mirror current active
		ActiveIndex = Hotbar->GetActiveIndex();
		ActiveItemId = Hotbar->GetActiveItemId();

		// Bind events
		Hotbar->OnSlotAssigned.AddDynamic(this, &UHotbarWidget::OnSlotAssigned);
		Hotbar->OnActiveChanged.AddDynamic(this, &UHotbarWidget::OnActiveChanged);
	}

 // Do NOT rebuild the tree here — it desyncs the on‑screen Slate widget from our arrays.
 // Just refresh visuals against the existing, on‑screen tree.
 RefreshAll();
}

void UHotbarWidget::OnInventoryItemAdded(const FItemEntry& /*Item*/)
{
    RefreshAll();
}

void UHotbarWidget::OnInventoryItemRemoved(const FGuid& /*ItemId*/)
{
    RefreshAll();
}

UItemDefinition* UHotbarWidget::GetAssignedType(int32 Index) const
{
    return AssignedTypes.IsValidIndex(Index) ? AssignedTypes[Index] : nullptr;
}

void UHotbarWidget::OnSlotAssigned(int32 Index, UItemDefinition* ItemType)
{
	if (!AssignedTypes.IsValidIndex(Index))
	{
		AssignedTypes.SetNum(FMath::Max(Index + 1, AssignedTypes.Num()));
	}
	AssignedTypes[Index] = ItemType;
	// Update visuals for this slot
	RefreshSlotVisual(Index);
}

void UHotbarWidget::OnActiveChanged(int32 NewIndex, FGuid ItemId)
{
	// Refresh previous and new active highlights by refreshing all (cheap: 9 slots)
	ActiveIndex = NewIndex;
	ActiveItemId = ItemId;
	RefreshAll();
}
