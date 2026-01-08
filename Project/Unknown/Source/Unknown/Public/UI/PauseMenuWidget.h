// Pause menu widget with terminal styling
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "PauseMenuWidget.generated.h"

struct FGeometry;
struct FKeyEvent;

UCLASS()
class UNKNOWN_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPauseMenuWidget(const FObjectInitializer& ObjectInitializer);

	// Show the pause menu
	UFUNCTION(BlueprintCallable, Category="PauseMenu")
	void Show();

	// Hide the pause menu
	UFUNCTION(BlueprintCallable, Category="PauseMenu")
	void Hide();

	// Toggle the pause menu
	UFUNCTION(BlueprintCallable, Category="PauseMenu")
	void Toggle();

	// Check if the menu is visible
	UFUNCTION(BlueprintPure, Category="PauseMenu")
	bool IsPauseMenuVisible() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// Title text
	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TitleText;

	// Button widgets
	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ResumeButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> SaveButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> LoadButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ExitToMainMenuButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ExitToDesktopButton;

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
	void OnResumeClicked();

	UFUNCTION()
	void OnSaveClicked();

	UFUNCTION()
	void OnLoadClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnExitToMainMenuClicked();

	UFUNCTION()
	void OnExitToDesktopClicked();

	// Save system menu handlers
	void CreateSaveSystemMenus();

	UFUNCTION()
	void OnSaveSlotMenuBack();

	UFUNCTION()
	void OnSaveButtonClicked();

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

	// Helper functions
	void ShowMainMenu();
	void ShowSaveSlotMenu();
	void ShowConfirmationMenu(const FString& Message);
	void ShowSaveNameInput(const FString& DefaultName = TEXT(""));

	// Additional handlers
	UFUNCTION()
	void OnCreateNewSlotClicked();

	UFUNCTION()
	void OnDeleteSlotClicked();

	// Pending operation state
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
	FString PendingSlotId;
};

