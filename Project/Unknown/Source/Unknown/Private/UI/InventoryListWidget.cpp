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
	// Only refresh if the widget is already constructed (RootList exists)
	// Otherwise, RebuildWidget() will call RebuildFromInventory() automatically
	if (RootList)
	{
		Refresh();
	}
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
		// RebuildFromInventory will use the current Inventory member variable
		// Only rebuild if we have inventory set, otherwise it will be called later
		if (Inventory)
		{
			RebuildFromInventory();
		}
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

void UInventoryListWidget::HandleRowLeftClicked(UItemDefinition* ItemType)
{
	OnRowLeftClicked.Broadcast(ItemType);
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
		UE_LOG(LogTemp, Warning, TEXT("[InventoryListWidget] BuildAggregate: Inventory is null"));
		return Out;
	}
	
	const TArray<FItemEntry>& Entries = Inventory->GetEntries();
	UE_LOG(LogTemp, Display, TEXT("[InventoryListWidget] BuildAggregate: Inventory has %d entries"), Entries.Num());
	
	// Aggregate by pointer
	TMap<UItemDefinition*, FAggregateRow> Map;
	for (const FItemEntry& E : Entries)
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
	UE_LOG(LogTemp, Display, TEXT("[InventoryListWidget] BuildAggregate: Created %d aggregate rows"), Out.Num());
	
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
		UE_LOG(LogTemp, Warning, TEXT("[InventoryListWidget] RebuildFromInventory: WidgetTree=%p, RootList=%p"), WidgetTree ? WidgetTree.Get() : nullptr, RootList ? RootList.Get() : nullptr);
		return;
	}
	
	// Clear existing children
	RootList->ClearChildren();
	
	const TArray<FAggregateRow> Rows = BuildAggregate();
	UE_LOG(LogTemp, Display, TEXT("[InventoryListWidget] RebuildFromInventory: Adding %d rows to list"), Rows.Num());
	
	for (const FAggregateRow& R : Rows)
	{
		UInventoryItemEntryWidget* RowWidget = WidgetTree->ConstructWidget<UInventoryItemEntryWidget>(UInventoryItemEntryWidget::StaticClass());
		if (!RowWidget)
		{
			UE_LOG(LogTemp, Error, TEXT("[InventoryListWidget] Failed to construct row widget"));
			continue;
		}
		
		// Set data first
		RowWidget->SetData(R.Def, R.Count, R.TotalVolume);
		
		// Bind events
		RowWidget->OnContextRequested.AddDynamic(this, &UInventoryListWidget::HandleRowContextRequested);
		RowWidget->OnLeftClicked.AddDynamic(this, &UInventoryListWidget::HandleRowLeftClicked);
		RowWidget->OnHovered.AddDynamic(this, &UInventoryListWidget::HandleRowHovered);
		RowWidget->OnUnhovered.AddDynamic(this, &UInventoryListWidget::HandleRowUnhovered);
		
		// Add to RootList - this will trigger widget construction automatically
		if (UVerticalBoxSlot* VBSlot = RootList->AddChildToVerticalBox(RowWidget))
		{
			VBSlot->SetPadding(FMargin(2.f));
			UE_LOG(LogTemp, Verbose, TEXT("[InventoryListWidget] Added row widget for %s"), R.Def ? *R.Def->GetName() : TEXT("NULL"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[InventoryListWidget] Failed to add row widget to RootList"));
		}
	}
	
	// Force the RootList to update its slate representation
	// This ensures all child widgets are properly constructed and visible
	if (RootList)
	{
		RootList->TakeWidget();
	}
}
