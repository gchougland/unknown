#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlateFwd.h"
#include "Widgets/SWidget.h"
#include "Input/Reply.h"
#include "Inventory/ItemTypes.h" // FItemEntry for delegate handler params
#include "Inventory/EquipmentTypes.h" // EEquipmentSlot for equipment events
#include "InventoryScreenWidget.generated.h"

class UInventoryComponent;
class UStorageComponent;
class UEquipmentComponent;
class UBorder;
class UVerticalBox;
class UTextBlock;
class UImage;
class UInventoryListWidget;
class UEquipmentPanelWidget;
class UItemDefinition;
struct FGeometry;
struct FKeyEvent;
struct FKey;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRequestClose);

/** Minimal modal inventory screen (terminal-style). */
UCLASS()
class UNKNOWN_API UInventoryScreenWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void Open(UInventoryComponent* InInventory, UStorageComponent* InStorage = nullptr);

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void Close();

	UFUNCTION(BlueprintPure, Category="Inventory")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetTerminalStyle(const FLinearColor& InBackground, const FLinearColor& InBorder, const FLinearColor& InText);

    // Current selection helper for controller shortcuts (1–9 assign)
	UFUNCTION(BlueprintPure, Category="Inventory")
	UItemDefinition* GetSelectedItemType() const;

	// External/API: request a UI refresh from outside (e.g., after picking up an item)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void RefreshInventoryView();

	// Internal refresh (public so player controller can call it)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void Refresh();

	// Getters for context menu actions
	UInventoryComponent* GetInventory() const { return Inventory; }
	UEquipmentComponent* GetEquipment() const { return Equipment; }

	// Transfer handlers (public so player controller can bind to storage window)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void HandleStorageItemLeftClick(const FGuid& ItemId);

    // Fired when the user presses Tab/I/Escape inside the widget
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryRequestClose OnRequestClose;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
    virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
    virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	void RebuildUI();
	bool HandleCloseKey(const FKey& Key);

 UFUNCTION()
 void HandleRowContextRequested(UItemDefinition* ItemType, FVector2D ScreenPosition);

 UFUNCTION()
 void HandleListRowHovered(UItemDefinition* ItemType);

 UFUNCTION()
 void HandleListRowUnhovered();

 UFUNCTION()
 void HandleInventoryRowLeftClicked(UItemDefinition* ItemType);

	void OpenContextMenu(UItemDefinition* ItemType, const FVector2D& ScreenPosition);

	void HandleInventoryItemLeftClick(UItemDefinition* ItemType);

 // React to equipment changes so we can refresh the UI immediately after equip/unequip
 UFUNCTION()
 void HandleEquipmentEquipped(EEquipmentSlot InEquipSlot, const FItemEntry& Item);

 UFUNCTION()
 void HandleEquipmentUnequipped(EEquipmentSlot InEquipSlot, const FItemEntry& Item);

	UPROPERTY(Transient)
	TObjectPtr<UInventoryComponent> Inventory;

	UPROPERTY(Transient)
	TObjectPtr<UStorageComponent> Storage;

    UPROPERTY(Transient)
    bool bOpen = false;
    
    // Track if UI has been built to avoid rebuilding unnecessarily
    UPROPERTY(Transient)
    bool bUIBuilt = false;

	// UI refs
 UPROPERTY(Transient)
 TObjectPtr<UBorder> RootBorder;
 UPROPERTY(Transient)
 TObjectPtr<UVerticalBox> RootVBox;
 UPROPERTY(Transient)
 TObjectPtr<UTextBlock> TitleText;
 // Aggregated inventory list
 UPROPERTY(Transient)
 TObjectPtr<UInventoryListWidget> InventoryList;

 // Top-right volume readout (e.g., "Volume: 12.4 / 30")
 UPROPERTY(Transient)
 TObjectPtr<UTextBlock> VolumeText;

 // Info panel (below the list)
 UPROPERTY(Transient)
 TObjectPtr<UBorder> InfoOuterBorder;
 UPROPERTY(Transient)
 TObjectPtr<UBorder> InfoInnerBorder;
 UPROPERTY(Transient)
 TObjectPtr<UImage> InfoIcon;
 UPROPERTY(Transient)
 TObjectPtr<UTextBlock> InfoNameText;
 UPROPERTY(Transient)
 TObjectPtr<UTextBlock> InfoDescText;

 // Currently hovered item definition (for async icon updates)
 UPROPERTY(Transient)
 TObjectPtr<UItemDefinition> CurrentHoverDef;

	// Style
	UPROPERTY(EditAnywhere, Category="Style")
	FLinearColor Background = FLinearColor(0.f,0.f,0.f,1.0f);
	UPROPERTY(EditAnywhere, Category="Style")
	FLinearColor Border = FLinearColor::White;
    UPROPERTY(EditAnywhere, Category="Style")
    FLinearColor Text = FLinearColor::White;
private:
    void UpdateVolumeReadout();
    void UpdateInfoPanelForDef(UItemDefinition* Def);
    void UpdateInfoText(UItemDefinition* Def);
    void UpdateInfoIcon(UItemDefinition* Def);
    void ClearInfoPanel();

    // Helper methods for Open()
    void ResolveComponents(UInventoryComponent* InInventory, UStorageComponent* InStorage);
    void BindEvents();
    void InitializeUI();
    void PrewarmIcons();
    
    // Update storage window visibility without rebuilding entire UI
    void UpdateStorageWindowVisibility();

    // Equipment access for RMB EQUIP and equipment panel (set during Open)
    UPROPERTY(Transient)
    TObjectPtr<UEquipmentComponent> Equipment;

    // Body layout containers
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> InventoryColumnVBox;

    UPROPERTY(Transient)
    TObjectPtr<UEquipmentPanelWidget> EquipmentPanel;

    // Storage window is managed separately by FirstPersonPlayerController
    // No need to store it here
};
