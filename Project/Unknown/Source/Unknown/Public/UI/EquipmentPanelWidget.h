#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/EquipmentTypes.h"
#include "Inventory/ItemTypes.h" // FItemEntry for UFUNCTION params
#include "EquipmentPanelWidget.generated.h"

class UVerticalBox;
class UTextBlock;
class UBorder;
class UEquipmentComponent;
struct FEquipmentPanelSlateCache;

/** Simple equipment panel shown to the right of the inventory panel. */
UCLASS()
class UNKNOWN_API UEquipmentPanelWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    ~UEquipmentPanelWidget();
    UFUNCTION(BlueprintCallable, Category="Equipment")
    void SetEquipmentComponent(UEquipmentComponent* InEquipment);

    UFUNCTION(BlueprintCallable, Category="Equipment")
    void Refresh();

    // React to equipment changes
    UFUNCTION()
    void OnItemEquipped(EEquipmentSlot InSlot, const FItemEntry& Item);

    UFUNCTION()
    void OnItemUnequipped(EEquipmentSlot InSlot, const FItemEntry& Item);

protected:
    virtual TSharedRef<SWidget> RebuildWidget() override;
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

private:
    UPROPERTY(Transient)
    TObjectPtr<UVerticalBox> RootVBox;

    UPROPERTY(Transient)
    TObjectPtr<UEquipmentComponent> Equipment;

    FString GetSlotLabel(EEquipmentSlot InSlot) const;

    // Opaque Slate cache (defined in .cpp to avoid Slate/SlateCore types in header parsed by UHT)
    FEquipmentPanelSlateCache* Cache = nullptr;

    void UpdateAllSlots();
    void UpdateSlot(EEquipmentSlot InSlot);
};
