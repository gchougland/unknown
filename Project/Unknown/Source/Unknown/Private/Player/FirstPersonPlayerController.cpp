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
#include "UI/InteractHighlightWidget.h"
#include "UI/HotbarWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"

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
	PickupAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_PickupAction"));
	SpawnCrowbarAction = CreateDefaultSubobject<UInputAction>(TEXT("FP_SpawnCrowbarAction"));
 	
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
	// Pickup (R) and debug spawn (P)
	if (DefaultMappingContext && PickupAction)
	{
		DefaultMappingContext->MapKey(PickupAction, EKeys::R);
	}
	if (DefaultMappingContext && SpawnCrowbarAction)
	{
		DefaultMappingContext->MapKey(SpawnCrowbarAction, EKeys::P);
	}
}

void AFirstPersonPlayerController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Display, TEXT("[PC] BeginPlay on %s"), *GetName());

	// Ensure our input assets exist even if a Blueprint default wiped C++ subobjects
	{
		auto EnsureAction = [this](TObjectPtr<UInputAction>& Field, const TCHAR* Name, EInputActionValueType Type)
		{
			if (!Field)
			{
				Field = NewObject<UInputAction>(this, Name);
			}
			if (Field)
			{
				Field->ValueType = Type;
			}
		};

		if (!DefaultMappingContext)
		{
			DefaultMappingContext = NewObject<UInputMappingContext>(this, TEXT("FP_DefaultMappingContext_RT"));
		}

		// Ensure actions exist with correct value types
		EnsureAction(MoveAction, TEXT("FP_MoveAction"), EInputActionValueType::Axis2D);
		EnsureAction(LookAction, TEXT("FP_LookAction"), EInputActionValueType::Axis2D);
		EnsureAction(JumpAction, TEXT("FP_JumpAction"), EInputActionValueType::Boolean);
		EnsureAction(CrouchAction, TEXT("FP_CrouchAction"), EInputActionValueType::Boolean);
		EnsureAction(SprintAction, TEXT("FP_SprintAction"), EInputActionValueType::Boolean);
		EnsureAction(InteractAction, TEXT("FP_InteractAction"), EInputActionValueType::Boolean);
		EnsureAction(RotateHeldAction, TEXT("FP_RotateHeldAction"), EInputActionValueType::Boolean);
		EnsureAction(ThrowAction, TEXT("FP_ThrowAction"), EInputActionValueType::Boolean);
		EnsureAction(FlashlightAction, TEXT("FP_FlashlightAction"), EInputActionValueType::Boolean);
		EnsureAction(PickupAction, TEXT("FP_PickupAction"), EInputActionValueType::Boolean);
		EnsureAction(SpawnCrowbarAction, TEXT("FP_SpawnCrowbarAction"), EInputActionValueType::Boolean);

		// Reapply minimal key mappings if the context exists (safe to call repeatedly)
		if (DefaultMappingContext)
		{
			// Axes
			if (LookAction) { DefaultMappingContext->MapKey(LookAction, EKeys::Mouse2D); }
			// Buttons
			if (JumpAction) { DefaultMappingContext->MapKey(JumpAction, EKeys::SpaceBar); }
			if (CrouchAction) { DefaultMappingContext->MapKey(CrouchAction, EKeys::LeftControl); }
			if (SprintAction) { DefaultMappingContext->MapKey(SprintAction, EKeys::LeftShift); }
			if (InteractAction) { DefaultMappingContext->MapKey(InteractAction, EKeys::E); }
			if (RotateHeldAction) { DefaultMappingContext->MapKey(RotateHeldAction, EKeys::RightMouseButton); }
			if (ThrowAction) { DefaultMappingContext->MapKey(ThrowAction, EKeys::LeftMouseButton); }
			if (FlashlightAction) { DefaultMappingContext->MapKey(FlashlightAction, EKeys::F); }
			if (PickupAction) { DefaultMappingContext->MapKey(PickupAction, EKeys::R); }
			if (SpawnCrowbarAction) { DefaultMappingContext->MapKey(SpawnCrowbarAction, EKeys::P); }
		}
	}

	// Apply our mapping context to the local player enhanced input subsystem
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultMappingContext)
			{
				Subsys->AddMappingContext(DefaultMappingContext, /*Priority*/0);
			}
		}
	}

	// Configure camera pitch limits (clamp handled by PlayerCameraManager)
	if (PlayerCameraManager)
	{
		PlayerCameraManager->ViewPitchMin = -89.f;
		PlayerCameraManager->ViewPitchMax = 89.f;
	}

	// Create and add the interact highlight widget (full-screen overlay)
	if (!InteractHighlightWidget)
	{
		InteractHighlightWidget = CreateWidget<UInteractHighlightWidget>(this, UInteractHighlightWidget::StaticClass());
		if (InteractHighlightWidget)
		{
			InteractHighlightWidget->AddToViewport(1000); // high Z-order to render above HUD
			// Fill the entire viewport so our paint coordinates map 1:1 to screen space
			InteractHighlightWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			InteractHighlightWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			// Keep widget always present but non-blocking; use SetVisible() only for the highlight border
			InteractHighlightWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			InteractHighlightWidget->SetVisible(false);
		}
	}

	// Bind to physics interaction state to hide highlight while holding
	if (AFirstPersonCharacter* C0 = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC0 = C0->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			TWeakObjectPtr<AFirstPersonPlayerController> WeakThis(this);
			PIC0->OnPickedUp.AddLambda([WeakThis](UPrimitiveComponent* /*Comp*/, const FVector& /*Pivot*/)
			{
				if (WeakThis.IsValid() && WeakThis->InteractHighlightWidget)
				{
					WeakThis->InteractHighlightWidget->SetVisible(false);
				}
			});
			// On release/throw we don't force-show; next tick will recompute normally
			PIC0->OnReleased.AddLambda([](UPrimitiveComponent* /*Comp*/)
			{
				// no-op
			});
			PIC0->OnThrown.AddLambda([](UPrimitiveComponent* /*Comp*/, const FVector& /*Dir*/)
			{
				// no-op
			});
		}
	}

	// Create and add the hotbar widget (always visible, left side)
	if (!HotbarWidget)
	{
		HotbarWidget = CreateWidget<UHotbarWidget>(this, UHotbarWidget::StaticClass());
		if (HotbarWidget)
		{
   HotbarWidget->SetOwningPlayer(this);
			HotbarWidget->AddToPlayerScreen(500);
   HotbarWidget->SetVisibility(ESlateVisibility::Visible);
   			HotbarWidget->SetRenderOpacity(1.f);
   HotbarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.1f, 0.f, 0.9f));
   HotbarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
   // No padding between the left edge of the screen and the slots
   HotbarWidget->SetPositionInViewport(FVector2D(0.f, 50.f), false);
			HotbarWidget->SetDesiredSizeInViewport(FVector2D(120.f, 9.f * 72.f));

			if (AFirstPersonCharacter* C1 = Cast<AFirstPersonCharacter>(GetPawn()))
			{
				if (UHotbarComponent* HB = C1->GetHotbar())
				{
					HotbarWidget->SetHotbar(HB);
				}
			}
		}
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
		if (PickupAction)
		{
			EIC->BindAction(PickupAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnPickup);
		}
		if (SpawnCrowbarAction)
		{
			EIC->BindAction(SpawnCrowbarAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnSpawnItem);
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

void AFirstPersonPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// WASD movement from raw keys (simple, avoids complex 2D mapping setup)
	const bool bInventoryOpen = (InventoryScreen && InventoryScreen->IsOpen());
	bInventoryUIOpen = bInventoryOpen;
	FVector2D MoveAxis(0.f, 0.f);
	if (!bInventoryOpen)
	{
		if (IsInputKeyDown(EKeys::W)) MoveAxis.X += 1.f;
		if (IsInputKeyDown(EKeys::S)) MoveAxis.X -= 1.f;
		if (IsInputKeyDown(EKeys::D)) MoveAxis.Y += 1.f;
		if (IsInputKeyDown(EKeys::A)) MoveAxis.Y -= 1.f;
	}

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

	// Number keys 1-9 select hotbar slots on just-press
	if (!bInventoryOpen)
	{
		if (AFirstPersonCharacter* CharForHotbar = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			if (WasInputKeyJustPressed(EKeys::One)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 0")); CharForHotbar->SelectHotbarSlot(0); }
			if (WasInputKeyJustPressed(EKeys::Two)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 1")); CharForHotbar->SelectHotbarSlot(1); }
			if (WasInputKeyJustPressed(EKeys::Three)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 2")); CharForHotbar->SelectHotbarSlot(2); }
			if (WasInputKeyJustPressed(EKeys::Four)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 3")); CharForHotbar->SelectHotbarSlot(3); }
			if (WasInputKeyJustPressed(EKeys::Five)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 4")); CharForHotbar->SelectHotbarSlot(4); }
			if (WasInputKeyJustPressed(EKeys::Six)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 5")); CharForHotbar->SelectHotbarSlot(5); }
			if (WasInputKeyJustPressed(EKeys::Seven)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 6")); CharForHotbar->SelectHotbarSlot(6); }
			if (WasInputKeyJustPressed(EKeys::Eight)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 7")); CharForHotbar->SelectHotbarSlot(7); }
			if (WasInputKeyJustPressed(EKeys::Nine)) { UE_LOG(LogTemp, Display, TEXT("[PC] SelectHotbarSlot 8")); CharForHotbar->SelectHotbarSlot(8); }
		}
	}

	// Toggle inventory screen with Tab (primary) or I (fallback)
	if (WasInputKeyJustPressed(EKeys::Tab) || WasInputKeyJustPressed(EKeys::I))
	{
		ToggleInventory();
	}

	// Update hold target for physics interaction so held object follows view
	UPhysicsInteractionComponent* PIC = nullptr;
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		PIC = C->FindComponentByClass<UPhysicsInteractionComponent>();
		if (PIC && PIC->IsHolding())
		{
			FVector CamLoc; FRotator CamRot;
			GetPlayerViewPoint(CamLoc, CamRot);
			PIC->UpdateHoldTarget(CamLoc, CamRot.Vector());
		}
	}

	// Compute screen-space selection rect for current interactable under crosshair
	if (InteractHighlightWidget)
	{
		// Hide highlight entirely while inventory UI is open
		if (bInventoryOpen)
		{
			InteractHighlightWidget->SetVisible(false);
		}
		// Suppress highlight while holding a physics object
		else if (PIC && PIC->IsHolding())
		{
			InteractHighlightWidget->SetVisible(false);
		}
		else
		{
			bool bHasTarget = false;
			FVector2D TL(0, 0), BR(0, 0);

			FVector CamLoc; FRotator CamRot;
			GetPlayerViewPoint(CamLoc, CamRot);
			const FVector Dir = CamRot.Vector();
			const float MaxDist = (PIC ? PIC->GetMaxPickupDistance() : 300.f);
			const FVector Start = CamLoc;
			const FVector End = Start + Dir * MaxDist;

			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(InteractHighlight), /*bTraceComplex*/ false);
			if (APawn* PawnToIgnore = GetPawn()) { Params.AddIgnoredActor(PawnToIgnore); }

			if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
			{
				UPrimitiveComponent* Prim = Hit.GetComponent();
				if (Prim)
				{
					const FBoxSphereBounds& B = Prim->Bounds;
					const FVector Ctr = B.Origin;
					const FVector Ext = B.BoxExtent;
					FVector Corners[8] = {
						Ctr + FVector(-Ext.X, -Ext.Y, -Ext.Z),
						Ctr + FVector( Ext.X, -Ext.Y, -Ext.Z),
						Ctr + FVector(-Ext.X,  Ext.Y, -Ext.Z),
						Ctr + FVector( Ext.X,  Ext.Y, -Ext.Z),
						Ctr + FVector(-Ext.X, -Ext.Y,  Ext.Z),
						Ctr + FVector( Ext.X, -Ext.Y,  Ext.Z),
						Ctr + FVector(-Ext.X,  Ext.Y,  Ext.Z),
						Ctr + FVector( Ext.X,  Ext.Y,  Ext.Z)
					};

					FVector2D MinPt(FLT_MAX, FLT_MAX), MaxPt(-FLT_MAX, -FLT_MAX);
					bool bAnyProjected = false;
					for (int32 i = 0; i < 8; ++i)
					{
						FVector2D ScreenPt;
						// Use UWidgetLayoutLibrary to get DPI-aware, viewport-relative widget coordinates
						if (UWidgetLayoutLibrary::ProjectWorldLocationToWidgetPosition(this, Corners[i], ScreenPt, /*bPlayerViewportRelative*/ true))
						{
							bAnyProjected = true;
							MinPt.X = FMath::Min(MinPt.X, ScreenPt.X);
							MinPt.Y = FMath::Min(MinPt.Y, ScreenPt.Y);
							MaxPt.X = FMath::Max(MaxPt.X, ScreenPt.X);
							MaxPt.Y = FMath::Max(MaxPt.Y, ScreenPt.Y);
						}
					}

					if (bAnyProjected)
					{
						// Small padding
						const float Pad = 6.f;
						TL = MinPt - FVector2D(Pad, Pad);
						BR = MaxPt + FVector2D(Pad, Pad);
						bHasTarget = true;
					}
				}
			}

			if (bHasTarget)
			{
				InteractHighlightWidget->SetHighlightRect(TL, BR);
				InteractHighlightWidget->SetVisible(true);
			}
			else
			{
				InteractHighlightWidget->SetVisible(false);
			}
		}
	}
}

void AFirstPersonPlayerController::OnInteract(const FInputActionValue& Value)
{
	// Block interaction while inventory UI is open
	if (bInventoryUIOpen)
	{
		return;
	}
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
	if (bInventoryUIOpen)
	{
		return;
	}
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
	if (bInventoryUIOpen)
	{
		return;
	}
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
	if (bInventoryUIOpen)
	{
		return;
	}
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
	// Lazy-create inventory widget
	if (!InventoryScreen)
	{
		InventoryScreen = CreateWidget<UInventoryScreenWidget>(this, UInventoryScreenWidget::StaticClass());
		if (InventoryScreen)
		{
			InventoryScreen->AddToViewport(900);
			InventoryScreen->SetAnchorsInViewport(FAnchors(0.2f, 0.2f, 0.8f, 0.8f));
			InventoryScreen->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			// Close the inventory when the widget requests it (Tab/I/Esc)
			InventoryScreen->OnRequestClose.AddDynamic(this, &AFirstPersonPlayerController::ToggleInventory);
			UE_LOG(LogTemp, Display, TEXT("[PC] Inventory widget created and added to viewport"));
		}
	}

	if (!InventoryScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] ToggleInventory: InventoryScreen is null (failed to create)"));
		return;
	}

	const bool bCurrentlyOpen = InventoryScreen->IsOpen();
	if (bCurrentlyOpen)
	{
		bInventoryUIOpen = false;
		InventoryScreen->Close();
  // Restore crosshair/highlight when closing inventory
  if (InteractHighlightWidget)
  {
      InteractHighlightWidget->SetCrosshairEnabled(true);
      InteractHighlightWidget->SetVisible(false); // will auto-show next tick if a target exists
  }
		FInputModeGameOnly GM;
		SetInputMode(GM);
		SetIgnoreLookInput(false);
		SetIgnoreMoveInput(false);
		bShowMouseCursor = false;
		UE_LOG(LogTemp, Display, TEXT("[PC] Inventory closed; returning to GameOnly input"));
	}
	else
	{
		UInventoryComponent* Inv = nullptr;
		if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			Inv = Char->GetInventory();
		}
		InventoryScreen->Open(Inv, nullptr);
		bInventoryUIOpen = true;
  // Hide crosshair/highlight behind the inventory
  if (InteractHighlightWidget)
  {
      InteractHighlightWidget->SetCrosshairEnabled(false);
      InteractHighlightWidget->SetVisible(false);
  }
		// Switch to UI-only input and ignore game look/move
		FInputModeUIOnly Mode;
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		SetInputMode(Mode);
		SetIgnoreLookInput(true);
		SetIgnoreMoveInput(true);
		bShowMouseCursor = true;
		UE_LOG(LogTemp, Display, TEXT("[PC] Inventory opened; using UIOnly input"));
	}
}

void AFirstPersonPlayerController::OnPickup(const FInputActionValue& Value)
{
	if (bInventoryUIOpen)
	{
		return;
	}
	AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn());
	if (!C)
	{
		return;
	}
	UInventoryComponent* Inv = C->GetInventory();
	if (!Inv)
	{
		return;
	}
	// Trace for an item pickup in front of the camera
	FVector CamLoc; FRotator CamRot;
	GetPlayerViewPoint(CamLoc, CamRot);
	const FVector Start = CamLoc;
	const FVector End = Start + CamRot.Vector() * 300.f;
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
	Params.AddIgnoredActor(C);
	bool bHitAnything = false;
	if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
	{
		bHitAnything = true;
		if (AItemPickup* Pickup = Cast<AItemPickup>(Hit.GetActor()))
		{
			UItemDefinition* Def = Pickup->GetItemDef();
			if (Def)
			{
				FItemEntry Entry; Entry.Def = Def; Entry.ItemId = FGuid::NewGuid();
				if (Inv->TryAdd(Entry))
				{
					UE_LOG(LogTemp, Display, TEXT("[Pickup] Added %s to inventory"), *Def->GetName());
					Pickup->Destroy();
					// If inventory UI is open, refresh it so the new item appears immediately
					if (bInventoryUIOpen && InventoryScreen)
					{
					  InventoryScreen->RefreshInventoryView();
					}
				}
				else
				{
					UE_LOG(LogTemp, Display, TEXT("[Pickup] Inventory full or invalid item; could not add %s"), *Def->GetName());
				}
			}
		}
	}
	// If we did not hit any interactable and the character is holding an item, release it
	if (!bHitAnything)
	{
		if (C->IsHoldingItem())
		{
			UE_LOG(LogTemp, Display, TEXT("[Pickup] No interactable under crosshair; releasing held item on R"));
			C->ReleaseHeldItem();
		}
	}
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
