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

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// Button widgets
	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> NewGameButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ContinueButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> SettingsButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ExitButton;

private:
	// Button click handlers
	UFUNCTION()
	void OnNewGameClicked();

	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnSettingsClicked();

	UFUNCTION()
	void OnExitClicked();
};

