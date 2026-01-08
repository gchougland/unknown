// Confirmation menu widget for destructive actions
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ConfirmationMenuWidget.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmationConfirmed);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnConfirmationCancelled);

UCLASS()
class UNKNOWN_API UConfirmationMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UConfirmationMenuWidget(const FObjectInitializer& ObjectInitializer);

	// Set the confirmation message
	UFUNCTION(BlueprintCallable, Category="ConfirmationMenu")
	void SetMessage(const FString& Message);

	// Delegate for confirmation
	UPROPERTY(BlueprintAssignable, Category="ConfirmationMenu")
	FOnConfirmationConfirmed OnConfirmed;

	// Delegate for cancellation
	UPROPERTY(BlueprintAssignable, Category="ConfirmationMenu")
	FOnConfirmationCancelled OnCancelled;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	// Widget tree references
	UPROPERTY(Transient)
	TObjectPtr<class UBorder> RootBorder;

	UPROPERTY(Transient)
	TObjectPtr<class UTextBlock> MessageText;

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
};

