#include "UI/InventoryListWidget.h"
#include "UI/InventoryItemEntryWidget.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Blueprint/WidgetTree.h"

void UInventoryListWidget::SetInventory(UInventoryComponent* InInventory)
{
	Inventory = InInventory;
	Refresh();
}

void UInventoryListWidget::Refresh()
{
	RebuildFromInventory();
}

void UInventoryListWidget::SelectType(UItemDefinition* Def)
{
	if (SelectedType == Def)
	{
		return;
	}
	SelectedType = Def;
	int32 Count = 0;
	if (Inventory && Def)
	{
		Count = Inventory->CountByDef(Def);
	}
	OnSelectionChanged.Broadcast(SelectedType, Count);
}

TSharedRef<SWidget> UInventoryListWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		RebuildUI();
		RebuildFromInventory();
	}
	return Super::RebuildWidget();
}

void UInventoryListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildFromInventory();
}

void UInventoryListWidget::HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition)
{
    OnRowContextRequested.Broadcast(ItemType, ScreenPosition);
}

void UInventoryListWidget::HandleRowHovered(UItemDefinition* ItemType)
{
    OnRowHovered.Broadcast(ItemType);
}

void UInventoryListWidget::HandleRowUnhovered()
{
    OnRowUnhovered.Broadcast();
}

void UInventoryListWidget::RebuildUI()
{
	if (!WidgetTree)
	{
		return;
	}
	RootList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootList"));
	WidgetTree->RootWidget = RootList;
}

TArray<UInventoryListWidget::FAggregateRow> UInventoryListWidget::BuildAggregate() const
{
	TArray<FAggregateRow> Out;
	if (!Inventory)
	{
		return Out;
	}
	// Aggregate by pointer
	TMap<UItemDefinition*, FAggregateRow> Map;
	for (const FItemEntry& E : Inventory->GetEntries())
	{
		if (!E.Def) { continue; }
		FAggregateRow* Row = Map.Find(E.Def);
		if (!Row)
		{
			FAggregateRow NewRow; NewRow.Def = E.Def; NewRow.Count = 0; NewRow.TotalVolume = 0.f;
			Row = &Map.Add(E.Def, NewRow);
		}
		Row->Count += 1;
		Row->TotalVolume += FMath::Max(0.f, E.Def->VolumePerUnit);
	}
	Map.GenerateValueArray(Out);
	// Optional: sort by name
	Out.Sort([](const FAggregateRow& A, const FAggregateRow& B)
	{
		const FString AN = A.Def ? A.Def->GetName() : FString();
		const FString BN = B.Def ? B.Def->GetName() : FString();
		return AN < BN;
	});
	return Out;
}

void UInventoryListWidget::RebuildFromInventory()
{
	if (!WidgetTree || !RootList)
	{
		return;
	}
	RootList->ClearChildren();
	const TArray<FAggregateRow> Rows = BuildAggregate();
 for (const FAggregateRow& R : Rows)
 {
     UInventoryItemEntryWidget* RowWidget = WidgetTree->ConstructWidget<UInventoryItemEntryWidget>(UInventoryItemEntryWidget::StaticClass());
     RowWidget->SetData(R.Def, R.Count, R.TotalVolume);
     RowWidget->OnContextRequested.AddDynamic(this, &UInventoryListWidget::HandleRowContextRequested);
     RowWidget->OnHovered.AddDynamic(this, &UInventoryListWidget::HandleRowHovered);
     RowWidget->OnUnhovered.AddDynamic(this, &UInventoryListWidget::HandleRowUnhovered);
     if (UVerticalBoxSlot* VBSlot = RootList->AddChildToVerticalBox(RowWidget))
     {
         VBSlot->SetPadding(FMargin(2.f));
     }
 }
}
