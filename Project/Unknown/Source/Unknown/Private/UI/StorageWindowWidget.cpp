#include "UI/StorageWindowWidget.h"
#include "UI/StorageListWidget.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Components/Border.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Blueprint/WidgetTree.h"
#include "UI/TerminalWidgetHelpers.h"
#include "UI/InventoryScreenWidgetBuilder.h"
#include "UI/InventoryUIConstants.h"

TSharedRef<SWidget> UStorageWindowWidget::RebuildWidget()
{
	if (WidgetTree)
	{
		RebuildUI();
	}
	return Super::RebuildWidget();
}

void UStorageWindowWidget::NativeConstruct()
{
	Super::NativeConstruct();
	// Don't set to Collapsed here - let Open() control visibility
	// This allows embedded widgets to be visible when their parent is visible
	SetIsFocusable(true);
}

void UStorageWindowWidget::NativeDestruct()
{
	if (StorageList)
	{
		StorageList->OnItemLeftClicked.RemoveAll(this);
		StorageList->OnRowContextRequested.RemoveAll(this);
		StorageList->OnRowHovered.RemoveAll(this);
		StorageList->OnRowUnhovered.RemoveAll(this);
	}
	Super::NativeDestruct();
}

void UStorageWindowWidget::SetTerminalStyle(const FLinearColor& InBackground, const FLinearColor& InBorder, const FLinearColor& InText)
{
	Background = InBackground;
	Border = InBorder;
	Text = InText;
	RebuildUI();
	if (VolumeText)
	{
		VolumeText->SetColorAndOpacity(Text);
	}
	if (TitleText)
	{
		TitleText->SetColorAndOpacity(Text);
	}
}

void UStorageWindowWidget::Open(UStorageComponent* InStorage, UItemDefinition* ContainerItemDef)
{
	Storage = InStorage;
	bOpen = true;

	if (!Storage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[StorageWindow] Open called with null Storage component"));
		return;
	}

	// Ensure UI is built before trying to access StorageList
	if (!StorageList && WidgetTree)
	{
		RebuildUI();
	}

	UpdateTitle(ContainerItemDef);
	UpdateVolumeReadout();

	// Bind storage delegates to refresh when items are added/removed
	if (Storage)
	{
		Storage->OnItemAdded.RemoveAll(this);
		Storage->OnItemRemoved.RemoveAll(this);
		Storage->OnItemAdded.AddDynamic(this, &UStorageWindowWidget::OnStorageItemAdded);
		Storage->OnItemRemoved.AddDynamic(this, &UStorageWindowWidget::OnStorageItemRemoved);
	}

	if (StorageList)
	{
		StorageList->SetStorage(Storage);
		StorageList->Refresh();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[StorageWindow] StorageList is null after Open()"));
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UStorageWindowWidget::Close()
{
	bOpen = false;
	SetVisibility(ESlateVisibility::Collapsed);
	
	// Unbind storage delegates
	if (Storage)
	{
		Storage->OnItemAdded.RemoveAll(this);
		Storage->OnItemRemoved.RemoveAll(this);
	}
	
	Storage = nullptr;
}

void UStorageWindowWidget::Refresh()
{
	UpdateVolumeReadout();
	if (StorageList && Storage)
	{
		StorageList->Refresh();
	}
}

void UStorageWindowWidget::OnStorageItemAdded(const FItemEntry& Item)
{
	Refresh();
}

void UStorageWindowWidget::OnStorageItemRemoved(const FGuid& ItemId)
{
	Refresh();
}

void UStorageWindowWidget::UpdateVolumeReadout()
{
	if (!VolumeText || !Storage)
	{
		return;
	}

	const float Used = Storage->GetUsedVolume();
	const float Max = Storage->MaxVolume;
	FText VolumeTextValue = FText::Format(
		NSLOCTEXT("StorageWindow", "VolumeFormat", "Volume: {0} / {1}"),
		FText::AsNumber(FMath::RoundToInt(Used)),
		FText::AsNumber(FMath::RoundToInt(Max))
	);
	VolumeText->SetText(VolumeTextValue);
}

void UStorageWindowWidget::UpdateTitle(UItemDefinition* ContainerItemDef)
{
	if (!TitleText)
	{
		return;
	}

	FText Title;
	if (ContainerItemDef)
	{
		FString TitleStr = ContainerItemDef->DisplayName.IsEmpty() 
			? ContainerItemDef->GetName() 
			: ContainerItemDef->DisplayName.ToString();
		TitleStr = TitleStr.ToUpper();
		Title = FText::FromString(TitleStr);
	}
	else
	{
		Title = NSLOCTEXT("StorageWindow", "DefaultTitle", "STORAGE");
	}

	TitleText->SetText(Title);
}

void UStorageWindowWidget::RebuildUI()
{
	if (!WidgetTree)
	{
		return;
	}

	// Build root layout similar to inventory screen
	InventoryScreenWidgetBuilder::FWidgetReferences Refs;
	InventoryScreenWidgetBuilder::BuildRootLayout(WidgetTree, Border, Background, Refs);
	RootBorder = Refs.RootBorder;
	RootVBox = Refs.RootVBox;

	// Build top bar with title (left) and volume readout (right)
	UHorizontalBox* TopBarHBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("TopBarHBox"));
	if (TopBarHBox && RootVBox)
	{
		if (UVerticalBoxSlot* TopBarSlot = RootVBox->AddChildToVerticalBox(TopBarHBox))
		{
			TopBarSlot->SetPadding(FMargin(0.f, 0.f, 0.f, InventoryUIConstants::Padding_TopBar));
			TopBarSlot->SetSize(ESlateSizeRule::Automatic);
		}

		// Volume readout on the left
		VolumeText = TerminalWidgetHelpers::CreateTerminalTextBlock(
			WidgetTree,
			TEXT("Volume: 0 / 0"),
			InventoryUIConstants::FontSize_VolumeReadout,
			Text,
			TEXT("VolumeText")
		);
		if (VolumeText && TopBarHBox)
		{
			if (UHorizontalBoxSlot* VolumeSlot = TopBarHBox->AddChildToHorizontalBox(VolumeText))
			{
				VolumeSlot->SetHorizontalAlignment(HAlign_Left);
				VolumeSlot->SetVerticalAlignment(VAlign_Center);
				VolumeSlot->SetSize(ESlateSizeRule::Automatic);
			}
		}

		// Title text on the right
		TitleText = TerminalWidgetHelpers::CreateTerminalTextBlock(
			WidgetTree,
			TEXT("STORAGE"),
			InventoryUIConstants::FontSize_Title,
			Text,
			TEXT("TitleText")
		);
		if (TitleText && TopBarHBox)
		{
			TitleText->SetShadowOffset(InventoryUIConstants::ShadowOffset);
			TitleText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, InventoryUIConstants::ShadowOpacity));
			if (UHorizontalBoxSlot* TitleSlot = TopBarHBox->AddChildToHorizontalBox(TitleText))
			{
				TitleSlot->SetHorizontalAlignment(HAlign_Right);
				TitleSlot->SetVerticalAlignment(VAlign_Center);
				FSlateChildSize FillSize;
				FillSize.SizeRule = ESlateSizeRule::Fill;
				TitleSlot->SetSize(FillSize);
			}
		}

		// Divider below top bar (simple border)
		UBorder* TopBarDivider = TerminalWidgetHelpers::CreateTerminalBorderedPanel(
			WidgetTree,
			Text,
			FLinearColor::Black,
			FMargin(0.f),
			FMargin(0.f),
			TEXT("TopBarDivider"),
			TEXT("TopBarDividerInner")
		);
		if (TopBarDivider && RootVBox)
		{
			if (UVerticalBoxSlot* DividerSlot = RootVBox->AddChildToVerticalBox(TopBarDivider))
			{
				DividerSlot->SetPadding(FMargin(InventoryUIConstants::Padding_InnerSmall, 0.f, InventoryUIConstants::Padding_InnerSmall, 0.f));
				DividerSlot->SetSize(ESlateSizeRule::Automatic);
			}
		}
	}

	// Create storage list widget
	StorageList = WidgetTree->ConstructWidget<UStorageListWidget>(UStorageListWidget::StaticClass(), TEXT("StorageList"));
	if (StorageList && RootVBox)
	{
		// Set storage if we have one
		if (Storage)
		{
			StorageList->SetStorage(Storage);
		}

		// Add to root vbox
		if (UVerticalBoxSlot* StorageSlot = RootVBox->AddChildToVerticalBox(StorageList))
		{
			StorageSlot->SetPadding(FMargin(4.f));
			FSlateChildSize FillSize;
			FillSize.SizeRule = ESlateSizeRule::Fill;
			StorageSlot->SetSize(FillSize);
		}
	}
}

