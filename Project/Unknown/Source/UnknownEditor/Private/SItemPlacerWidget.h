#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ContentBrowserDelegates.h"
#include "AssetRegistry/AssetData.h"

class UItemDefinition;
struct FAssetData;
struct FSlateBrush;

/** Dockable editor widget that lists ItemDefinition assets and places AItemPickup actors */
class SItemPlacerWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SItemPlacerWidget){}
	SLATE_END_ARGS();

	void Construct(const FArguments& InArgs);

private:
	// UI callbacks
	void OnAssetsActivated(const TArray<FAssetData>& ActivatedAssets, EAssetTypeActivationMethod::Type ActivationType);
	void OnAssetSelected(const FAssetData& AssetData);
	FReply OnPlaceAtCursorClicked();
	void OnSearchTextChanged(const FText& NewText);

	// Placement helpers
	void PlaceItemAtCursor(const FAssetData& AssetData);
	bool GetCursorWorldHit(FVector& OutLocation, FVector& OutNormal) const;

	// Filtering
	bool ShouldFilterAsset(const FAssetData& AssetData) const;

	// Utilities
	UItemDefinition* ResolveItemDefinition(const FAssetData& AssetData) const;

private:
	// State
	TSharedPtr<class SWidget> AssetPickerWidget;
	// Delegate to refresh the asset view when search text changes
	FRefreshAssetViewDelegate RefreshAssetViewDelegate;
	FText SearchText;
	FAssetData SelectedAssetForButton;
	bool bAlignToFloor = true;
	bool bSnapToGrid = true;
	float GridSize = 50.f;
};
