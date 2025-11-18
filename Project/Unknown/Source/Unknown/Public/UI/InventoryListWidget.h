#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryListWidget.generated.h"

class UVerticalBox;
class UInventoryItemEntryWidget;
class UInventoryComponent;
class UItemDefinition;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySelectionChanged, UItemDefinition*, ItemType, int32, Count);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryRowContextMenu, UItemDefinition*, ItemType, FVector2D, ScreenPosition);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryRowHoveredGlobal, UItemDefinition*, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRowUnhoveredGlobal);

/** Aggregated list of inventory items (per ItemDefinition) */
UCLASS()
class UNKNOWN_API UInventoryListWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetInventory(UInventoryComponent* InInventory);

	UFUNCTION(BlueprintPure, Category="Inventory")
	UInventoryComponent* GetInventory() const { return Inventory; }

	// Rebuild aggregation and refresh the list
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void Refresh();

	// Selection API (by type)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SelectType(UItemDefinition* Def);

	UFUNCTION(BlueprintPure, Category="Inventory")
	UItemDefinition* GetSelectedType() const { return SelectedType; }

 // Event when selection changes
 UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
 FOnInventorySelectionChanged OnSelectionChanged;

 // Fired when a row requests a context menu
 UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
 FOnInventoryRowContextMenu OnRowContextRequested;

 // Fired when any row in the list is hovered/unhovered
 UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
 FOnInventoryRowHoveredGlobal OnRowHovered;

 UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
 FOnInventoryRowUnhoveredGlobal OnRowUnhovered;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

 UFUNCTION()
 void HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition);

 UFUNCTION()
 void HandleRowHovered(UItemDefinition* ItemType);

 UFUNCTION()
 void HandleRowUnhovered();

	void RebuildUI();
	void RebuildFromInventory();

	// Simple aggregation struct
	struct FAggregateRow
	{
		UItemDefinition* Def = nullptr;
		int32 Count = 0;
		float TotalVolume = 0.f;
	};

	TArray<FAggregateRow> BuildAggregate() const;

	// Data
	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UItemDefinition> SelectedType;

	// UI
	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootList;
};
