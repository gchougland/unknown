#include "Computers/DimensionalScannerComputer.h"
#include "Components/PhysicsObjectSocketComponent.h"
#include "Dimensions/ScannerStageController.h"
#include "Dimensions/DimensionalSignal.h"
#include "Dimensions/DimensionCartridgeHelpers.h"
#include "Dimensions/DimensionScanningSubsystem.h"
#include "Dimensions/DimensionCartridgeData.h"
#include "Inventory/ItemPickup.h"
#include "UI/MessageLogSubsystem.h"
#include "Player/FirstPersonPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "TimerManager.h"

ADimensionalScannerComputer::ADimensionalScannerComputer(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bIsScanning(false)
{
	PrimaryActorTick.bCanEverTick = true;

	// Create cartridge socket
	CartridgeSocket = CreateDefaultSubobject<UPhysicsObjectSocketComponent>(TEXT("CartridgeSocket"));
	CartridgeSocket->SetupAttachment(RootComponent);
}

void ADimensionalScannerComputer::BeginPlay()
{
	Super::BeginPlay();

	PrimaryActorTick.bCanEverTick = true;

	// Create input actions and mapping context
	ComputerMappingContext = NewObject<UInputMappingContext>(this, TEXT("ComputerMappingContext"));

	// Create input actions
	ScannerMoveAction = NewObject<UInputAction>(this, TEXT("ScannerMoveAction"));
	ScannerMoveAction->ValueType = EInputActionValueType::Axis2D;

	ScannerScrollAction = NewObject<UInputAction>(this, TEXT("ScannerScrollAction"));
	ScannerScrollAction->ValueType = EInputActionValueType::Axis1D;

	ScannerScanAction = NewObject<UInputAction>(this, TEXT("ScannerScanAction"));
	ScannerScanAction->ValueType = EInputActionValueType::Boolean;

	ComputerExitAction = NewObject<UInputAction>(this, TEXT("ComputerExitAction"));
	ComputerExitAction->ValueType = EInputActionValueType::Boolean;

	// Map keys - for WASD, we'll handle it manually in tick similar to movement system
	// For now, just map the other keys
	if (ComputerMappingContext)
	{
		ComputerMappingContext->MapKey(ScannerScrollAction, EKeys::MouseWheelAxis);
		ComputerMappingContext->MapKey(ScannerScanAction, EKeys::SpaceBar);
		ComputerMappingContext->MapKey(ComputerExitAction, EKeys::Escape);
	}
}


void ADimensionalScannerComputer::EnterComputerMode(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	Super::EnterComputerMode(PC);
}

void ADimensionalScannerComputer::ExitComputerMode(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	Super::ExitComputerMode(PC);
}

void ADimensionalScannerComputer::SetupComputerControls(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	// Set computer mode flag
	if (AFirstPersonPlayerController* FPPC = Cast<AFirstPersonPlayerController>(PC))
	{
		FPPC->bInComputerMode = true;
		DefaultMappingContext = FPPC->GetDefaultMappingContext();
	}

	// Remove default mapping context and add computer mapping context
	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// Remove default context
			if (DefaultMappingContext)
			{
				Subsys->RemoveMappingContext(DefaultMappingContext);
			}

			// Add computer context with higher priority
			if (ComputerMappingContext)
			{
				Subsys->AddMappingContext(ComputerMappingContext, 10);
			}
		}
	}

	// Bind input actions to PlayerController's InputComponent
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PC->InputComponent))
	{
		// ScannerMoveAction handled in Tick instead
		if (ScannerScrollAction)
		{
			EIC->BindAction(ScannerScrollAction, ETriggerEvent::Triggered, this, &ADimensionalScannerComputer::OnScannerScroll);
		}
		if (ScannerScanAction)
		{
			EIC->BindAction(ScannerScanAction, ETriggerEvent::Started, this, &ADimensionalScannerComputer::OnScannerScan);
		}
		if (ComputerExitAction)
		{
			EIC->BindAction(ComputerExitAction, ETriggerEvent::Started, this, &ADimensionalScannerComputer::OnComputerExit);
		}
	}
}

void ADimensionalScannerComputer::RestorePlayerControls(APlayerController* PC)
{
	if (!PC)
	{
		return;
	}

	// Clear computer mode flag
	if (AFirstPersonPlayerController* FPPC = Cast<AFirstPersonPlayerController>(PC))
	{
		FPPC->bInComputerMode = false;
	}

	// Remove computer mapping context and restore default
	// Note: Input bindings are automatically cleaned up when mapping context is removed
	if (ULocalPlayer* LP = PC->GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			// Remove computer context
			if (ComputerMappingContext)
			{
				Subsys->RemoveMappingContext(ComputerMappingContext);
			}

			// Restore default context
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, 0);
			}
		}
	}
}

void ADimensionalScannerComputer::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Handle WASD movement manually (similar to player movement system)
	if (CurrentPlayerController.IsValid() && ScannerStageController)
	{
		FVector2D MoveAxis(0.0f, 0.0f);
		
		if (CurrentPlayerController->IsInputKeyDown(EKeys::W)) MoveAxis.X += 1.0f;
		if (CurrentPlayerController->IsInputKeyDown(EKeys::S)) MoveAxis.X -= 1.0f;
		if (CurrentPlayerController->IsInputKeyDown(EKeys::D)) MoveAxis.Y += 1.0f;
		if (CurrentPlayerController->IsInputKeyDown(EKeys::A)) MoveAxis.Y -= 1.0f;

		if (!MoveAxis.IsNearlyZero())
		{
			// Normalize and apply delta time
			MoveAxis.Normalize();
			HandleScannerMove(MoveAxis * DeltaTime);
		}
	}
}

void ADimensionalScannerComputer::OnScannerMove(const FInputActionValue& Value)
{
	// Not used - handled in Tick instead
}

void ADimensionalScannerComputer::OnScannerScroll(const FInputActionValue& Value)
{
	const float ScrollDelta = Value.Get<float>();
	HandleScannerScroll(ScrollDelta);
}

void ADimensionalScannerComputer::OnScannerScan(const FInputActionValue& Value)
{
	// Only process if the value is true (key pressed, not released)
	if (Value.Get<bool>())
	{
		PerformScan();
	}
}

void ADimensionalScannerComputer::OnComputerExit(const FInputActionValue& Value)
{
	if (CurrentPlayerController.IsValid())
	{
		ExitComputerMode(CurrentPlayerController.Get());
	}
}

void ADimensionalScannerComputer::HandleScannerMove(const FVector2D& Input)
{
	if (!ScannerStageController)
	{
		return;
	}

	// Convert 2D input to 3D movement
	// W/S = Up/Down (Z-axis), A/D = Left/Right (Y-axis)
	// Input.X is W/S, Input.Y is A/D
	// Input is already scaled by delta time from Tick
	// W = +Z (up), S = -Z (down), A = -Y (left), D = +Y (right)
	// FVector is (X, Y, Z) where X=forward, Y=left/right, Z=up/down
	// W/S should map to Z-axis, A/D should map to Y-axis
	FVector DeltaMovement(0.0f, Input.X * CursorMoveSpeed, -Input.Y * CursorMoveSpeed);

	ScannerStageController->MoveCursor(DeltaMovement);

	// Update Niagara parameter
	FVector LocalPos = ScannerStageController->GetCursorLocalPosition();
	UpdateNiagaraParameter(LocalPos.X);
}

void ADimensionalScannerComputer::HandleScannerScroll(float ScrollDelta)
{
	if (!ScannerStageController)
	{
		return;
	}

	// Scroll wheel moves forward/backward (X-axis)
	// ScrollDelta is already per-frame, so we multiply by sensitivity
	FVector DeltaMovement(ScrollDelta * ScrollSensitivity, 0.0f, 0.0f);

	ScannerStageController->MoveCursor(DeltaMovement);

	// Update Niagara parameter
	FVector LocalPos = ScannerStageController->GetCursorLocalPosition();
	UpdateNiagaraParameter(LocalPos.X);
}

void ADimensionalScannerComputer::PerformScan()
{
	// Prevent double-scanning
	if (bIsScanning)
	{
		return;
	}
	bIsScanning = true;

	// Check if cartridge is inserted
	if (!CheckCartridgeInserted())
	{
		if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
		{
			MsgLog->PushMessage(FText::FromString(TEXT("No cartridge inserted")));
		}
		bIsScanning = false;
		return;
	}

	// Check if cartridge is empty (check BEFORE scanning, not after)
	if (!IsCartridgeEmpty())
	{
		if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
		{
			MsgLog->PushMessage(FText::FromString(TEXT("Cartridge already contains a dimension")));
		}
		bIsScanning = false;
		return;
	}

	// Play scan Niagara effect at cursor position
	if (ScanNiagaraEffect && ScannerStageController)
	{
		FVector CursorWorldPos = ScannerStageController->GetCursorWorldPosition();
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), ScanNiagaraEffect, CursorWorldPos, FRotator::ZeroRotator);
	}

	// Get cursor world position
	FVector CursorWorldPos = ScannerStageController->GetCursorWorldPosition();

	// Find nearby signals
	TArray<ADimensionalSignal*> NearbySignals = ScannerStageController->GetSignalsNearPosition(CursorWorldPos, ScanDetectionRange);

	if (NearbySignals.Num() > 0)
	{
		// Get dimension from scanning subsystem
		if (UWorld* World = GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				if (UDimensionScanningSubsystem* ScanningSubsystem = GameInstance->GetSubsystem<UDimensionScanningSubsystem>())
				{
					UDimensionDefinition* DimensionDef = ScanningSubsystem->ScanForDimension();
					if (DimensionDef)
					{
						// Get socketed item
						AItemPickup* Cartridge = CartridgeSocket->GetSocketedItem();
						if (Cartridge)
						{
							// Get item entry
							FItemEntry ItemEntry = Cartridge->GetItemEntry();

							// Create new cartridge data with dimension
							FVector SpawnPos = Cartridge->GetActorLocation();
							UDimensionCartridgeData* NewCartridgeData = UDimensionCartridgeHelpers::CreateCartridgeFromDimension(DimensionDef, SpawnPos);

							if (NewCartridgeData)
							{
								// Update item entry
								UDimensionCartridgeHelpers::SetCartridgeDataInItemEntry(ItemEntry, NewCartridgeData);

								// Update the socketed item
								Cartridge->SetItemEntry(ItemEntry);

								// Set material parameter to indicate cartridge is no longer empty
								if (UStaticMeshComponent* CartridgeMesh = Cartridge->Mesh)
								{
									const int32 MaterialElementIndex = 2;
									if (UMaterialInterface* BaseMaterial = CartridgeMesh->GetMaterial(MaterialElementIndex))
									{
										// Get or create dynamic material instance
										UMaterialInstanceDynamic* DynamicMaterial = CartridgeMesh->CreateDynamicMaterialInstance(MaterialElementIndex, BaseMaterial);
										if (DynamicMaterial)
										{
											// Set IsEmpty parameter to 0 (not empty)
											DynamicMaterial->SetScalarParameterValue(FName("IsEmpty"), 0.0f);
										}
									}
								}
							}
						}
					}
				}
			}
		}

		// Scan the first unscanned signal found
		if (NearbySignals.Num() > 0)
		{
			// Find the first signal that hasn't been scanned yet
			for (ADimensionalSignal* Signal : NearbySignals)
			{
				if (Signal && IsValid(Signal) && !Signal->HasBeenScanned())
				{
					Signal->OnScanned();
					break; // Only scan one signal per scan attempt
				}
			}
		}
	}

	// Clear scanning flag after a small delay to prevent rapid re-triggering
	// Use a timer to clear the flag instead of clearing immediately
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ScanCooldownTimerHandle,
			this,
			&ADimensionalScannerComputer::ClearScanFlag,
			0.1f, // 100ms cooldown
			false
		);
	}
	else
	{
		// Fallback if world is invalid
		bIsScanning = false;
	}
}

bool ADimensionalScannerComputer::CheckCartridgeInserted() const
{
	if (!CartridgeSocket)
	{
		return false;
	}

	return CartridgeSocket->GetSocketedItem() != nullptr;
}

bool ADimensionalScannerComputer::IsCartridgeEmpty() const
{
	if (!CartridgeSocket)
	{
		return false;
	}

	AItemPickup* Cartridge = CartridgeSocket->GetSocketedItem();
	if (!Cartridge)
	{
		return false;
	}

	UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(Cartridge);
	if (!CartridgeData)
	{
		// No cartridge data means it's empty
		return true;
	}

	// Check if dimension definition is null (empty cartridge)
	return CartridgeData->GetDimensionDefinition() == nullptr;
}

void ADimensionalScannerComputer::UpdateNiagaraParameter(float LocalX)
{
	if (ScannerStageController)
	{
		ScannerStageController->SetNiagaraCaptureX(LocalX);
	}
}

void ADimensionalScannerComputer::ClearScanFlag()
{
	bIsScanning = false;
}
