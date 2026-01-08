// Save name input widget for entering save names
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SaveNameInputWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSaveNameEntered, const FString&, SaveName);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnSaveNameCancelled);

UCLASS()
class UNKNOWN_API USaveNameInputWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	USaveNameInputWidget(const FObjectInitializer& ObjectInitializer);

	// Set default name for the text box
	UFUNCTION(BlueprintCallable, Category="SaveNameInput")
	void SetDefaultName(const FString& DefaultName);

	// Delegate for name entry
	UPROPERTY(BlueprintAssignable, Category="SaveNameInput")
	FOnSaveNameEntered OnNameEntered;

	// Delegate for cancellation
	UPROPERTY(BlueprintAssignable, Category="SaveNameInput")
	FOnSaveNameCancelled OnCancelled;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// Widget tree references
	UPROPERTY(Transient)
	TObjectPtr<class UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> TitleText;

	UPROPERTY(Transient)
	TObjectPtr<class UEditableTextBox> NameTextBox;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> ConfirmButton;

	UPROPERTY(Transient)
	TObjectPtr<class UMainMenuButtonWidget> CancelButton;

private:
	// Button click handlers
	UFUNCTION()
	void OnConfirmClicked();

	UFUNCTION()
	void OnCancelClicked();

	// Validate the entered name
	bool ValidateName(const FString& Name) const;
};

