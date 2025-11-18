#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InventoryItemEntryWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UImage;
class UTextBlock;
class UItemDefinition;

// Right-click context request on a row
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryRowContextRequested, UItemDefinition*, ItemType, FVector2D, ScreenPosition);
// Hover notifications for a row
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInventoryRowHovered, UItemDefinition*, ItemType);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryRowUnhovered);

/** One row in the inventory list: icon, name, quantity, mass, total volume */
UCLASS()
class UNKNOWN_API UInventoryItemEntryWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	// Set row data
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void SetData(UItemDefinition* InDef, int32 InCount, float InTotalVolume);

	UFUNCTION(BlueprintPure, Category="Inventory")
	UItemDefinition* GetDefinition() const { return Def; }

	// Fired when user requests a context menu (RMB) on this row
	UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
	FOnInventoryRowContextRequested OnContextRequested;

	// Terminal style colors
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor BorderColor = FLinearColor::White;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor Background = FLinearColor(0.f,0.f,0.f,0.85f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Style")
	FLinearColor TextColor = FLinearColor::White;

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

	void RebuildUI();
	void Refresh();

	// Data
 UPROPERTY(Transient)
 TObjectPtr<UItemDefinition> Def;
	UPROPERTY(Transient)
	int32 Count = 0;
	UPROPERTY(Transient)
	float TotalVolume = 0.f;

    // UI
    UPROPERTY(Transient)
    TObjectPtr<UBorder> RootBorder;
    UPROPERTY(Transient)
    TObjectPtr<UHorizontalBox> RowHBox;
    UPROPERTY(Transient)
    TObjectPtr<UImage> IconImage;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> NameText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> CountText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> MassText;
    UPROPERTY(Transient)
    TObjectPtr<UTextBlock> VolumeText;

public:
    // Fired when the mouse hovers/unhovers this row
    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryRowHovered OnHovered;

    UPROPERTY(BlueprintAssignable, Category="Inventory|Events")
    FOnInventoryRowUnhovered OnUnhovered;
};
