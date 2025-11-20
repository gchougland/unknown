#include "UI/EquipmentPanelWidget.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemTypes.h"
#include "UI/ItemIconHelper.h"
#include "Components/VerticalBox.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Blueprint/WidgetTree.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Input/SButton.h"
#include "UI/InventoryScreenWidget.h"
#include "UI/ProjectStyle.h"
#include "Inventory/InventoryComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

// Opaque cache for Slate widgets and brushes to keep UHT-clean header
struct FEquipmentPanelSlateCache
{
    TSharedPtr<SVerticalBox> RootVBox;
    TMap<EEquipmentSlot, TSharedPtr<SImage>> SlotIconImages;
    TMap<EEquipmentSlot, TSharedPtr<STextBlock>> SlotNameTexts;
    // Per-slot brushes to hold UTexture references
    TMap<EEquipmentSlot, FSlateBrush> StaticSlotBrushes;
};

namespace
{
static void UpdateInventoryScreenVolumeText(UEquipmentPanelWidget* Panel)
{
    if (!Panel)
    {
        return;
    }
    UInventoryScreenWidget* Screen = Panel->GetTypedOuter<UInventoryScreenWidget>();
    if (!Screen)
    {
        return;
    }
    // Find the text block by name on the screen's widget tree and set text
    if (UWidgetTree* Tree = Screen->WidgetTree)
    {
        if (UTextBlock* VolumeText = Cast<UTextBlock>(Tree->FindWidget(TEXT("VolumeText"))))
        {
            // Resolve inventory
            UInventoryComponent* Inv = nullptr;
            if (APlayerController* PC = Screen->GetOwningPlayer())
            {
                if (APawn* Pawn = PC->GetPawn())
                {
                    Inv = Pawn->FindComponentByClass<UInventoryComponent>();
                }
            }
            float Used = 0.f, Max = 0.f;
            if (Inv)
            {
                Used = FMath::Max(0.f, Inv->GetUsedVolume());
                Max = FMath::Max(0.f, Inv->MaxVolume);
            }
            const FString Str = FString::Printf(TEXT("Volume: %.1f / %.1f"), Used, Max);
            VolumeText->SetText(FText::FromString(Str));
        }
    }
}
}

UEquipmentPanelWidget::~UEquipmentPanelWidget()
{
    // Ensure complete type is known here; safely delete
    if (Cache)
    {
        delete Cache;
        Cache = nullptr;
    }
}

void UEquipmentPanelWidget::SetEquipmentComponent(UEquipmentComponent* InEquipment)
{
    if (Equipment == InEquipment)
    {
        return;
    }
    if (Equipment)
    {
        Equipment->OnItemEquipped.RemoveAll(this);
        Equipment->OnItemUnequipped.RemoveAll(this);
    }
    Equipment = InEquipment;
    if (Equipment)
    {
        Equipment->OnItemEquipped.AddDynamic(this, &UEquipmentPanelWidget::OnItemEquipped);
        Equipment->OnItemUnequipped.AddDynamic(this, &UEquipmentPanelWidget::OnItemUnequipped);
    }
    Refresh();
}

void UEquipmentPanelWidget::Refresh()
{
    // Always ensure widget exists, then update all slots
    TakeWidget();
    UpdateAllSlots();
}

TSharedRef<SWidget> UEquipmentPanelWidget::RebuildWidget()
{
    // Ensure cache exists
    if (!Cache)
    {
        Cache = new FEquipmentPanelSlateCache();
    }
    // Build persistent root and rows; cache per-slot widgets for live updates
    Cache->RootVBox = SNew(SVerticalBox);

    auto AddRow = [&](EEquipmentSlot EquipSlot)
    {
        // Prepare cached widgets for this slot
        TSharedPtr<SImage> IconImage;
        TSharedPtr<STextBlock> ItemNameText;

        TSharedRef<SWidget> Row =
        SNew(SBorder)
        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
        .BorderBackgroundColor(FLinearColor::White)
        .Padding(FMargin(1.f))
        [
            SNew(SBorder)
            .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
            .BorderBackgroundColor(FLinearColor::Black)
            .Padding(FMargin(6.f))
            .OnMouseButtonDown_Lambda([this, EquipSlot](const FGeometry& Geo, const FPointerEvent& Evt)
            {
                if (Evt.GetEffectingButton() == EKeys::RightMouseButton)
                {
                    if (Equipment)
                    {
                        FText Error;
                        const bool bOk = Equipment->TryUnequipToInventory(EquipSlot, Error);
                        // Ask owning inventory screen (if any) to refresh lists
                        if (UInventoryScreenWidget* Screen = GetTypedOuter<UInventoryScreenWidget>())
                        {
                            Screen->RefreshInventoryView();
                            // Our own panel will receive events, but force a refresh as a fallback
                            Refresh();
                        }
                        (void)bOk; // suppress unused in shipping if logging removed
                    }
                    return FReply::Handled();
                }
                return FReply::Unhandled();
            })
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Top)
                .Padding(FMargin(0.f, 0.f, 8.f, 0.f))
                [
                    // Icon square: outer white border, inner black center; fixed to 36x36 even when empty
                    SNew(SBorder)
                    .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                    .BorderBackgroundColor(FLinearColor::White)
                    .Padding(FMargin(1.f))
                    [
                        SNew(SBorder)
                        .BorderImage(FCoreStyle::Get().GetBrush("GenericWhiteBox"))
                        .BorderBackgroundColor(FLinearColor::Black)
                        .Padding(FMargin(0.f))
                        [
                            SNew(SBox)
                            .WidthOverride(36.f)
                            .HeightOverride(36.f)
                            [
                                SAssignNew(IconImage, SImage)
                                .ColorAndOpacity(FLinearColor::White)
                                .Image(nullptr) // start empty; will be set when icon is available
                            ]
                        ]
                    ]
                ]
                + SHorizontalBox::Slot()
                .AutoWidth()
                .VAlign(VAlign_Fill)
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SNew(STextBlock)
                        .Text(FText::FromString(GetSlotLabel(EquipSlot)))
                        .ColorAndOpacity(FLinearColor::White)
                        .Font(ProjectStyle::GetMonoFont(12))
                    ]
                    + SVerticalBox::Slot()
                    .AutoHeight()
                    [
                        SAssignNew(ItemNameText, STextBlock)
                        .Text(FText::FromString(TEXT("None")))
                        .ColorAndOpacity(FLinearColor::White)
                        .Font(ProjectStyle::GetMonoFont(12))
                    ]
                ]
            ]
        ];

        // Cache pointers
        Cache->SlotIconImages.Add(EquipSlot, IconImage);
        Cache->SlotNameTexts.Add(EquipSlot, ItemNameText);

        Cache->RootVBox->AddSlot()
        .AutoHeight()
        .Padding(FMargin(0.f, 2.f))
        [ Row ];
    };

    AddRow(EEquipmentSlot::Head);
    AddRow(EEquipmentSlot::Chest);
    AddRow(EEquipmentSlot::Back);
    AddRow(EEquipmentSlot::Hands);
    AddRow(EEquipmentSlot::Primary);
    AddRow(EEquipmentSlot::Secondary);
    AddRow(EEquipmentSlot::Utility);
    AddRow(EEquipmentSlot::Gadget);

    // Populate initial state
    UpdateAllSlots();

    return Cache->RootVBox.ToSharedRef();
}

void UEquipmentPanelWidget::NativeConstruct()
{
    Super::NativeConstruct();
    // Ensure we receive events if Equipment is assigned post-construct
    if (Equipment)
    {
        Equipment->OnItemEquipped.AddDynamic(this, &UEquipmentPanelWidget::OnItemEquipped);
        Equipment->OnItemUnequipped.AddDynamic(this, &UEquipmentPanelWidget::OnItemUnequipped);
    }
}

void UEquipmentPanelWidget::NativeDestruct()
{
    if (Equipment)
    {
        Equipment->OnItemEquipped.RemoveAll(this);
        Equipment->OnItemUnequipped.RemoveAll(this);
    }
    Super::NativeDestruct();
}

void UEquipmentPanelWidget::OnItemEquipped(EEquipmentSlot /*InSlot*/, const FItemEntry& /*Item*/)
{
    UpdateAllSlots();
    // Also refresh inventory list and volume readout on the owning screen
    if (UInventoryScreenWidget* Screen = GetTypedOuter<UInventoryScreenWidget>())
    {
        Screen->RefreshInventoryView();
    }
    UpdateInventoryScreenVolumeText(this);
}

void UEquipmentPanelWidget::OnItemUnequipped(EEquipmentSlot /*InSlot*/, const FItemEntry& /*Item*/)
{
    UpdateAllSlots();
    if (UInventoryScreenWidget* Screen = GetTypedOuter<UInventoryScreenWidget>())
    {
        Screen->RefreshInventoryView();
    }
    UpdateInventoryScreenVolumeText(this);
}

FString UEquipmentPanelWidget::GetSlotLabel(EEquipmentSlot InSlot) const
{
    switch (InSlot)
    {
    case EEquipmentSlot::Head: return TEXT("Head");
    case EEquipmentSlot::Chest: return TEXT("Chest");
    case EEquipmentSlot::Hands: return TEXT("Hands");
    case EEquipmentSlot::Back: return TEXT("Back");
    case EEquipmentSlot::Primary: return TEXT("Primary");
    case EEquipmentSlot::Secondary: return TEXT("Secondary");
    case EEquipmentSlot::Utility: return TEXT("Utility");
    case EEquipmentSlot::Gadget: return TEXT("Gadget");
    default: break;
    }
    return TEXT("Slot");
}

void UEquipmentPanelWidget::UpdateAllSlots()
{
    UpdateSlot(EEquipmentSlot::Head);
    UpdateSlot(EEquipmentSlot::Chest);
    UpdateSlot(EEquipmentSlot::Back);
    UpdateSlot(EEquipmentSlot::Hands);
    UpdateSlot(EEquipmentSlot::Primary);
    UpdateSlot(EEquipmentSlot::Secondary);
    UpdateSlot(EEquipmentSlot::Utility);
    UpdateSlot(EEquipmentSlot::Gadget);
}

void UEquipmentPanelWidget::UpdateSlot(EEquipmentSlot InSlot)
{
    if (!Cache)
    {
        return;
    }
    TSharedPtr<STextBlock> NameText = Cache->SlotNameTexts.FindRef(InSlot);
    TSharedPtr<SImage> IconImg = Cache->SlotIconImages.FindRef(InSlot);
    if (!NameText.IsValid() || !IconImg.IsValid())
    {
        return;
    }

    FText ItemText = FText::FromString(TEXT("None"));
    UTexture2D* IconTex = nullptr;

    if (Equipment)
    {
        FItemEntry Entry;
        if (Equipment->GetEquipped(InSlot, Entry) && Entry.Def)
        {
            ItemText = Entry.Def->DisplayName.IsEmpty() ? FText::FromString(Entry.Def->GetName()) : Entry.Def->DisplayName;

            // Use transparent background for equipment slots
            EItemIconBackground TransparentBg = EItemIconBackground::Transparent;
            const FItemIconStyle Style = ItemIconHelper::CreateIconStyle(0, &TransparentBg);
            
            IconTex = ItemIconHelper::LoadIconSync(Entry.Def, Style);
            
            if (!IconTex)
            {
                // Request async load
                TWeakPtr<SImage> WeakImg = IconImg;
                TWeakObjectPtr<UEquipmentPanelWidget> WeakThis(this);
                const EEquipmentSlot SlotCopy = InSlot;
                UItemDefinition* Requested = Entry.Def;
                const FVector2D IconSize(36.f, 36.f);
                FOnItemIconReady Callback = FOnItemIconReady::CreateLambda([WeakImg, WeakThis, Requested, SlotCopy, IconSize](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
                {
                    if (!ReadyTex || ReadyDef != Requested)
                    {
                        return;
                    }
                    if (!WeakThis.IsValid())
                    {
                        return;
                    }
                    if (TSharedPtr<SImage> Img = WeakImg.Pin())
                    {
                        if (WeakThis->Cache)
                        {
                            WeakThis->Cache->StaticSlotBrushes.FindOrAdd(SlotCopy) = FSlateBrush();
                            FSlateBrush& B2 = WeakThis->Cache->StaticSlotBrushes[SlotCopy];
                            B2.DrawAs = ESlateBrushDrawType::Image;
                            B2.ImageSize = IconSize;
                            B2.SetResourceObject(ReadyTex);
                            Img->SetImage(&B2);
                        }
                    }
                });
                ItemIconHelper::LoadIconAsync(Entry.Def, Style, MoveTemp(Callback));
            }
        }
    }

    NameText->SetText(ItemText);

    if (IconTex)
    {
        // Set image brush
        Cache->StaticSlotBrushes.FindOrAdd(InSlot) = FSlateBrush();
        FSlateBrush& B = Cache->StaticSlotBrushes[InSlot];
        B.DrawAs = ESlateBrushDrawType::Image;
        B.ImageSize = FVector2D(36.f, 36.f);
        B.SetResourceObject(IconTex);
        IconImg->SetImage(&B);
    }
    else
    {
        // Clear image so black center shows
        IconImg->SetImage(nullptr);
    }
}
