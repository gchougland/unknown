// Save slot menu widget for selecting save slots
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveSlotMenuWidget.generated.h"

UENUM(BlueprintType)
enum class ESaveSlotMenuMode : uint8
{
	NewGame,
	LoadGame,
	Save
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveSlotSelected, const FString&, SlotId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveSlotMenuBack);

UCLASS()
class UNKNOWN_API USaveSlotMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USaveSlotMenuWidget(const FObjectInitializer& ObjectInitializer);

	// Set the menu mode (NewGame or LoadGame)
	UFUNCTION(BlueprintCallable, Category="SaveSlotMenu")
	void SetMode(ESaveSlotMenuMode InMode);

	// Get the current menu mode
	UFUNCTION(BlueprintCallable, Category="SaveSlotMenu")
	ESaveSlotMenuMode GetMode() const { return MenuMode; }

	// Refresh the save list (placeholder for now)
	UFUNCTION(BlueprintCallable, Category="SaveSlotMenu")
	void RefreshSaveList();

	// Delegate for save slot selection
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnSaveSlotSelected OnSaveSlotSelected;

	// Delegate for back button
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnSaveSlotMenuBack OnBackClicked;

	// Delegate for create new slot button
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnCreateNewSlotClicked);
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnCreateNewSlotClicked OnCreateNewSlotClicked;

	// Delegate for delete slot button
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeleteSlotClicked);
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnDeleteSlotClicked OnDeleteSlotButtonClicked;

	// Delegate for save button (Save mode)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveButtonClicked);
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnSaveButtonClicked OnSaveButtonClicked;

	// Delegate for load button (Load mode)
	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadButtonClicked);
	UPROPERTY(BlueprintAssignable, Category="SaveSlotMenu")
	FOnLoadButtonClicked OnLoadButtonClicked;

	// Set the selected slot
	UFUNCTION(BlueprintCallable, Category="SaveSlotMenu")
	void SetSelectedSlot(const FString& SlotId);

	// Get the selected slot
	UFUNCTION(BlueprintPure, Category="SaveSlotMenu")
	FString GetSelectedSlot() const { return SelectedSlotId; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// Widget tree references
	UPROPERTY(Transient)
	TObjectPtr<class UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<class UScrollBox> ScrollBox;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> BackButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> CreateNewSlotButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> DeleteSlotButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> SaveButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> LoadButton;

private:
	// Menu mode
	ESaveSlotMenuMode MenuMode;

	// Selected slot ID
	FString SelectedSlotId;

	// Button click handlers
	UFUNCTION()
	void OnBackButtonClicked();

	UFUNCTION()
	void OnSlotEntryClicked(const FString& SlotId);

	UFUNCTION()
	void OnCreateNewSlotButtonClicked();

	UFUNCTION()
	void OnDeleteSlotButtonClickedHandler();

	// Update title text based on mode
	void UpdateTitle();

	// Update button visibility based on mode
	void UpdateButtonVisibility();

	// Update delete button enabled state
	void UpdateDeleteButtonState();

	// Update save/load button enabled state
	void UpdateActionButtonState();

	UFUNCTION()
	void OnSaveButtonClickedHandler();

	UFUNCTION()
	void OnLoadButtonClickedHandler();
};

