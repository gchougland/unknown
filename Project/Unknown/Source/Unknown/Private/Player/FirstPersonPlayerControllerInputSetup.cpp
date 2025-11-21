#include "Player/FirstPersonPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"

// Helper struct for input setup logic
struct FPlayerControllerInputSetup
{
	// Initialize input assets in constructor
	static void InitializeInputAssets(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		// Create code-defined Enhanced Input assets
		PC->DefaultMappingContext = PC->CreateDefaultSubobject<UInputMappingContext>(TEXT("FP_DefaultMappingContext"));
		PC->MoveAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_MoveAction"));
		PC->LookAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_LookAction"));
		PC->JumpAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_JumpAction"));
		PC->CrouchAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_CrouchAction"));
		PC->SprintAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_SprintAction"));
		PC->InteractAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_InteractAction"));
		PC->RotateHeldAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_RotateHeldAction"));
		PC->ThrowAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_ThrowAction"));
		PC->FlashlightAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_FlashlightAction"));
		PC->PickupAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_PickupAction"));
		PC->SpawnCrowbarAction = PC->CreateDefaultSubobject<UInputAction>(TEXT("FP_SpawnCrowbarAction"));
		
		// Value types
		if (PC->MoveAction) PC->MoveAction->ValueType = EInputActionValueType::Axis2D;
		if (PC->LookAction) PC->LookAction->ValueType = EInputActionValueType::Axis2D;
		if (PC->JumpAction) PC->JumpAction->ValueType = EInputActionValueType::Boolean;
		if (PC->CrouchAction) PC->CrouchAction->ValueType = EInputActionValueType::Boolean;
		if (PC->SprintAction) PC->SprintAction->ValueType = EInputActionValueType::Boolean;
		if (PC->InteractAction) PC->InteractAction->ValueType = EInputActionValueType::Boolean;
		if (PC->RotateHeldAction) PC->RotateHeldAction->ValueType = EInputActionValueType::Boolean;
		if (PC->ThrowAction) PC->ThrowAction->ValueType = EInputActionValueType::Boolean;
		if (PC->FlashlightAction) PC->FlashlightAction->ValueType = EInputActionValueType::Boolean;
		
		// Basic mappings (mouse + simple keys)
		if (PC->DefaultMappingContext && PC->LookAction)
		{
			PC->DefaultMappingContext->MapKey(PC->LookAction, EKeys::Mouse2D);
		}
		if (PC->DefaultMappingContext && PC->JumpAction)
		{
			PC->DefaultMappingContext->MapKey(PC->JumpAction, EKeys::SpaceBar);
		}
		if (PC->DefaultMappingContext && PC->CrouchAction)
		{
			PC->DefaultMappingContext->MapKey(PC->CrouchAction, EKeys::LeftControl);
		}
		if (PC->DefaultMappingContext && PC->SprintAction)
		{
			PC->DefaultMappingContext->MapKey(PC->SprintAction, EKeys::LeftShift);
		}
		if (PC->DefaultMappingContext && PC->InteractAction)
		{
			PC->DefaultMappingContext->MapKey(PC->InteractAction, EKeys::E);
		}
		if (PC->DefaultMappingContext && PC->RotateHeldAction)
		{
			PC->DefaultMappingContext->MapKey(PC->RotateHeldAction, EKeys::RightMouseButton);
		}
		if (PC->DefaultMappingContext && PC->ThrowAction)
		{
			PC->DefaultMappingContext->MapKey(PC->ThrowAction, EKeys::LeftMouseButton);
		}
		if (PC->DefaultMappingContext && PC->FlashlightAction)
		{
			PC->DefaultMappingContext->MapKey(PC->FlashlightAction, EKeys::F);
		}
		// Pickup (R) and debug spawn (P)
		if (PC->DefaultMappingContext && PC->PickupAction)
		{
			PC->DefaultMappingContext->MapKey(PC->PickupAction, EKeys::R);
		}
		if (PC->DefaultMappingContext && PC->SpawnCrowbarAction)
		{
			PC->DefaultMappingContext->MapKey(PC->SpawnCrowbarAction, EKeys::P);
		}
	}

	// Ensure input assets exist and are properly configured (called from BeginPlay)
	static void EnsureInputAssets(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		auto EnsureAction = [PC](TObjectPtr<UInputAction>& Field, const TCHAR* Name, EInputActionValueType Type)
		{
			if (!Field)
			{
				Field = NewObject<UInputAction>(PC, Name);
			}
			if (Field)
			{
				Field->ValueType = Type;
			}
		};

		if (!PC->DefaultMappingContext)
		{
			PC->DefaultMappingContext = NewObject<UInputMappingContext>(PC, TEXT("FP_DefaultMappingContext_RT"));
		}

		// Ensure actions exist with correct value types
		EnsureAction(PC->MoveAction, TEXT("FP_MoveAction"), EInputActionValueType::Axis2D);
		EnsureAction(PC->LookAction, TEXT("FP_LookAction"), EInputActionValueType::Axis2D);
		EnsureAction(PC->JumpAction, TEXT("FP_JumpAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->CrouchAction, TEXT("FP_CrouchAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->SprintAction, TEXT("FP_SprintAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->InteractAction, TEXT("FP_InteractAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->RotateHeldAction, TEXT("FP_RotateHeldAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->ThrowAction, TEXT("FP_ThrowAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->FlashlightAction, TEXT("FP_FlashlightAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->PickupAction, TEXT("FP_PickupAction"), EInputActionValueType::Boolean);
		EnsureAction(PC->SpawnCrowbarAction, TEXT("FP_SpawnCrowbarAction"), EInputActionValueType::Boolean);

		// Reapply minimal key mappings if the context exists (safe to call repeatedly)
		if (PC->DefaultMappingContext)
		{
			// Axes
			if (PC->LookAction) { PC->DefaultMappingContext->MapKey(PC->LookAction, EKeys::Mouse2D); }
			// Buttons
			if (PC->JumpAction) { PC->DefaultMappingContext->MapKey(PC->JumpAction, EKeys::SpaceBar); }
			if (PC->CrouchAction) { PC->DefaultMappingContext->MapKey(PC->CrouchAction, EKeys::LeftControl); }
			if (PC->SprintAction) { PC->DefaultMappingContext->MapKey(PC->SprintAction, EKeys::LeftShift); }
			if (PC->InteractAction) { PC->DefaultMappingContext->MapKey(PC->InteractAction, EKeys::E); }
			if (PC->RotateHeldAction) { PC->DefaultMappingContext->MapKey(PC->RotateHeldAction, EKeys::RightMouseButton); }
			if (PC->ThrowAction) { PC->DefaultMappingContext->MapKey(PC->ThrowAction, EKeys::LeftMouseButton); }
			if (PC->FlashlightAction) { PC->DefaultMappingContext->MapKey(PC->FlashlightAction, EKeys::F); }
			if (PC->PickupAction) { PC->DefaultMappingContext->MapKey(PC->PickupAction, EKeys::R); }
			if (PC->SpawnCrowbarAction) { PC->DefaultMappingContext->MapKey(PC->SpawnCrowbarAction, EKeys::P); }
		}
	}

	// Apply mapping context to enhanced input subsystem
	static void ApplyMappingContext(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		// Apply our mapping context to the local player enhanced input subsystem
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (PC->DefaultMappingContext)
				{
					Subsys->AddMappingContext(PC->DefaultMappingContext, /*Priority*/0);
				}
			}
		}
	}

	// Setup input bindings (called from SetupInputComponent)
	static void SetupInputBindings(AFirstPersonPlayerController* PC)
	{
		if (!PC)
		{
			return;
		}

		if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
		{
			if (PC->LookAction)
			{
				EIC->BindAction(PC->LookAction, ETriggerEvent::Triggered, PC, &AFirstPersonPlayerController::OnLook);
			}
			if (PC->JumpAction)
			{
				EIC->BindAction(PC->JumpAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnJumpPressed);
				EIC->BindAction(PC->JumpAction, ETriggerEvent::Completed, PC, &AFirstPersonPlayerController::OnJumpReleased);
			}
			if (PC->CrouchAction)
			{
				EIC->BindAction(PC->CrouchAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnCrouchToggle);
			}
			if (PC->SprintAction)
			{
				EIC->BindAction(PC->SprintAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnSprintPressed);
				EIC->BindAction(PC->SprintAction, ETriggerEvent::Completed, PC, &AFirstPersonPlayerController::OnSprintReleased);
			}
			if (PC->InteractAction)
			{
				EIC->BindAction(PC->InteractAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnInteractPressed);
				EIC->BindAction(PC->InteractAction, ETriggerEvent::Triggered, PC, &AFirstPersonPlayerController::OnInteractOngoing);
				EIC->BindAction(PC->InteractAction, ETriggerEvent::Canceled, PC, &AFirstPersonPlayerController::OnInteractReleased);
				EIC->BindAction(PC->InteractAction, ETriggerEvent::Completed, PC, &AFirstPersonPlayerController::OnInteractReleased);
			}
			if (PC->RotateHeldAction)
			{
				EIC->BindAction(PC->RotateHeldAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnRotateHeldPressed);
				EIC->BindAction(PC->RotateHeldAction, ETriggerEvent::Completed, PC, &AFirstPersonPlayerController::OnRotateHeldReleased);
			}
			if (PC->ThrowAction)
			{
				EIC->BindAction(PC->ThrowAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnThrow);
			}
			if (PC->FlashlightAction)
			{
				EIC->BindAction(PC->FlashlightAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnFlashlightToggle);
			}
			if (PC->PickupAction)
			{
				EIC->BindAction(PC->PickupAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnPickupPressed);
				EIC->BindAction(PC->PickupAction, ETriggerEvent::Triggered, PC, &AFirstPersonPlayerController::OnPickupOngoing);
				EIC->BindAction(PC->PickupAction, ETriggerEvent::Canceled, PC, &AFirstPersonPlayerController::OnPickupReleased);
				EIC->BindAction(PC->PickupAction, ETriggerEvent::Completed, PC, &AFirstPersonPlayerController::OnPickupReleased);
			}
			if (PC->SpawnCrowbarAction)
			{
				EIC->BindAction(PC->SpawnCrowbarAction, ETriggerEvent::Started, PC, &AFirstPersonPlayerController::OnSpawnItem);
			}
		}
	}
};

