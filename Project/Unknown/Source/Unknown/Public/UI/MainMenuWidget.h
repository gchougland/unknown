// Main menu widget with terminal styling
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"

UCLASS()
class UNKNOWN_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMainMenuWidget(const FObjectInitializer& ObjectInitializer);
	
	// Helper to re-apply position and anchors (call after visibility changes or level transitions)
	void ReapplyPositionAndAnchors();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// Button widgets
	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> LoadGameButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ExitButton;

	// Save system menu widgets
	UPROPERTY(Transient)
	TObjectPtr<class USaveSlotMenuWidget> SaveSlotMenu;

	UPROPERTY(Transient)
	TObjectPtr<class UConfirmationMenuWidget> ConfirmationMenu;

	UPROPERTY(Transient)
	TObjectPtr<class USaveNameInputWidget> SaveNameInput;

private:
	// Button click handlers
	UFUNCTION()
	void OnNewGameClicked();

	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnLoadGameClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnExitClicked();

	// Save system menu handlers
	void CreateSaveSystemMenus();

	UFUNCTION()
	void OnSaveSlotMenuBack();

	UFUNCTION()
	void OnSaveSlotSelected(const FString& SlotId);

	UFUNCTION()
	void OnCreateNewSlotClicked();

	UFUNCTION()
	void OnDeleteSlotClicked();

	UFUNCTION()
	void OnLoadButtonClicked();

	UFUNCTION()
	void OnSaveNameEntered(const FString& SaveName);

	UFUNCTION()
	void OnSaveNameCancelled();

	UFUNCTION()
	void OnConfirmationConfirmed();

	UFUNCTION()
	void OnConfirmationCancelled();

	// Pending slot ID for confirmation
	FString PendingSlotId;
	
	// Pending operation (same enum as pause menu)
	enum class EPendingOperation
	{
		None,
		Save,
		Load,
		Delete,
		Overwrite,
		NewGame,
		ExitToMainMenu,
		ExitToDesktop
	};
	
	EPendingOperation PendingOperation;
};

