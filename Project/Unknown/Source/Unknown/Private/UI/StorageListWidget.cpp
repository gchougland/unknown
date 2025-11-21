#include "UI/StorageListWidget.h"
#include "UI/InventoryItemEntryWidget.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Blueprint/WidgetTree.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryUIConstants.h"
#include "UI/ProjectStyle.h"

void UStorageListWidget::SetStorage(UStorageComponent* InStorage)
{
	Storage = InStorage;
	Refresh();
}

void UStorageListWidget::Refresh()
{
	RebuildFromStorage();
	UpdateVolumeReadout();
}

void UStorageListWidget::SetTitle(const FText& InTitle)
{
	if (TitleText)
	{
		TitleText->SetText(InTitle);
	}
}

void UStorageListWidget::UpdateVolumeReadout()
{
	if (!VolumeText || !Storage)
	{
		return;
	}

	const float Used = Storage->GetUsedVolume();
	const float Max = Storage->MaxVolume;
	FText VolumeTextValue = FText::Format(
		NSLOCTEXT("StorageList", "VolumeFormat", "Volume: {0} / {1}"),
		FText::AsNumber(Used, &FNumberFormattingOptions::DefaultWithGrouping()),
		FText::AsNumber(Max, &FNumberFormattingOptions::DefaultWithGrouping())
	);
	VolumeText->SetText(VolumeTextValue);
}

TSharedRef<SWidget> UStorageListWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		RebuildUI();
		RebuildFromStorage();
	}
	return Super::RebuildWidget();
}

void UStorageListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	RebuildFromStorage();
	UpdateVolumeReadout();
}

void UStorageListWidget::HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition)
{
	OnRowContextRequested.Broadcast(ItemType, ScreenPosition);
}

void UStorageListWidget::HandleRowHovered(UItemDefinition* ItemType)
{
	OnRowHovered.Broadcast(ItemType);
}

void UStorageListWidget::HandleRowUnhovered()
{
	OnRowUnhovered.Broadcast();
}

void UStorageListWidget::HandleRowLeftClicked(UItemDefinition* ItemType)
{
	if (!Storage || !ItemType)
	{
		return;
	}

	// Find the first item entry with this definition and trigger left-click
	for (const FItemEntry& Entry : Storage->GetEntries())
	{
		if (Entry.Def == ItemType && Entry.ItemId.IsValid())
		{
			OnItemLeftClicked.Broadcast(Entry.ItemId);
			break;
		}
	}
}

void UStorageListWidget::RebuildUI()
{
	if (!WidgetTree)
	{
		return;
	}

	// Create root vertical box
	RootVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootVBox"));
	WidgetTree->RootWidget = RootVBox;

	// Build header row with column titles (like inventory has)
	BuildHeaderRow();

	// Create list container (will be populated in RebuildFromStorage)
	UVerticalBox* ListContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListContainer"));
	if (ListContainer && RootVBox)
	{
		if (UVerticalBoxSlot* ContainerSlot = RootVBox->AddChildToVerticalBox(ListContainer))
		{
			ContainerSlot->SetPadding(FMargin(4.f));
			FSlateChildSize FillSize;
			FillSize.SizeRule = ESlateSizeRule::Fill;
			ContainerSlot->SetSize(FillSize);
		}
	}
}

void UStorageListWidget::BuildHeaderRow()
{
	if (!WidgetTree || !RootVBox)
	{
		return;
	}

	// Header row with column titles (similar to inventory)
	UBorder* HeaderOuter = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
		WidgetTree,
		FLinearColor::White,
		FLinearColor::Black,
		FMargin(InventoryUIConstants::Padding_Cell),
		FMargin(0.f),
		TEXT("HeaderOuter"),
		TEXT("HeaderInner")
	);
	if (UVerticalBoxSlot* HeaderSlot = RootVBox->AddChildToVerticalBox(HeaderOuter))
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
			FLinearColor::White,
			FLinearColor::Black,
			FLinearColor::White
		);
	};

	// Columns: Icon, Name, Qty, Mass, Volume (match widths used by row widgets)
	MakeHeaderCell(InventoryUIConstants::ColumnWidth_Icon, TEXT(" "));
	MakeHeaderCell(InventoryUIConstants::ColumnWidth_Name, TEXT("Name"));
	MakeHeaderCell(InventoryUIConstants::ColumnWidth_Quantity, TEXT("Qty"));
	MakeHeaderCell(InventoryUIConstants::ColumnWidth_Mass, TEXT("Mass"));
	MakeHeaderCell(InventoryUIConstants::ColumnWidth_Volume, TEXT("Volume"));
}

TArray<UStorageListWidget::FAggregateRow> UStorageListWidget::BuildAggregate() const
{
	TArray<FAggregateRow> Out;
	if (!Storage)
	{
		return Out;
	}

	// Aggregate by ItemDefinition pointer
	TMap<UItemDefinition*, FAggregateRow> Map;
	for (const FItemEntry& E : Storage->GetEntries())
	{
		if (!E.Def) { continue; }
		FAggregateRow* Row = Map.Find(E.Def);
		if (!Row)
		{
			FAggregateRow NewRow;
			NewRow.Def = E.Def;
			NewRow.Count = 0;
			NewRow.TotalVolume = 0.f;
			Row = &Map.Add(E.Def, NewRow);
		}
		Row->Count += 1;
		Row->TotalVolume += FMath::Max(0.f, E.Def->VolumePerUnit);
		Row->ItemIds.Add(E.ItemId);
	}
	Map.GenerateValueArray(Out);

	// Sort by name
	Out.Sort([](const FAggregateRow& A, const FAggregateRow& B)
	{
		const FString AN = A.Def ? A.Def->GetName() : FString();
		const FString BN = B.Def ? B.Def->GetName() : FString();
		return AN < BN;
	});

	return Out;
}

void UStorageListWidget::RebuildFromStorage()
{
	if (!WidgetTree || !RootVBox)
	{
		return;
	}

	// Find or create the list container
	UVerticalBox* ListContainer = nullptr;
	if (RootVBox->GetChildrenCount() > 1)
	{
		// List container should be the second child (after header)
		ListContainer = Cast<UVerticalBox>(RootVBox->GetChildAt(1));
	}

	if (!ListContainer)
	{
		// Create list container if it doesn't exist
		ListContainer = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ListContainer"));
		if (ListContainer && RootVBox)
		{
			if (UVerticalBoxSlot* ListSlot = RootVBox->AddChildToVerticalBox(ListContainer))
			{
				ListSlot->SetPadding(FMargin(4.f));
			}
		}
	}

	if (!ListContainer)
	{
		return;
	}

	ListContainer->ClearChildren();

	const TArray<FAggregateRow> Rows = BuildAggregate();
	for (const FAggregateRow& R : Rows)
	{
		UInventoryItemEntryWidget* RowWidget = WidgetTree->ConstructWidget<UInventoryItemEntryWidget>(UInventoryItemEntryWidget::StaticClass());
		RowWidget->SetData(R.Def, R.Count, R.TotalVolume);
		
		// Bind events
		RowWidget->OnContextRequested.AddDynamic(this, &UStorageListWidget::HandleRowContextRequested);
		RowWidget->OnLeftClicked.AddDynamic(this, &UStorageListWidget::HandleRowLeftClicked);
		RowWidget->OnHovered.AddDynamic(this, &UStorageListWidget::HandleRowHovered);
		RowWidget->OnUnhovered.AddDynamic(this, &UStorageListWidget::HandleRowUnhovered);
		
		if (UVerticalBoxSlot* RowSlot = ListContainer->AddChildToVerticalBox(RowWidget))
		{
			RowSlot->SetPadding(FMargin(2.f));
		}
	}
}

