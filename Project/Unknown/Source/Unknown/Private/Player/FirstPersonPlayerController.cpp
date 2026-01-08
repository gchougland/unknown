#include "Player/FirstPersonPlayerController.h"

#include "Player/FirstPersonCharacter.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/HotbarComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "UI/HotbarWidget.h"
#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"

// Include helper module implementations (must be before first use)
#include "FirstPersonPlayerControllerInputSetup.cpp"
#include "FirstPersonPlayerControllerUI.cpp"
#include "FirstPersonPlayerControllerInteraction.cpp"
#include "FirstPersonPlayerControllerTick.cpp"
#include "UI/PauseMenuWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AFirstPersonPlayerController::AFirstPersonPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// Delegate input asset creation to helper
	FPlayerControllerInputSetup::InitializeInputAssets(this);
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Display, TEXT("[PC] BeginPlay on %s"), *GetName());

	// Ensure input assets exist and are configured
	FPlayerControllerInputSetup::EnsureInputAssets(this);
	
	// Apply mapping context to enhanced input subsystem
	FPlayerControllerInputSetup::ApplyMappingContext(this);

	// Configure camera pitch limits (clamp handled by PlayerCameraManager)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -89.f;
		PlayerCameraManager->ViewPitchMax = 89.f;
	}

	// Initialize all widgets
	FPlayerControllerUIManager::InitializeWidgets(this);

	// Lock input to game only and hide cursor for FPS
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
	
	// Check for pending new game save (if we just loaded MainMap from main menu)
	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GameInstance = World->GetGameInstance())
		{
			if (USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>())
			{
				// Check after a short delay to ensure level is fully loaded
				FTimerHandle TimerHandle;
				World->GetTimerManager().SetTimer(TimerHandle, [SaveSystem]()
				{
					SaveSystem->CheckAndCreatePendingNewGameSave();
				}, 0.5f, false);
			}
		}
	}
	
	// Check for position restore after level load
	// This happens when loading a save that required a level transition
	RestorePlayerPositionAfterLevelLoad();
}

void AFirstPersonPlayerController::RestorePlayerPositionAfterLevelLoad()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Check if there's a temp save with position data
	FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
	if (UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(TempSlotName, 0)))
	{
		// Wait a bit for the player to fully spawn
		FTimerHandle RestoreTimer;
		World->GetTimerManager().SetTimer(RestoreTimer, [this, World, TempSlotName]()
		{
			// Load the temporary save to restore position
			if (UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(TempSlotName, 0)))
			{
				if (APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(World, 0))
				{
					if (AFirstPersonCharacter* PlayerCharacter = Cast<AFirstPersonCharacter>(PlayerPawn))
					{
						PlayerCharacter->SetActorLocation(TempSave->PlayerLocation);
						PlayerCharacter->SetActorRotation(TempSave->PlayerRotation);
						UE_LOG(LogTemp, Display, TEXT("[SaveSystem] Restored player position after level load"));
						
						// Clean up temp save
						UGameplayStatics::DeleteGameInSlot(TempSlotName, 0);
					}
				}
			}
		}, 0.2f, false); // 0.2 second delay to ensure player is fully spawned
	}
}

void AFirstPersonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Delegate input binding setup to helper
	FPlayerControllerInputSetup::SetupInputBindings(this);
}

void AFirstPersonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Delegate tick subsystems to helpers
	FPlayerControllerTickHandler::TickMovement(this, DeltaTime);
	FPlayerControllerTickHandler::TickHoldToDrop(this, DeltaTime);
	FPlayerControllerTickHandler::TickHoldToUse(this, DeltaTime);
	FPlayerControllerTickHandler::TickPhysicsInteraction(this);
	FPlayerControllerTickHandler::TickInteractHighlight(this);
	FPlayerControllerTickHandler::TickHungerBar(this);
}

void AFirstPersonPlayerController::OnMove(const FInputActionValue& Value)
{
	// Not used when mapping from keys directly; we process movement in PlayerTick for WASD
	const FVector2D Axis = Value.Get<FVector2D>();
	if (APawn* P = GetPawn())
	{
		const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
		const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
		const FVector Rt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
		P->AddMovementInput(Fwd, Axis.X);
		P->AddMovementInput(Rt, Axis.Y);
	}
}

void AFirstPersonPlayerController::OnLook(const FInputActionValue& Value)
{
	if (bInventoryUIOpen)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();

	// When right mouse is held and we are holding a physics object, forward look deltas to rotate the held object
	if (bRotateHeld)
	{
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				if (PIC->IsHolding())
				{
					PIC->AddRotationInput(Axis);
					return; // do not move camera while rotating the held object
				}
			}
		}
	}

	// Normal camera look
	AddYawInput(Axis.X);
	AddPitchInput(-Axis.Y);

	// Pitch clamping is handled by PlayerCameraManager's ViewPitchMin/Max
}

void AFirstPersonPlayerController::OnJumpPressed(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->Jump();
	}
}

void AFirstPersonPlayerController::OnJumpReleased(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StopJumping();
	}
}

void AFirstPersonPlayerController::OnCrouchToggle(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->ToggleCrouch();
	}
}

void AFirstPersonPlayerController::OnSprintPressed(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StartSprint();
	}
}

void AFirstPersonPlayerController::OnSprintReleased(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->StopSprint();
	}
}

void AFirstPersonPlayerController::OnInteractPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractPressed(this, Value);
}

void AFirstPersonPlayerController::OnInteractOngoing(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractOngoing(this, Value);
}

void AFirstPersonPlayerController::OnInteractReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnInteractReleased(this, Value);
}

// Legacy handler - kept for compatibility but should not be called
void AFirstPersonPlayerController::OnInteract(const FInputActionValue& Value)
{
	// This should not be called anymore, but redirect to pressed handler for safety
	OnInteractPressed(Value);
}

void AFirstPersonPlayerController::OnRotateHeldPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnRotateHeldPressed(this, Value);
}

void AFirstPersonPlayerController::OnRotateHeldReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnRotateHeldReleased(this, Value);
}

void AFirstPersonPlayerController::OnThrow(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnThrow(this, Value);
}

void AFirstPersonPlayerController::OnFlashlightToggle(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->ToggleFlashlight();
	}
}

void AFirstPersonPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	UE_LOG(LogTemp, Display, TEXT("[PC] OnPossess Pawn=%s"), InPawn ? *InPawn->GetName() : TEXT("<null>"));

	// If we already created the hotbar widget, bind it now that we have a pawn
	if (HotbarWidget)
	{
		if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(InPawn))
		{
			if (UHotbarComponent* HB = C->GetHotbar())
			{
				HotbarWidget->SetHotbar(HB);
			}
		}
	}
}

void AFirstPersonPlayerController::ToggleInventory()
{
	FPlayerControllerUIManager::ToggleInventory(this);
}

void AFirstPersonPlayerController::OpenInventoryWithStorage(UStorageComponent* Storage)
{
	FPlayerControllerUIManager::OpenInventoryWithStorage(this, Storage);
}

UStorageWindowWidget* AFirstPersonPlayerController::GetStorageWindow()
{
	return FPlayerControllerUIManager::GetStorageWindow(this);
}

void AFirstPersonPlayerController::OnPickupPressed(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupPressed(this, Value);
}

void AFirstPersonPlayerController::OnPickupOngoing(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupOngoing(this, Value);
}

void AFirstPersonPlayerController::OnPickupReleased(const FInputActionValue& Value)
{
	FPlayerControllerInteractionHandler::OnPickupReleased(this, Value);
}

// Legacy handler - kept for compatibility but should not be called
void AFirstPersonPlayerController::OnPickup(const FInputActionValue& Value)
{
	// This should not be called anymore, but redirect to pressed handler for safety
	OnPickupPressed(Value);
}

FString AFirstPersonPlayerController::GetKeyDisplayName(UInputAction* Action) const
{
	if (!Action || !DefaultMappingContext)
	{
		return FString();
	}

	// Query the mapping context for mappings to this action
	const TArray<FEnhancedActionKeyMapping>& Mappings = DefaultMappingContext->GetMappings();
	for (const auto& Mapping : Mappings)
	{
		if (Mapping.Action == Action)
		{
			// Get the first key from the mapping
			if (Mapping.Key.IsValid())
			{
				return Mapping.Key.GetDisplayName().ToString();
			}
		}
	}

	return FString();
}

void AFirstPersonPlayerController::OnSpawnItem(const FInputActionValue& Value)
{
#if !UE_BUILD_SHIPPING
	if (bInventoryUIOpen)
	{
		return;
	}
	// Resolve the item definition from the soft reference (Blueprint-configurable)
	UItemDefinition* Def = DebugSpawnItem.LoadSynchronous();
	if (!Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SpawnItem] DebugSpawnItem is not set or failed to load"));
		return;
	}
	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	FActorSpawnParameters Params; Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	const FVector BaseLoc = CamLoc + CamRot.Vector() * 200.f;
	const FRotator BaseRot = FRotator(0.f, CamRot.Yaw, 0.f);
	FTransform BaseTransform(BaseRot, BaseLoc, FVector(1.f));
	FTransform FinalTransform = Def ? Def->DefaultDropTransform * BaseTransform : BaseTransform;
	if (UWorld* World = GetWorld())
	{
		AItemPickup* Pickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FinalTransform, Params);
		if (Pickup)
		{
			Pickup->SetItemDef(Def);
			UE_LOG(LogTemp, Display, TEXT("[SpawnItem] Spawned pickup %s at %s"), *GetNameSafe(Def), *FinalTransform.GetLocation().ToString());
		}
	}
#endif
}

float AFirstPersonPlayerController::ClampPitchDegrees(float Degrees)
{
	return FMath::Clamp(Degrees, -89.f, 89.f);
}

void AFirstPersonPlayerController::OnPausePressed(const FInputActionValue& Value)
{
	// Don't pause if inventory is open
	if (bInventoryUIOpen)
	{
		return;
	}

	if (!PauseMenuWidget)
	{
		return;
	}

	// Use actual game pause state as source of truth
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}
	
	bool bGameIsPaused = World->IsPaused();

	// Check if we just unpaused to prevent buffered input from immediately re-pausing
	// This check applies to both pausing and unpausing
	if (bIsUnpausing)
	{
		// Ignore this input - it's likely buffered from the unpause
		return;
	}

	if (bGameIsPaused)
	{
		// Game is paused - Enhanced Input might not fire reliably when paused,
		// but if it does, forward to the widget which handles Escape key directly
		// The widget's NativeOnKeyDown will handle unpausing
		// For now, just call Hide() - the widget should handle keyboard focus
		PauseMenuWidget->Hide();
		UE_LOG(LogTemp, Display, TEXT("[PC] Pause menu closed via Enhanced Input"));
	}
	else
	{
		// Game is not paused - user wants to pause
		
		// Show pause menu
		PauseMenuWidget->Show();
		
		// Set input mode to UI only
		FInputModeUIOnly InputMode;
		InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(InputMode);
		bShowMouseCursor = true;
		
		// Pause the game
		SetPause(true);
		bIsPaused = true;
		
		UE_LOG(LogTemp, Display, TEXT("[PC] Pause menu opened"));
	}
}
