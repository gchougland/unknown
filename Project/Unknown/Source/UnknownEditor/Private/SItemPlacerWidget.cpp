#include "SItemPlacerWidget.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "ContentBrowserDelegates.h"
#include "IContentBrowserSingleton.h"
#include "AssetViewTypes.h"
#include "Editor.h"
#include "Editor/EditorEngine.h"
#include "Engine/Selection.h"
#include "LevelEditor.h"
#include "Modules/ModuleManager.h"
#include "Widgets/Input/SSearchBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Math/UnrealMathUtility.h"
#include "EditorViewportClient.h"
#include "Engine/World.h"

#define LOCTEXT_NAMESPACE "SItemPlacerWidget"

void SItemPlacerWidget::Construct(const FArguments& InArgs)
{
	// Build asset picker filtered to UItemDefinition
	FContentBrowserModule& CBModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	FAssetPickerConfig PickerConfig;
	PickerConfig.Filter.ClassPaths.Add(UItemDefinition::StaticClass()->GetClassPathName());
	PickerConfig.Filter.bRecursiveClasses = true; // ensure derived classes are included
	PickerConfig.InitialAssetViewType = EAssetViewType::Tile;
	PickerConfig.bAllowDragging = true;
	PickerConfig.bCanShowDevelopersFolder = true;
	PickerConfig.bShowBottomToolbar = false;
	PickerConfig.bAllowNullSelection = false;
	// Disable double-click/open activation to avoid accidental placement/crash; use drag-and-drop or the button instead.
	// PickerConfig.OnAssetsActivated = FOnAssetsActivated::CreateRaw(this, &SItemPlacerWidget::OnAssetsActivated);
	PickerConfig.OnAssetSelected = FOnAssetSelected::CreateRaw(this, &SItemPlacerWidget::OnAssetSelected);
	PickerConfig.bFocusSearchBoxWhenOpened = false;
	// Live filtering: register external asset filter and refresh delegate
	PickerConfig.OnShouldFilterAsset = FOnShouldFilterAsset::CreateRaw(this, &SItemPlacerWidget::ShouldFilterAsset);
	PickerConfig.RefreshAssetViewDelegates.Add(&RefreshAssetViewDelegate);

	AssetPickerWidget = CBModule.Get().CreateAssetPicker(PickerConfig);
	// Trigger an initial refresh so the external filter is honored immediately
	if (RefreshAssetViewDelegate.IsBound())
	{
		RefreshAssetViewDelegate.Execute(true /*UpdateSources*/);
	}

	ChildSlot
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center).Padding(2.f)
			[
				SNew(STextBlock).Text(LOCTEXT("SearchLabel", "Search:"))
			]
			+ SHorizontalBox::Slot().FillWidth(1.f).Padding(2.f)
			[
				SNew(SSearchBox)
				.OnTextChanged(this, &SItemPlacerWidget::OnSearchTextChanged)
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(6.f,0.f)
			[
				SNew(SButton)
				.Text(LOCTEXT("PlaceAtCursor", "Place At Camera"))
				.ToolTipText(LOCTEXT("PlaceAtCursor_TT", "Place the selected item at the camera aim point"))
				.OnClicked(this, &SItemPlacerWidget::OnPlaceAtCursorClicked)
			]
		]
		+ SVerticalBox::Slot().AutoHeight().Padding(4.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot().AutoWidth().Padding(2.f).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]{ return bAlignToFloor ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bAlignToFloor = (S == ECheckBoxState::Checked); })
				[ SNew(STextBlock).Text(LOCTEXT("AlignToFloor", "Align to floor")) ]
			]
			+ SHorizontalBox::Slot().AutoWidth().Padding(12.f,2.f).VAlign(VAlign_Center)
			[
				SNew(SCheckBox)
				.IsChecked_Lambda([this]{ return bSnapToGrid ? ECheckBoxState::Checked : ECheckBoxState::Unchecked; })
				.OnCheckStateChanged_Lambda([this](ECheckBoxState S){ bSnapToGrid = (S == ECheckBoxState::Checked); })
				[ SNew(STextBlock).Text(LOCTEXT("SnapToGrid", "Snap to grid")) ]
			]
		]
		+ SVerticalBox::Slot().FillHeight(1.f)
		[
			AssetPickerWidget.ToSharedRef()
		]
	];
}

void SItemPlacerWidget::OnAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationType)
{
	if (ActivatedAssets.Num() == 0)
	{
		return;
	}
	if (ActivationType == EAssetTypeActivationMethod::Opened || ActivationType == EAssetTypeActivationMethod::DoubleClicked)
	{
		PlaceItemAtCursor(ActivatedAssets[0]);
	}
}

FReply SItemPlacerWidget::OnPlaceAtCursorClicked()
{
	// Prefer the asset selected inside this picker
	if (SelectedAssetForButton.IsValid())
	{
		PlaceItemAtCursor(SelectedAssetForButton);
		return FReply::Handled();
	}
	// Fallback: query global Content Browser selection
	TArray<FAssetData> Selected;
	if (FContentBrowserModule* CB = FModuleManager::GetModulePtr<FContentBrowserModule>("ContentBrowser"))
	{
		CB->Get().GetSelectedAssets(Selected);
	}
	if (Selected.Num() > 0)
	{
		PlaceItemAtCursor(Selected[0]);
	}
	return FReply::Handled();
}

void SItemPlacerWidget::OnAssetSelected(const FAssetData& AssetData)
{
	SelectedAssetForButton = AssetData;
}

void SItemPlacerWidget::OnSearchTextChanged(const FText& NewText)
{
	SearchText = NewText;
	// Trigger a refresh on the asset view so our external filter runs
	if (RefreshAssetViewDelegate.IsBound())
	{
		// UpdateSources=true ensures the view fully re-evaluates external filters
		RefreshAssetViewDelegate.Execute(true /*UpdateSources*/);
	}
}

bool SItemPlacerWidget::ShouldFilterAsset(const FAssetData& AssetData) const
{
	// Only filter UItemDefinition assets; others are excluded by the class path filter already.
	if (!AssetData.IsValid())
	{
		return true; // filter out invalid
	}
	// If no search, do not filter out
	const FString Query = SearchText.ToString().TrimStartAndEnd();
	if (Query.IsEmpty())
	{
		return false;
	}
	// Match against asset name and package path to be resilient
	const FString Name = AssetData.AssetName.ToString();
	const FString Package = AssetData.PackageName.ToString();
	const bool bNameMatch = Name.Contains(Query, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	const bool bPathMatch = Package.Contains(Query, ESearchCase::IgnoreCase, ESearchDir::FromStart);
	return !(bNameMatch || bPathMatch);
}

UItemDefinition* SItemPlacerWidget::ResolveItemDefinition(const FAssetData& AssetData) const
{
	if (AssetData.IsValid() && AssetData.AssetClassPath == UItemDefinition::StaticClass()->GetClassPathName())
	{
		return Cast<UItemDefinition>(AssetData.GetAsset());
	}
	return nullptr;
}

bool SItemPlacerWidget::GetCursorWorldHit(FVector& OutLocation, FVector& OutNormal) const
{
	OutLocation = FVector::ZeroVector;
	OutNormal = FVector::UpVector;

	// Find a perspective level viewport client
	FEditorViewportClient* FoundClient = nullptr;
	for (FEditorViewportClient* VC : GEditor->GetAllViewportClients())
	{
		if (VC && VC->IsPerspective())
		{
			FoundClient = VC; break;
		}
	}
	if (!FoundClient)
	{
		return false;
	}

	// Ray from camera forward
	const FVector CamLoc = FoundClient->GetViewLocation();
	const FVector CamDir = FoundClient->GetViewRotation().Vector();
	const FVector Start = CamLoc;
	const FVector End = Start + CamDir * 100000.f;

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return false;
	}

	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPlacerTrace), false);
	bool bHit = World->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_Visibility, Params);
	if (bHit)
	{
		OutLocation = Hit.Location;
		OutNormal = Hit.Normal;
		return true;
	}
	// Fallback: place in front of camera
	OutLocation = End;
	OutNormal = FVector::UpVector;
	return true;
}

void SItemPlacerWidget::PlaceItemAtCursor(const FAssetData& AssetData)
{
	UItemDefinition* Def = ResolveItemDefinition(AssetData);
	if (!Def)
	{
		return;
	}
	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World)
	{
		return;
	}

	FVector Loc; FVector Normal;
	if (!GetCursorWorldHit(Loc, Normal))
	{
		return;
	}

	if (bSnapToGrid && GridSize > 1.0f)
	{
		const float InvGrid = 1.0f / GridSize;
		Loc.X = FMath::RoundToFloat(Loc.X * InvGrid) * GridSize;
		Loc.Y = FMath::RoundToFloat(Loc.Y * InvGrid) * GridSize;
		Loc.Z = FMath::RoundToFloat(Loc.Z * InvGrid) * GridSize;
	}

	FRotator Rot = FRotator::ZeroRotator;
	if (bAlignToFloor)
	{
		Rot = Normal.Rotation();
	}

 // Compose a base transform from cursor location/orientation and then apply the item's DefaultDropTransform
 // so any authoring-time offsets (including scale) are preserved during placement.
 const FTransform BaseTransform(Rot, Loc, FVector(1.f, 1.f, 1.f));
 const FTransform FinalTransform = Def ? (Def->DefaultDropTransform * BaseTransform) : BaseTransform;

 FActorSpawnParameters Params;
 Params.Name = MakeUniqueObjectName(World, AItemPickup::StaticClass(), FName(TEXT("ItemPickup")));
 Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
 AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FinalTransform, Params);
	if (Pickup)
	{
		Pickup->SetItemDef(Def);
		// Select in editor
		GEditor->SelectNone(false, true, false);
		GEditor->SelectActor(Pickup, true, true, true);
		GEditor->NoteSelectionChange();
	}
}

#undef LOCTEXT_NAMESPACE
