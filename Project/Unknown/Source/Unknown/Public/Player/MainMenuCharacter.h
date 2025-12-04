// Minimal character for main menu - no movement, just a camera
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "MainMenuCharacter.generated.h"

class UCameraComponent;

UCLASS()
class UNKNOWN_API AMainMenuCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AMainMenuCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	// Camera used for rendering the main menu level
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> MainMenuCamera;
};

