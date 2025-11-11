#include "Player/FirstPersonPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputTriggers.h"
#include "Player/FirstPersonCharacter.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"

AFirstPersonPlayerController::AFirstPersonPlayerController(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	// Create code-defined Enhanced Input assets
	DefaultMappingContext = CreateDefaultSubobject<UInputMappingContext>(TEXT("FP_DefaultMappingContext"));
	MoveAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_MoveAction"));
	LookAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_LookAction"));
	JumpAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_JumpAction"));
	CrouchAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_CrouchAction"));
	SprintAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_SprintAction"));
	InteractAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_InteractAction"));
	RotateHeldAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_RotateHeldAction"));
	ThrowAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_ThrowAction"));
	FlashlightAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_FlashlightAction"));
	
	// Value types
	if (MoveAction) MoveAction->ValueType = EInputActionValueType::Axis2D;
	if (LookAction) LookAction->ValueType = EInputActionValueType::Axis2D;
	if (JumpAction) JumpAction->ValueType = EInputActionValueType::Boolean;
	if (CrouchAction) CrouchAction->ValueType = EInputActionValueType::Boolean;
	if (SprintAction) SprintAction->ValueType = EInputActionValueType::Boolean;
	if (InteractAction) InteractAction->ValueType = EInputActionValueType::Boolean;
	if (RotateHeldAction) RotateHeldAction->ValueType = EInputActionValueType::Boolean;
	if (ThrowAction) ThrowAction->ValueType = EInputActionValueType::Boolean;
	if (FlashlightAction) FlashlightAction->ValueType = EInputActionValueType::Boolean;
	
	// Basic mappings (mouse + simple keys)
	if (DefaultMappingContext && LookAction)
	{
		DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D);
	}
	if (DefaultMappingContext && JumpAction)
	{
		DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	}
	if (DefaultMappingContext && CrouchAction)
	{
		DefaultMappingContext->MapKey(CrouchAction, EKeys::LeftControl);
	}
	if (DefaultMappingContext && SprintAction)
	{
		DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift);
	}
	if (DefaultMappingContext && InteractAction)
	{
		DefaultMappingContext->MapKey(InteractAction, EKeys::E);
	}
	if (DefaultMappingContext && RotateHeldAction)
	{
		DefaultMappingContext->MapKey(RotateHeldAction, EKeys::RightMouseButton);
	}
	if (DefaultMappingContext && ThrowAction)
	{
		DefaultMappingContext->MapKey(ThrowAction, EKeys::LeftMouseButton);
	}
	if (DefaultMappingContext && FlashlightAction)
	{
		DefaultMappingContext->MapKey(FlashlightAction, EKeys::F);
	}
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// Apply our mapping context to the local player enhanced input subsystem
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			Subsys->AddMappingContext(DefaultMappingContext, /*Priority*/0);
		}
	}

	// Configure camera pitch limits (clamp handled by PlayerCameraManager)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -89.f;
		PlayerCameraManager->ViewPitchMax = 89.f;
	}

	// Lock input to game only and hide cursor for FPS
	FInputModeGameOnly Mode;
	SetInputMode(Mode);
	bShowMouseCursor = false;
}

void AFirstPersonPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent))
	{
		if (LookAction)
		{
			EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AFirstPersonPlayerController::OnLook);
		}
		if (JumpAction)
		{
			EIC->BindAction(JumpAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnJumpPressed);
			EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AFirstPersonPlayerController::OnJumpReleased);
		}
		if (CrouchAction)
		{
			EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnCrouchToggle);
		}
		if (SprintAction)
		{
			EIC->BindAction(SprintAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnSprintPressed);
			EIC->BindAction(SprintAction, ETriggerEvent::Completed, this, &AFirstPersonPlayerController::OnSprintReleased);
		}
		if (InteractAction)
		{
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnInteract);
		}
		if (RotateHeldAction)
		{
			EIC->BindAction(RotateHeldAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnRotateHeldPressed);
			EIC->BindAction(RotateHeldAction, ETriggerEvent::Completed, this, &AFirstPersonPlayerController::OnRotateHeldReleased);
		}
		if (ThrowAction)
		{
			EIC->BindAction(ThrowAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnThrow);
		}
		if (FlashlightAction)
		{
			EIC->BindAction(FlashlightAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnFlashlightToggle);
		}
	}
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
	const FVector2D Axis = Value.Get<FVector2D>();

	// If we're holding an object and RMB rotate is active, suppress camera look and only rotate the held object
	bool bDidRotateHeld = false;
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (bRotateHeld && PIC->IsHolding())
			{
				const FVector2D Scaled = Axis * PIC->GetRotationSensitivity();
				PIC->AddRotationInput(Scaled);
				bDidRotateHeld = true;
			}
		}
	}

	if (!bDidRotateHeld)
	{
		// Normal camera look
		AddYawInput(Axis.X);
		AddPitchInput(-Axis.Y); // Invert Y so moving mouse up looks up (UE default is opposite)

		// Forwarding look deltas when not rotating is harmless; component ignores unless rotate mode is active
		if (AFirstPersonCharacter* C2 = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			if (UPhysicsInteractionComponent* PIC2 = C2->FindComponentByClass<UPhysicsInteractionComponent>())
			{
				const FVector2D Scaled2 = Axis * PIC2->GetRotationSensitivity();
				PIC2->AddRotationInput(Scaled2);
			}
		}
	}

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

void AFirstPersonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// WASD movement from raw keys (simple, avoids complex 2D mapping setup)
	FVector2D MoveAxis(0.f, 0.f);
	if (IsInputKeyDown(EKeys::W)) MoveAxis.X += 1.f;
	if (IsInputKeyDown(EKeys::S)) MoveAxis.X -= 1.f;
	if (IsInputKeyDown(EKeys::D)) MoveAxis.Y += 1.f;
	if (IsInputKeyDown(EKeys::A)) MoveAxis.Y -= 1.f;

	if (!MoveAxis.IsNearlyZero())
	{
		if (APawn* P = GetPawn())
		{
			const FRotator YawRot(0.f, GetControlRotation().Yaw, 0.f);
			const FVector Fwd = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
			const FVector Rt = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
			P->AddMovementInput(Fwd, MoveAxis.X);
			P->AddMovementInput(Rt, MoveAxis.Y);
		}
	}

	// Update hold target for physics interaction so held object follows view
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (PIC->IsHolding())
			{
				FVector CamLoc; FRotator CamRot;
				GetPlayerViewPoint(CamLoc, CamRot);
				PIC->UpdateHoldTarget(CamLoc, CamRot.Vector());
			}
		}
	}
}

void AFirstPersonPlayerController::OnInteract(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
   if (PIC->IsHolding())
			{
				UE_LOG(LogTemp, Display, TEXT("[Interact] Releasing currently held object."));
				// Ensure rotate state is cleared when we release
				bRotateHeld = false;
				PIC->SetRotateHeld(false);
				PIC->Release();
				return;
			}

			// Perform a line trace from the camera to try to pick a physics object
			FVector CamLoc; FRotator CamRot;
			GetPlayerViewPoint(CamLoc, CamRot);
			const FVector Dir = CamRot.Vector();
			const float MaxDist = PIC->GetMaxPickupDistance();
			const FVector Start = CamLoc;
			const FVector End = Start + Dir * MaxDist;

			UE_LOG(LogTemp, Display, TEXT("[Interact] Trace Start=%s End=%s MaxDist=%.1f"), *Start.ToString(), *End.ToString(), MaxDist);

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(PhysicsInteract), /*bTraceComplex*/ false);
			Params.AddIgnoredActor(C);

			if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
				{
					AActor* HitActor = Hit.GetActor();
					UPrimitiveComponent* Prim = Hit.GetComponent();
					UE_LOG(LogTemp, Display, TEXT("[Interact] Hit Actor=%s Comp=%s Channel=Interactable Simulating=%s"),
						HitActor ? *HitActor->GetName() : TEXT("<None>"),
						Prim ? *Prim->GetName() : TEXT("<None>"),
						Prim && Prim->IsSimulatingPhysics() ? TEXT("true") : TEXT("false"));
					if (Prim && Prim->IsSimulatingPhysics())
					{
						// Prefer authored socket "HoldPivot"; fallback to Center of Mass
						FVector PivotWorld = Hit.ImpactPoint;
						if (UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Prim))
						{
							static const FName HoldPivotSocket(TEXT("HoldPivot"));
							if (SMC->DoesSocketExist(HoldPivotSocket))
							{
								PivotWorld = SMC->GetSocketLocation(HoldPivotSocket);
							}
						}
						// If still equal to impact point, try Center of Mass
						if (PivotWorld.Equals(Hit.ImpactPoint, KINDA_SMALL_NUMBER))
						{
							PivotWorld = Prim->GetCenterOfMass();
						}
						const float DistanceToPivot = (PivotWorld - Start).Length();
						const bool bBegan = PIC->BeginHold(Prim, PivotWorld, DistanceToPivot);
						UE_LOG(LogTemp, Display, TEXT("[Interact] BeginHold (pivot=%s) %s Dist=%.1f"), *PivotWorld.ToString(), bBegan ? TEXT("true") : TEXT("false"), DistanceToPivot);
					}
					else
					{
						UE_LOG(LogTemp, Display, TEXT("[Interact] Ignored hit because component is not simulating physics."));
					}
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("[Interact] Trace miss."));
				}
		}
	}
}

void AFirstPersonPlayerController::OnRotateHeldPressed(const FInputActionValue& Value)
{
	bRotateHeld = true;
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			PIC->SetRotateHeld(true);
		}
	}
}

void AFirstPersonPlayerController::OnRotateHeldReleased(const FInputActionValue& Value)
{
	bRotateHeld = false;
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			PIC->SetRotateHeld(false);
		}
	}
}

void AFirstPersonPlayerController::OnThrow(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			const FVector Dir = GetControlRotation().Vector();
			PIC->Throw(Dir);
			// Clear rotate state on throw to restore camera look immediately
			bRotateHeld = false;
			PIC->SetRotateHeld(false);
		}
	}
}

void AFirstPersonPlayerController::OnFlashlightToggle(const FInputActionValue& Value)
{
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		C->ToggleFlashlight();
	}
}

float AFirstPersonPlayerController::ClampPitchDegrees(float Degrees)
{
	return FMath::Clamp(Degrees, -89.f, 89.f);
}
