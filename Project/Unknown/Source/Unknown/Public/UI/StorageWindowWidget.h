#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "StorageWindowWidget.generated.h"

class UStorageComponent;
class UStorageListWidget;
class UBorder;
class UVerticalBox;
class UTextBlock;
class UItemDefinition;

/**
 * Separate window widget for displaying storage container contents.
 * This is positioned independently to the left of the inventory screen.
 */
UCLASS()
class UNKNOWN_API UStorageWindowWidget : public UUserWidget
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category="Storage")
	void Open(UStorageComponent* InStorage, UItemDefinition* ContainerItemDef = nullptr);

	UFUNCTION(BlueprintCallable, Category="Storage")
	void Close();

	UFUNCTION(BlueprintPure, Category="Storage")
	bool IsOpen() const { return bOpen; }

	UFUNCTION(BlueprintCallable, Category="Storage")
	void Refresh();

	UFUNCTION()
	void OnStorageItemAdded(const FItemEntry& Item);

	UFUNCTION()
	void OnStorageItemRemoved(const FGuid& ItemId);

	UFUNCTION(BlueprintCallable, Category="Storage")
	void SetTerminalStyle(const FLinearColor& InBackground, const FLinearColor& InBorder, const FLinearColor& InText);

	UStorageListWidget* GetStorageList() const { return StorageList; }

	void UpdateVolumeReadout();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void RebuildUI();
	void UpdateTitle(UItemDefinition* ContainerItemDef);

	UPROPERTY(Transient)
	TObjectPtr<UStorageComponent> Storage;

	UPROPERTY(Transient)
	bool bOpen = false;

	// UI refs
	UPROPERTY(Transient)
	TObjectPtr<UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<UVerticalBox> RootVBox;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> VolumeText;

	UPROPERTY(Transient)
	TObjectPtr<UStorageListWidget> StorageList;

	// Style
	UPROPERTY(EditAnywhere, Category="Style")
	FLinearColor Background = FLinearColor(0.f, 0.f, 0.f, 1.0f);

	UPROPERTY(EditAnywhere, Category="Style")
	FLinearColor Border = FLinearColor::White;

	UPROPERTY(EditAnywhere, Category="Style")
	FLinearColor Text = FLinearColor::White;
};

