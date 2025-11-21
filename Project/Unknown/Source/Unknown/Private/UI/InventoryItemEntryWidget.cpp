#include "UI/InventoryItemEntryWidget.h"
#include "Inventory/ItemDefinition.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "Input/Events.h"
#include "Framework/Application/SlateApplication.h"
// Icon subsystem integration
#include "UI/ItemIconHelper.h"
#include "UI/InventoryUIConstants.h"
#include "UI/ProjectStyle.h"

void UInventoryItemEntryWidget::SetData(UItemDefinition* InDef, int32 InCount, float InTotalVolume)
{
	Def = InDef;
	Count = InCount;
	TotalVolume = InTotalVolume;
	Refresh();
}

TSharedRef<SWidget> UInventoryItemEntryWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		RebuildUI();
	}
	return Super::RebuildWidget();
}

void UInventoryItemEntryWidget::NativeConstruct()
{
    Super::NativeConstruct();
    Refresh();
}

FReply UInventoryItemEntryWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		const FVector2D ScreenPos = InMouseEvent.GetScreenSpacePosition();
		OnContextRequested.Broadcast(Def, ScreenPos);
		return FReply::Handled();
	}
	else if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnLeftClicked.Broadcast(Def);
		return FReply::Handled();
	}
    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

void UInventoryItemEntryWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
    OnHovered.Broadcast(Def);
}

void UInventoryItemEntryWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
    Super::NativeOnMouseLeave(InMouseEvent);
    OnUnhovered.Broadcast();
}

static void ApplyBorderBrush(UBorder* Border, const FLinearColor& Color)
{
    if (!Border) return;
    FSlateBrush Brush;
    Brush.DrawAs = ESlateBrushDrawType::Box;
    Brush.TintColor = Color;
    Brush.Margin = FMargin(0.f);
    Border->SetBrush(Brush);
}

static UBorder* MakeCell(UPanelWidget* Parent, UWidgetTree* WidgetTree, float FixedWidth, const FLinearColor& BorderColor)
{
    if (!Parent || !WidgetTree) return nullptr;
    UBorder* CellBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    // White cell border
    FSlateBrush B; B.DrawAs = ESlateBrushDrawType::Box; B.TintColor = BorderColor; B.Margin = FMargin(0.f);
    CellBorder->SetBrush(B);
    CellBorder->SetPadding(FMargin(0.f));
    if (UHorizontalBox* HB = Cast<UHorizontalBox>(Parent))
    {
        if (UHorizontalBoxSlot* Slot = HB->AddChildToHorizontalBox(CellBorder))
        {
            Slot->SetPadding(FMargin(1.f, 0.f));
            Slot->SetHorizontalAlignment(HAlign_Fill);
            Slot->SetVerticalAlignment(VAlign_Fill);
        }
    }
    // Size box to enforce strict width
    USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass());
    Size->SetWidthOverride(FixedWidth);
    Size->SetMinDesiredHeight(52.f);
    // Inner background for contrast
    UBorder* Inner = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass());
    ApplyBorderBrush(Inner, FLinearColor(0.f, 0.f, 0.f, 0.85f));
    // Symmetric padding and vertical centering
    Inner->SetPadding(FMargin(InventoryUIConstants::Padding_InnerTiny, InventoryUIConstants::Padding_Cell));
    Inner->SetHorizontalAlignment(HAlign_Fill);
    Inner->SetVerticalAlignment(VAlign_Center);
    Size->SetContent(Inner);
    CellBorder->SetContent(Size);
    return Inner; // return inner border to place actual content
}

void UInventoryItemEntryWidget::RebuildUI()
{
	if (!WidgetTree)
	{
		return;
	}

 // Outer white border around the row
 UBorder* OuterBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowOuterBorder"));
 ApplyBorderBrush(OuterBorder, BorderColor);
 OuterBorder->SetPadding(FMargin(1.f));
 WidgetTree->RootWidget = OuterBorder;

 // Inner background border
 RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBorder"));
 ApplyBorderBrush(RootBorder, Background);
 RootBorder->SetPadding(FMargin(0.f));
 OuterBorder->SetContent(RootBorder);

	// Horizontal content
	RowHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowHBox"));
	RootBorder->SetContent(RowHBox);

 // Icon cell (fixed)
 UBorder* IconCell = MakeCell(RowHBox, WidgetTree, InventoryUIConstants::ColumnWidth_Icon, BorderColor);
 IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("Icon"));
 if (IconCell)
 {
     IconCell->SetContent(IconImage);
 }
	
 // Name (widest)
 UBorder* NameCell = MakeCell(RowHBox, WidgetTree, InventoryUIConstants::ColumnWidth_Name, BorderColor);
 NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Name"));
 NameText->SetColorAndOpacity(TextColor);
 if (NameText)
 {
     NameText->SetFont(ProjectStyle::GetMonoFont(InventoryUIConstants::FontSize_ItemEntry));
 }
 if (NameCell)
 {
     NameCell->SetContent(NameText);
 }

 // Quantity
 UBorder* QtyCell = MakeCell(RowHBox, WidgetTree, InventoryUIConstants::ColumnWidth_Quantity, BorderColor);
 CountText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Count"));
 CountText->SetColorAndOpacity(TextColor);
 if (CountText)
 {
     CountText->SetFont(ProjectStyle::GetMonoFont(InventoryUIConstants::FontSize_ItemEntry));
 }
 if (QtyCell)
 {
     QtyCell->SetContent(CountText);
 }

 // Mass
 UBorder* MassCell = MakeCell(RowHBox, WidgetTree, InventoryUIConstants::ColumnWidth_Mass, BorderColor);
 MassText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Mass"));
 MassText->SetColorAndOpacity(TextColor);
 if (MassText)
 {
     MassText->SetFont(ProjectStyle::GetMonoFont(InventoryUIConstants::FontSize_ItemEntry));
 }
 if (MassCell)
 {
     MassCell->SetContent(MassText);
 }

 // Volume
 UBorder* VolCell = MakeCell(RowHBox, WidgetTree, InventoryUIConstants::ColumnWidth_Volume, BorderColor);
 VolumeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Volume"));
 VolumeText->SetColorAndOpacity(TextColor);
 if (VolumeText)
 {
     VolumeText->SetFont(ProjectStyle::GetMonoFont(InventoryUIConstants::FontSize_ItemEntry));
 }
 if (VolCell)
 {
     VolCell->SetContent(VolumeText);
 }
}

void UInventoryItemEntryWidget::Refresh()
{
    // Update texts and icon
    if (NameText)
    {
		FText Name = FText::FromString(TEXT("<None>"));
		if (Def)
		{
			Name = Def->DisplayName.IsEmpty() ? FText::FromString(Def->GetName()) : Def->DisplayName;
		}
		NameText->SetText(Name);
	}
	if (CountText)
	{
		CountText->SetText(FText::FromString(FString::Printf(TEXT("x%d"), Count)));
	}
 if (MassText)
 {
     float Mass = 0.f;
     if (Def)
     {
         const float PerUnit = FMath::Max(0.f, Def->MassKg);
         Mass = PerUnit * static_cast<float>(Count);
     }
     MassText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), Mass)));
 }
 if (VolumeText)
 {
     VolumeText->SetText(FText::FromString(FString::Printf(TEXT("%.2f"), TotalVolume)));
 }
    if (IconImage)
    {
        const FVector2D IconSize = InventoryUIConstants::IconSize_ItemEntry_Vec;
        if (Def)
        {
            const FItemIconStyle Style = ItemIconHelper::CreateIconStyle();
            
            // Try synchronous load first
            UTexture2D* IconTex = ItemIconHelper::LoadIconSync(Def, Style);
            if (IconTex)
            {
                ItemIconHelper::ApplyIconToImage(IconImage, IconTex, IconSize);
            }
            else
            {
                // Clear icon while waiting
                ItemIconHelper::ApplyIconToImage(IconImage, nullptr, IconSize);
                
                // Request async load
                TWeakObjectPtr<UInventoryItemEntryWidget> WeakThis(this);
                TWeakObjectPtr<UImage> WeakImage(IconImage);
                UItemDefinition* RequestedDef = Def;
                FOnItemIconReady Callback = FOnItemIconReady::CreateLambda([WeakThis, WeakImage, RequestedDef, IconSize](const UItemDefinition* ReadyDef, UTexture2D* ReadyTex)
                {
                    if (!WeakThis.IsValid() || !WeakImage.IsValid() || ReadyTex == nullptr)
                    {
                        return;
                    }
                    // Ensure row still represents the same item type
                    if (WeakThis->Def != RequestedDef)
                    {
                        return;
                    }
                    ItemIconHelper::ApplyIconToImage(WeakImage.Get(), ReadyTex, IconSize);
                });
                ItemIconHelper::LoadIconAsync(Def, Style, MoveTemp(Callback));
            }
        }
        else
        {
            // No definition - clear icon
            ItemIconHelper::ApplyIconToImage(IconImage, nullptr, IconSize);
        }
    }
    if (RootBorder)
    {
        ApplyBorderBrush(RootBorder, Background);
    }
}
