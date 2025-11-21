#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/ItemTypes.h"
#include "StorageListWidget.generated.h"

class UVerticalBox;
class UInventoryItemEntryWidget;
class UStorageComponent;
class UItemDefinition;
class UTextBlock;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageItemLeftClicked, const FGuid&, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnStorageRowContextRequested, UItemDefinition*, ItemType, FVector2D, ScreenPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStorageRowHovered, UItemDefinition*, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnStorageRowUnhovered);

/**
 * List widget for displaying storage container contents (similar to InventoryListWidget)
 */
UCLASS()
class UNKNOWN_API UStorageListWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Storage")
	void SetStorage(UStorageComponent* InStorage);

	UFUNCTION(BlueprintPure, Category="Storage")
	UStorageComponent* GetStorage() const { return Storage; }

	UFUNCTION(BlueprintCallable, Category="Storage")
	void Refresh();

	UFUNCTION(BlueprintCallable, Category="Storage")
	void SetTitle(const FText& InTitle);

	UFUNCTION(BlueprintCallable, Category="Storage")
	void UpdateVolumeReadout();

	// Event when an item is left-clicked (for transfer)
	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageItemLeftClicked OnItemLeftClicked;

	// Fired when a row requests a context menu
	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageRowContextRequested OnRowContextRequested;

	// Fired when any row in the list is hovered/unhovered
	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageRowHovered OnRowHovered;

	UPROPERTY(BlueprintAssignable, Category="Storage|Events")
	FOnStorageRowUnhovered OnRowUnhovered;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UFUNCTION()
	void HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition);

	UFUNCTION()
	void HandleRowHovered(UItemDefinition* ItemType);

	UFUNCTION()
	void HandleRowUnhovered();

	UFUNCTION()
	void HandleRowLeftClicked(UItemDefinition* ItemType);

	void RebuildUI();
	void BuildHeaderRow();
	void RebuildFromStorage();

	// Simple aggregation struct
	struct FAggregateRow
	{
		UItemDefinition* Def = nullptr;
		int32 Count = 0;
		float TotalVolume = 0.f;
		TArray<FGuid> ItemIds; // Store ItemIds for left-click transfers
	};

	TArray<FAggregateRow> BuildAggregate() const;

	// Data
	UPROPERTY(Transient)
	TObjectPtr<UStorageComponent> Storage;

	// UI
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootVBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VolumeText;
};

