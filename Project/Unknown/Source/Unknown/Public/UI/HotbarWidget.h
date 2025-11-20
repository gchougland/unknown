#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlateFwd.h"
#include "Widgets/SWidget.h"
#include "Inventory/ItemTypes.h" // For FItemEntry in UFUNCTION params
#include "HotbarWidget.generated.h"

class UHotbarComponent;
class UItemDefinition;
class UInventoryComponent;
class UVerticalBox;
class UBorder;
class UImage;
class UTextBlock;

/**
 * Minimal hotbar widget that binds to a UHotbarComponent and mirrors its state for UI.
 * It does not render custom Slate yet; intended for logic tests and as a binding stub.
 */
UCLASS()
class UNKNOWN_API UHotbarWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Hotbar")
	void SetHotbar(UHotbarComponent* InHotbar);

	UFUNCTION(BlueprintPure, Category="Hotbar")
	UHotbarComponent* GetHotbar() const { return Hotbar; }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	int32 GetActiveIndex() const { return ActiveIndex; }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	FGuid GetActiveItemId() const { return ActiveItemId; }

	UFUNCTION(BlueprintPure, Category="Hotbar")
	UItemDefinition* GetAssignedType(int32 Index) const;

	// Style
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor SlotBackground = FLinearColor(0.f, 0.f, 0.f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor BorderColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor ActiveBackground = FLinearColor(0.05f, 0.05f, 0.05f, 0.95f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor TextColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FVector2D SlotSize = FVector2D(64.f, 64.f);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

	void RebuildSlots();
	void RefreshAll();
	void RefreshSlotVisual(int32 Index);
	void UpdateSlotBackground(int32 Index);
	void UpdateSlotIcon(int32 Index);
	void UpdateSlotQuantity(int32 Index);

	UFUNCTION()
	void OnSlotAssigned(int32 Index, UItemDefinition* ItemType);

	UFUNCTION()
	void OnActiveChanged(int32 NewIndex, FGuid ItemId);

	// Inventory change handlers to keep quantities in sync
	UFUNCTION()
	void OnInventoryItemAdded(const FItemEntry& Item);

	UFUNCTION()
	void OnInventoryItemRemoved(const FGuid& ItemId);

	// Bound hotbar comp
	UPROPERTY()
	TObjectPtr<UHotbarComponent> Hotbar;

	// Constructed UMG subtree refs
	UPROPERTY(Transient)
	TObjectPtr<class UVerticalBox> RootBox;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UBorder>> SlotBorders;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UImage>> SlotIcons;

	UPROPERTY(Transient)
	TArray<TObjectPtr<class UTextBlock>> SlotLabels;

	// Per-slot hotkey labels (top-left corner, always visible)
	UPROPERTY(Transient)
	TArray<TObjectPtr<class UTextBlock>> SlotHotkeys;

    // Bound inventory for live quantity updates
    UPROPERTY(Transient)
    TObjectPtr<UInventoryComponent> BoundInventory;

    // Tracks whether the slot widget tree has been constructed to avoid rebuilding and breaking references
    UPROPERTY(Transient)
    bool bSlotsBuilt = false;

    // Mirrored state
    UPROPERTY(Transient)
    TArray<TObjectPtr<UItemDefinition>> AssignedTypes;

	UPROPERTY(Transient)
	int32 ActiveIndex = INDEX_NONE;

	UPROPERTY(Transient)
	FGuid ActiveItemId;
};
