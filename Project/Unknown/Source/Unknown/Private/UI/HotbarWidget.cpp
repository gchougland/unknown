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
#include "Icons/ItemIconSubsystem.h"
#include "Icons/ItemIconSettings.h"
#include "Icons/ItemIconTypes.h"
#include "UI/ProjectStyle.h"

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
        // Outer white border for terminal-style outline
        UBorder* Outer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("SlotOuter_%d"), i));
        {
            FSlateBrush Brush;
            Brush.DrawAs = ESlateBrushDrawType::Box;
            Brush.TintColor = BorderColor;
            Brush.Margin = FMargin(0.f);
            Outer->SetBrush(Brush);
        }
        // Match inventory outline thickness: keep a small inner padding so the border is visible
        Outer->SetPadding(FMargin(2.f));

        UVerticalBoxSlot* VBSlot = RootBox->AddChildToVerticalBox(Outer);
        if (VBSlot)
        {
            // No external spacing between slots and do not stretch to full width — keep square
            VBSlot->SetPadding(FMargin(0.f));
            VBSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
            VBSlot->SetHorizontalAlignment(HAlign_Left);
        }

        // Inner black panel that we will highlight for active slot
        UBorder* Border = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("SlotBorder_%d"), i));
        {
            FSlateBrush InnerBrush;
            InnerBrush.DrawAs = ESlateBrushDrawType::Box;
            InnerBrush.TintColor = SlotBackground;
            InnerBrush.Margin = FMargin(0.f);
            Border->SetBrush(InnerBrush);
        }
        Border->SetPadding(FMargin(0.f));
        Outer->SetContent(Border);

        // Size box to enforce slot geometry even without textures
        USizeBox* SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("SizeBox_%d"), i));
        SizeBox->SetWidthOverride(SlotSize.X);
        SizeBox->SetHeightOverride(SlotSize.Y);
        Border->SetContent(SizeBox);

        // Inner overlay: icon with quantity text overlaid
        UOverlay* Inner = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Inner_%d"), i));
        SizeBox->SetContent(Inner);

        UImage* Icon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), *FString::Printf(TEXT("Icon_%d"), i));
        // Ensure the icon fills the square slot
        {
            FSlateBrush IconBrush;
            IconBrush.DrawAs = ESlateBrushDrawType::Image;
            IconBrush.Tiling = ESlateBrushTileType::NoTile;
            IconBrush.ImageSize = SlotSize;
            Icon->SetBrush(IconBrush);
        }
        UOverlaySlot* IconSlot = Inner->AddChildToOverlay(Icon);
        if (IconSlot)
        {
            IconSlot->SetHorizontalAlignment(HAlign_Fill);
            IconSlot->SetVerticalAlignment(VAlign_Fill);
            IconSlot->SetPadding(FMargin(0.f));
        }

        // Hotkey label (always visible) at top-left
        UTextBlock* Hotkey = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Hotkey_%d"), i));
        Hotkey->SetText(FText::FromString(FString::FromInt((i + 1) % 10 == 0 ? 0 : (i + 1))));
        Hotkey->SetColorAndOpacity(TextColor);
        {
            Hotkey->SetFont(ProjectStyle::GetMonoFont(14));
            Hotkey->SetShadowOffset(FVector2D(1.f, 1.f));
            Hotkey->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
        }
        UOverlaySlot* HotkeySlot = Inner->AddChildToOverlay(Hotkey);
        if (HotkeySlot)
        {
            HotkeySlot->SetHorizontalAlignment(HAlign_Left);
            HotkeySlot->SetVerticalAlignment(VAlign_Top);
            HotkeySlot->SetPadding(FMargin(2.f, 2.f, 0.f, 0.f));
            // Overlay draws children in the order they are added; no ZOrder API here.
        }

        // Quantity text; ensure it renders above the hotkey so it can't be occluded
        UTextBlock* Quantity = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Quantity_%d"), i));
        Quantity->SetText(FText::FromString("0"));
        Quantity->SetColorAndOpacity(TextColor);
        {
            // Final spec: modest size near the hotkey
            Quantity->SetFont(ProjectStyle::GetMonoFont(16));
            Quantity->SetShadowOffset(FVector2D(1.f, 1.f));
            Quantity->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
            Quantity->SetAutoWrapText(false);
            // Guard against zero-width measurement by ensuring a minimal desired width
            Quantity->SetMinDesiredWidth(12.f);
            // Start hidden until we compute a quantity > 1
            Quantity->SetVisibility(ESlateVisibility::Collapsed);
        }
        UOverlaySlot* QuantitySlot = Inner->AddChildToOverlay(Quantity);
        if (QuantitySlot)
        {
            // Right-aligned as requested
            QuantitySlot->SetHorizontalAlignment(HAlign_Right);
            QuantitySlot->SetVerticalAlignment(VAlign_Top);
            QuantitySlot->SetPadding(FMargin(0.f, 2.f, 2.f, 0.f));
        }

        SlotBorders.Add(Border);
        SlotIcons.Add(Icon);
        SlotLabels.Add(Quantity);
        SlotHotkeys.Add(Hotkey);
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
    UBorder* Border = SlotBorders[Index];
    UImage* Icon = SlotIcons[Index];
    UTextBlock* Label = SlotLabels[Index];

	// Background highlight for active slot
	Border->SetBrushColor(Index == ActiveIndex ? ActiveBackground : SlotBackground);

    // Icon update & opacity rules
    UTexture2D* Tex = nullptr;
    if (AssignedTypes.IsValidIndex(Index))
    {
        if (UItemDefinition* Def = AssignedTypes[Index])
        {
            Tex = Def->IconOverride;
            if (!Tex)
            {
                // Query runtime icon subsystem for generated icon
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

                    if (UTexture2D* Cached = IconSys->GetIconIfReady(Def, Style))
                    {
                        Tex = Cached;
                    }
                    else
                    {
                        // Defer: request generation and update this slot when ready
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
                            FSlateBrush Brush;
                            Brush.DrawAs = ESlateBrushDrawType::Image;
                            Brush.SetResourceObject(ReadyTex);
                            Brush.ImageSize = WeakThis->SlotSize;
                            WeakIcon->SetBrush(Brush);
                            WeakIcon->SetBrushTintColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
                            WeakIcon->SetOpacity(1.f);
                            WeakIcon->SetVisibility(ESlateVisibility::Visible);
                        });
                        IconSys->RequestIcon(Def, Style, MoveTemp(Callback));
                    }
                }
            }
        }
    }
    if (Tex)
    {
        FSlateBrush Brush;
        Brush.DrawAs = ESlateBrushDrawType::Image;
        Brush.SetResourceObject(Tex);
        // Keep icon square and filling the slot
        Brush.ImageSize = SlotSize;
        Icon->SetBrush(Brush);
        // Ensure brush tint alpha is 1 for non-empty
        Icon->SetBrushTintColor(FLinearColor(1.f, 1.f, 1.f, 1.f));
        Icon->SetOpacity(1.f);
        Icon->SetVisibility(ESlateVisibility::Visible);
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

    // Quantity overlay: count items of this type from owning pawn's inventory
    int32 Quantity = 0;
    if (AssignedTypes.IsValidIndex(Index))
    {
        if (UItemDefinition* Def = AssignedTypes[Index])
        {
            if (APlayerController* PC = GetOwningPlayer())
            {
                if (APawn* Pawn = PC->GetPawn())
                {
                    if (UInventoryComponent* Inv = Pawn->FindComponentByClass<UInventoryComponent>())
                    {
                        Quantity = Inv->CountByDef(Def);
                    }
                }
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
            UTextBlock* QtyLabel = SlotLabels[Index];
            QtyLabel->SetText(FText::AsNumber(Quantity));
            // Ensure it renders — explicitly set to Visible
            QtyLabel->SetVisibility(ESlateVisibility::Visible);
            QtyLabel->SetIsEnabled(true);
            QtyLabel->SetRenderOpacity(1.f);
            // Ensure Slate allocates horizontal space (Widget Reflector showed width=0)
            QtyLabel->SetMinDesiredWidth(12.f);
            // Ensure readable, fully opaque color (force A=1)
            FLinearColor OpaqueText = TextColor; OpaqueText.A = 1.f;
            QtyLabel->SetColorAndOpacity(OpaqueText);
            // Extra defensive: clear any render transform that could push it out of view
            QtyLabel->SetRenderTransform(FWidgetTransform());
            // Apply immediately
            QtyLabel->InvalidateLayoutAndVolatility();
            InvalidateLayoutAndVolatility();
            // Overlay draws children in the order added; Quantity is added after Hotkey, so it renders above it.
            UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] Show Qty slot %d => %d (Vis=%d, Opacity=%.2f, Label=%s, Parent=%s)"), Index, Quantity, (int32)QtyLabel->GetVisibility(), QtyLabel->GetRenderOpacity(), *GetNameSafe(QtyLabel), *GetNameSafe(QtyLabel->GetParent()));
        }
        else
        {
            SlotLabels[Index]->SetText(FText::GetEmpty());
            // Hide when not used
            SlotLabels[Index]->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] Hide Qty slot %d (Q=%d)"), Index, Quantity);
        }
    }
    else
    {
        // Empty slot: ensure quantity hidden
        SlotLabels[Index]->SetText(FText::GetEmpty());
        SlotLabels[Index]->SetVisibility(ESlateVisibility::Hidden);
        UE_LOG(LogTemp, Display, TEXT("[HotbarWidget] No type for slot %d -> hide qty"), Index);
    }
    // Note: final color is applied when visible above
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
