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
#include "UI/InteractInfoWidget.h"
#include "UI/HotbarWidget.h"
#include "UI/DropProgressBarWidget.h"
#include "UI/StatBarWidget.h"
#include "UI/InventoryScreenWidget.h"
#include "Player/HungerComponent.h"
#include "UI/StorageWindowWidget.h"
#include "UI/StorageListWidget.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/StorageComponent.h"
#include "Inventory/ItemDefinition.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemUseAction.h"
#include "Inventory/ItemAttackAction.h"
#include "Inventory/StorageSerialization.h"
#include "Interfaces/IAttackable.h"
#include "UI/MessageLogSubsystem.h"

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

	// Create and add the interact info widget (full-screen overlay)
	if (!InteractInfoWidget)
	{
		InteractInfoWidget = CreateWidget<UInteractInfoWidget>(this, UInteractInfoWidget::StaticClass());
		if (InteractInfoWidget)
		{
			InteractInfoWidget->AddToViewport(1001); // slightly higher Z-order than highlight
			// Fill the entire viewport so our paint coordinates map 1:1 to screen space
			InteractInfoWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			InteractInfoWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			InteractInfoWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			InteractInfoWidget->SetVisible(false);
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

	// Create and add the drop progress bar widget (full-screen overlay)
	if (!DropProgressBarWidget)
	{
		DropProgressBarWidget = CreateWidget<UDropProgressBarWidget>(this, UDropProgressBarWidget::StaticClass());
		if (DropProgressBarWidget)
		{
			DropProgressBarWidget->AddToViewport(1001); // higher Z-order than highlight widget
			DropProgressBarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 1.f, 1.f));
			DropProgressBarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			DropProgressBarWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
			DropProgressBarWidget->SetVisible(false);
		}
	}

	// Create and add the hunger bar widget (below hotbar, left side)
	if (!HungerBarWidget)
	{
		HungerBarWidget = CreateWidget<UStatBarWidget>(this, UStatBarWidget::StaticClass());
		if (HungerBarWidget)
		{
			HungerBarWidget->SetOwningPlayer(this);
			HungerBarWidget->AddToPlayerScreen(500);
			HungerBarWidget->SetVisibility(ESlateVisibility::Visible);
			HungerBarWidget->SetRenderOpacity(1.f);
			HungerBarWidget->SetAnchorsInViewport(FAnchors(0.f, 0.f, 0.f, 0.f));
			HungerBarWidget->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			// Position below hotbar: hotbar is at Y=50 with height 648, so hunger bar at Y=50+648+20=718
			HungerBarWidget->SetPositionInViewport(FVector2D(0.f, 718.f), false);
			HungerBarWidget->SetDesiredSizeInViewport(FVector2D(120.f, 24.f)); // Width matches hotbar, height for taller bar
			
			// Configure hunger bar appearance
			HungerBarWidget->SetLabel(TEXT("HGR"));
			HungerBarWidget->SetFillColor(FLinearColor(0.8f, 0.2f, 0.2f, 1.0f)); // Red
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
			EIC->BindAction(InteractAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnInteractPressed);
			EIC->BindAction(InteractAction, ETriggerEvent::Triggered, this, &AFirstPersonPlayerController::OnInteractOngoing);
			EIC->BindAction(InteractAction, ETriggerEvent::Canceled, this, &AFirstPersonPlayerController::OnInteractReleased);
			EIC->BindAction(InteractAction, ETriggerEvent::Completed, this, &AFirstPersonPlayerController::OnInteractReleased);
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
			EIC->BindAction(PickupAction, ETriggerEvent::Started, this, &AFirstPersonPlayerController::OnPickupPressed);
			EIC->BindAction(PickupAction, ETriggerEvent::Triggered, this, &AFirstPersonPlayerController::OnPickupOngoing);
			EIC->BindAction(PickupAction, ETriggerEvent::Canceled, this, &AFirstPersonPlayerController::OnPickupReleased);
			EIC->BindAction(PickupAction, ETriggerEvent::Completed, this, &AFirstPersonPlayerController::OnPickupReleased);
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

	// Update hold-to-drop/hold-to-pickup progress bar
	if (bIsHoldingDrop && !bInstantActionExecuted && !bInventoryUIOpen)
	{
		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn());
		if (!C)
		{
			bIsHoldingDrop = false;
			HoldDropTimer = 0.0f;
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}
			return;
		}

		// Trace to see what player is looking at
		FVector CamLoc; FRotator CamRot;
		GetPlayerViewPoint(CamLoc, CamRot);
		const FVector Start = CamLoc;
		const FVector End = Start + CamRot.Vector() * 300.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
		Params.AddIgnoredActor(C);
		
		bool bLookingAtPickup = false;
		AItemPickup* TargetPickup = nullptr;
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
		{
			TargetPickup = Cast<AItemPickup>(Hit.GetActor());
			bLookingAtPickup = (TargetPickup != nullptr);
		}

		// Update timer
		HoldDropTimer += DeltaTime;
		
		// Only show progress bar after tap threshold (0.25s) has passed
		const float TapThreshold = 0.25f;
		if (HoldDropTimer >= TapThreshold)
		{
			if (DropProgressBarWidget && !DropProgressBarWidget->GetIsVisible())
			{
				DropProgressBarWidget->SetVisible(true);
			}
			
			// Calculate progress from tap threshold onwards (0.25s to 0.75s = 0.5s range)
			const float EffectiveHoldTime = HoldDropTimer - TapThreshold;
			const float EffectiveHoldDuration = HoldDropDuration - TapThreshold;
			const float Progress = FMath::Clamp(EffectiveHoldTime / EffectiveHoldDuration, 0.0f, 1.0f);
			
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetProgress(Progress);
			}
		}

		// Check if we've held long enough (total time including tap threshold)
		if (HoldDropTimer >= HoldDropDuration)
		{
			if (bLookingAtPickup && TargetPickup)
			{
				// Hold the pickup item
				FItemEntry PickupEntry = TargetPickup->GetItemEntry();
				if (PickupEntry.IsValid())
				{
					// Generate a new GUID if the pickup has an invalid one (all zeros)
					if (!PickupEntry.ItemId.IsValid())
					{
						PickupEntry.ItemId = FGuid::NewGuid();
					}
					
					// If already holding an item, store it first
					if (C->IsHoldingItem())
					{
						C->PutHeldItemBack();
					}
					
					// Check if item can be stored
					bool bCanBeStored = PickupEntry.Def && PickupEntry.Def->bCanBeStored;
					
					UInventoryComponent* Inv = C->GetInventory();
					
					// If this is a storage container, save its storage data before holding
					if (UStorageComponent* StorageComp = TargetPickup->FindComponentByClass<UStorageComponent>())
					{
						StorageSerialization::SaveStorageToItemEntry(StorageComp, PickupEntry);
					}
					
					// If item cannot be stored, hold it directly without adding to inventory
					if (!bCanBeStored)
					{
						// Hold directly from pickup (no inventory needed)
						if (C->HoldItem(PickupEntry))
						{
							UE_LOG(LogTemp, Display, TEXT("[Pickup] Holding %s directly (cannot be stored)"), *GetNameSafe(PickupEntry.Def));
							TargetPickup->Destroy();
							bInstantActionExecuted = true;
						}
					}
					// If item can be stored, add to inventory first, then hold it
					else if (Inv && Inv->TryAdd(PickupEntry))
					{
						// Now hold it from inventory
						if (C->HoldItem(PickupEntry))
						{
							UE_LOG(LogTemp, Display, TEXT("[Pickup] Holding %s after progress"), *GetNameSafe(PickupEntry.Def));
							TargetPickup->Destroy();
							bInstantActionExecuted = true;
						}
						else
						{
							// Failed to hold, remove from inventory
							Inv->RemoveById(PickupEntry.ItemId);
						}
					}
				}
			}
			else if (C->IsHoldingItem())
			{
				// Drop held item at location
				C->DropHeldItemAtLocation();
				bInstantActionExecuted = true;
			}
			
			// Reset state
			bIsHoldingDrop = false;
			HoldDropTimer = 0.0f;
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}
		}
	}

	// Update hold-to-use progress bar
	if (bIsHoldingUse && !bInstantUseExecuted && !bInventoryUIOpen)
	{
		AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn());
		if (!C)
		{
			bIsHoldingUse = false;
			HoldUseTimer = 0.0f;
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}
			return;
		}

		// Trace to see what player is looking at
		FVector CamLoc; FRotator CamRot;
		GetPlayerViewPoint(CamLoc, CamRot);
		const FVector Start = CamLoc;
		const FVector End = Start + CamRot.Vector() * 300.f;
		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
		Params.AddIgnoredActor(C);
		
		bool bLookingAtPickup = false;
		AItemPickup* TargetPickup = nullptr;
		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
		{
			TargetPickup = Cast<AItemPickup>(Hit.GetActor());
			bLookingAtPickup = (TargetPickup != nullptr);
		}

		// Update timer
		HoldUseTimer += DeltaTime;
		
		// Only show progress bar after tap threshold (0.25s) has passed
		const float TapThreshold = 0.25f;
		if (HoldUseTimer >= TapThreshold)
		{
			if (DropProgressBarWidget && !DropProgressBarWidget->GetIsVisible())
			{
				DropProgressBarWidget->SetVisible(true);
			}
			
			// Calculate progress from tap threshold onwards (0.25s to 0.75s = 0.5s range)
			const float EffectiveHoldTime = HoldUseTimer - TapThreshold;
			const float EffectiveHoldDuration = HoldDropDuration - TapThreshold;
			const float Progress = FMath::Clamp(EffectiveHoldTime / EffectiveHoldDuration, 0.0f, 1.0f);
			
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetProgress(Progress);
			}
		}

		// Check if we've held long enough (total time including tap threshold)
		if (HoldUseTimer >= HoldDropDuration)
		{
			if (bLookingAtPickup && TargetPickup)
			{
				// Try to use the pickup item
				FItemEntry PickupEntry = TargetPickup->GetItemEntry();
				if (PickupEntry.IsValid() && PickupEntry.Def)
				{
					// Check if item has a USE action
					if (PickupEntry.Def->DefaultUseAction)
					{
						// Create instance of use action and execute it
						UItemUseAction* UseAction = NewObject<UItemUseAction>(this, PickupEntry.Def->DefaultUseAction);
						if (UseAction)
						{
							// Pass the pickup actor so the use action can update/replace it in the world
							if (UseAction->Execute(C, PickupEntry, TargetPickup))
							{
								UE_LOG(LogTemp, Display, TEXT("[Interact] Used %s"), *GetNameSafe(PickupEntry.Def));
								// Use action succeeded - item may have been consumed or modified
								// Don't destroy pickup here - let the use action handle it if needed
								bInstantUseExecuted = true;
							}
							else
							{
								UE_LOG(LogTemp, Display, TEXT("[Interact] Use action failed for %s"), *GetNameSafe(PickupEntry.Def));
							}
						}
					}
					else
					{
						// No USE action - show "Cannot use" message
						if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
						{
							MsgLog->PushMessage(FText::FromString(TEXT("Cannot use")));
						}
						UE_LOG(LogTemp, Display, TEXT("[Interact] Item %s has no USE action"), *GetNameSafe(PickupEntry.Def));
					}
				}
			}
			else
			{
				// Not looking at a pickup - nothing to use
				if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
				{
					MsgLog->PushMessage(FText::FromString(TEXT("Cannot use")));
				}
			}
			
			// Reset state
			bIsHoldingUse = false;
			bInstantUseExecuted = true; // Prevent OnInteractReleased from doing anything
			HoldUseTimer = 0.0f;
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}
		}
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
			if (InteractInfoWidget)
			{
				InteractInfoWidget->SetVisible(false);
			}
		}
		// Suppress highlight while holding a physics object
		else if (PIC && PIC->IsHolding())
		{
			InteractHighlightWidget->SetVisible(false);
			if (InteractInfoWidget)
			{
				InteractInfoWidget->SetVisible(false);
			}
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

				// Update interact info widget if we're looking at an item pickup
				if (InteractInfoWidget)
				{
					AActor* HitActor = Hit.GetActor();
					AItemPickup* ItemPickup = Cast<AItemPickup>(HitActor);
					
					if (ItemPickup && ItemPickup->GetItemDef())
					{
						// Set the item data
						InteractInfoWidget->SetInteractableItem(ItemPickup);
						
						// Get key names and set them
						FString InteractKeyName = GetKeyDisplayName(InteractAction);
						FString PickupKeyName = GetKeyDisplayName(PickupAction);
						InteractInfoWidget->SetKeybindings(InteractKeyName, PickupKeyName);
						
						// Set position and visibility
						InteractInfoWidget->SetPosition(TL, BR);
						InteractInfoWidget->SetVisible(true);
					}
					else
					{
						InteractInfoWidget->SetVisible(false);
					}
				}
			}
			else
			{
				InteractHighlightWidget->SetVisible(false);
				if (InteractInfoWidget)
				{
					InteractInfoWidget->SetVisible(false);
				}
			}
		}
	}

	// Update hunger bar widget
	if (HungerBarWidget)
	{
		if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(GetPawn()))
		{
			if (UHungerComponent* HungerComp = Char->GetHunger())
			{
				const float CurrentHunger = HungerComp->GetCurrentHunger();
				const float MaxHunger = HungerComp->GetMaxHunger();
				HungerBarWidget->SetValue(CurrentHunger, MaxHunger);
			}
		}
	}
}

void AFirstPersonPlayerController::OnInteractPressed(const FInputActionValue& Value)
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

	// Reset state
	bInstantUseExecuted = false;
	bIsHoldingUse = false;
	HoldUseTimer = 0.0f;

	// Check if holding physics object - release it immediately on tap
	if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
	{
		if (PIC->IsHolding())
		{
			// This is a tap - release immediately
			UE_LOG(LogTemp, Display, TEXT("[Interact] Releasing currently held object."));
			bRotateHeld = false;
			PIC->SetRotateHeld(false);
			PIC->Release();
			return;
		}
	}

	// Start hold timer for use action
	bIsHoldingUse = true;
	// Don't show progress bar yet - wait for tap threshold to pass
}

void AFirstPersonPlayerController::OnInteractOngoing(const FInputActionValue& Value)
{
	// This is called every frame while E is held
	// Actual timer update happens in PlayerTick
}

void AFirstPersonPlayerController::OnInteractReleased(const FInputActionValue& Value)
{
	if (bInstantUseExecuted)
	{
		// Reset flag for next press
		bInstantUseExecuted = false;
		return;
	}

	// Check if this was a tap (very short press) vs a hold
	const float TapThreshold = 0.25f;
	const bool bWasTap = (HoldUseTimer < TapThreshold);

	if (bIsHoldingUse)
	{
		if (bWasTap)
		{
			// This was a tap - execute tap behavior (physics interaction)
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}

			AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn());
			if (C && !bInventoryUIOpen)
			{
				if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
				{
					// Perform a line trace from the camera to try to pick a physics object
					FVector CamLoc; FRotator CamRot;
					GetPlayerViewPoint(CamLoc, CamRot);
					const FVector Dir = CamRot.Vector();
					const float MaxDist = PIC->GetMaxPickupDistance();
					const FVector Start = CamLoc;
					const FVector End = Start + Dir * MaxDist;

					FHitResult Hit;
					FCollisionQueryParams Params(SCENE_QUERY_STAT(PhysicsInteract), false);
					Params.AddIgnoredActor(C);

					if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
					{
						UPrimitiveComponent* Prim = Hit.GetComponent();
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
							PIC->BeginHold(Prim, PivotWorld, DistanceToPivot);
						}
					}
				}
			}
		}
		// If it was a hold that was canceled, just cancel without doing anything

		// Reset state
		bIsHoldingUse = false;
		HoldUseTimer = 0.0f;
		if (DropProgressBarWidget)
		{
			DropProgressBarWidget->SetVisible(false);
		}
	}
}

// Legacy handler - kept for compatibility but should not be called
void AFirstPersonPlayerController::OnInteract(const FInputActionValue& Value)
{
	// This should not be called anymore, but redirect to pressed handler for safety
	OnInteractPressed(Value);
}

void AFirstPersonPlayerController::OnRotateHeldPressed(const FInputActionValue& Value)
{
	if (bInventoryUIOpen)
	{
		return;
	}
	
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		// Priority 1: Check if physics object is being held -> rotate it
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (PIC->IsHolding())
			{
				bRotateHeld = true;
				PIC->SetRotateHeld(true);
				return;
			}
		}
		
		// Priority 2: Check if item is being held -> use it
		if (C->IsHoldingItem())
		{
			FItemEntry HeldEntry = C->GetHeldItemEntry();
			if (HeldEntry.IsValid() && HeldEntry.Def && HeldEntry.Def->DefaultUseAction)
			{
				// Create use action instance and execute it
				UItemUseAction* UseAction = NewObject<UItemUseAction>(this, HeldEntry.Def->DefaultUseAction);
				if (UseAction)
				{
					// Pass nullptr for WorldPickup since this is a held item (not a world pickup)
					if (UseAction->Execute(C, HeldEntry, nullptr))
					{
						UE_LOG(LogTemp, Display, TEXT("[UseHeldItem] Used %s"), *GetNameSafe(HeldEntry.Def));
						
						// Update the held item entry and actor with the modified data
						// (Execute modifies HeldEntry by reference, so we need to sync it back)
						C->UpdateHeldItemEntry(HeldEntry);
						
						// Refresh inventory UI if open
						if (InventoryScreen && InventoryScreen->IsOpen())
						{
							InventoryScreen->RefreshInventoryView();
						}
					}
					else
					{
						UE_LOG(LogTemp, Display, TEXT("[UseHeldItem] Use action failed for %s"), *GetNameSafe(HeldEntry.Def));
					}
				}
			}
		}
	}
}

void AFirstPersonPlayerController::OnRotateHeldReleased(const FInputActionValue& Value)
{
	if (bInventoryUIOpen)
	{
		return;
	}
	
	// Only disable rotation if physics object is being held
	bRotateHeld = false;
	if (AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
		{
			if (PIC->IsHolding())
			{
				PIC->SetRotateHeld(false);
			}
		}
	}
}

void AFirstPersonPlayerController::OnThrow(const FInputActionValue& Value)
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

	// PRIORITY 1: Check if physics object is being held - if yes, throw it and return early
	if (UPhysicsInteractionComponent* PIC = C->FindComponentByClass<UPhysicsInteractionComponent>())
	{
		if (PIC->IsHolding())
		{
			const FVector Dir = GetControlRotation().Vector();
			PIC->Throw(Dir);
			// Clear rotate state on throw to restore camera look immediately
			bRotateHeld = false;
			PIC->SetRotateHeld(false);
			return; // Early return - physics throw takes priority
		}
	}

	// Check if Shift is held and player is holding an item - if so, try to store in container
	const bool bShiftHeld = IsInputKeyDown(EKeys::LeftShift);
	const bool bHoldingItem = C->IsHoldingItem();
	
	if (bShiftHeld && bHoldingItem)
	{
		// Perform line trace to find interactables with storage components
		FVector CamLoc; FRotator CamRot;
		GetPlayerViewPoint(CamLoc, CamRot);
		const FVector Dir = CamRot.Vector();
		const float MaxDist = 300.f; // Match interactable detection range
		const FVector Start = CamLoc;
		const FVector End = Start + Dir * MaxDist;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(StoreItem), false);
		Params.AddIgnoredActor(C);
		// Note: Held item actor is attached to character and has collision disabled, so it's naturally ignored

		if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
		{
			AActor* HitActor = Hit.GetActor();
			if (HitActor)
			{
				// Check if the hit actor has a storage component
				UStorageComponent* StorageComp = HitActor->FindComponentByClass<UStorageComponent>();
				if (StorageComp)
				{
					// Get the held item entry
					FItemEntry HeldEntry = C->GetHeldItemEntry();
					if (HeldEntry.IsValid() && HeldEntry.Def)
					{
						// Check if item can be stored
						if (!HeldEntry.Def->bCanBeStored)
						{
							// Item cannot be stored
							if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
							{
								MsgLog->PushMessage(FText::FromString(FString::Printf(TEXT("%s cannot be stored"), *HeldEntry.Def->DisplayName.ToString())));
							}
							UE_LOG(LogTemp, Display, TEXT("[StoreItem] Item %s cannot be stored (bCanBeStored is false)"), *GetNameSafe(HeldEntry.Def));
							return; // Don't throw, just return
						}

						// Attempt to store the item
						if (StorageComp->TryAdd(HeldEntry))
						{
							// Success - remove item from held state
							UE_LOG(LogTemp, Display, TEXT("[StoreItem] Successfully stored %s in container"), *GetNameSafe(HeldEntry.Def));
							C->ReleaseHeldItem(false); // Don't try to put back, it's already stored
							
							// Refresh UI if inventory is open
							if (InventoryScreen && InventoryScreen->IsOpen())
							{
								InventoryScreen->RefreshInventoryView();
							}
							return; // Success - don't throw
						}
						else
						{
							// Storage is full or failed
							if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
							{
								MsgLog->PushMessage(FText::FromString(TEXT("Storage full")));
							}
							UE_LOG(LogTemp, Display, TEXT("[StoreItem] Failed to store %s - storage full or invalid"), *GetNameSafe(HeldEntry.Def));
							return; // Don't throw, just return
						}
					}
				}
			}
		}
		// If we get here, Shift was held but no valid storage was found - fall through to attack/throw behavior
	}

	// PRIORITY 2: Check if holding an item with attack action
	if (bHoldingItem)
	{
		FItemEntry HeldEntry = C->GetHeldItemEntry();
		if (HeldEntry.IsValid() && HeldEntry.Def && HeldEntry.Def->DefaultAttackAction)
		{
			// Get or create attack action instance (cached to maintain cooldown state)
			UItemAttackAction* AttackAction = CachedAttackAction;
			
			// Check if we need to create a new instance or if the cached one is for a different item
			if (!AttackAction || !AttackAction->GetClass()->IsChildOf(HeldEntry.Def->DefaultAttackAction))
			{
				AttackAction = NewObject<UItemAttackAction>(this, HeldEntry.Def->DefaultAttackAction);
				CachedAttackAction = AttackAction;
			}

			if (AttackAction)
			{
				// Check if attack is on cooldown
				if (AttackAction->IsOnCooldown())
				{
					UE_LOG(LogTemp, Verbose, TEXT("[Attack] Attack action is on cooldown, ignoring input"));
					return; // Attack on cooldown, do nothing
				}

				// Perform line trace to find attackable targets
				FVector CamLoc; FRotator CamRot;
				GetPlayerViewPoint(CamLoc, CamRot);
				const FVector Dir = CamRot.Vector();
				const float AttackRange = AttackAction->GetAttackRange();
				const FVector Start = CamLoc;
				const FVector End = Start + Dir * AttackRange;

				FHitResult Hit;
				FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemAttack), false);
				Params.AddIgnoredActor(C);
				// Ignore held item actor
				if (AItemPickup* HeldActor = C->GetHeldItemActor())
				{
					Params.AddIgnoredActor(HeldActor);
				}

				if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
				{
					AActor* HitActor = Hit.GetActor();
					if (HitActor)
					{
						// Check if hit actor implements IAttackable
						IAttackable* AttackableTarget = Cast<IAttackable>(HitActor);
						if (AttackableTarget)
						{
							// Calculate hit location and direction
							const FVector HitLocation = Hit.ImpactPoint;
							const FVector HitDirection = (HitLocation - Start).GetSafeNormal();

							// Execute attack action
							if (AttackAction->ExecuteAttack(C, HeldEntry, HitActor, HitLocation, HitDirection))
							{
								UE_LOG(LogTemp, Display, TEXT("[Attack] Successfully attacked %s with %s"), 
									*GetNameSafe(HitActor), *GetNameSafe(HeldEntry.Def));
								return; // Attack successful, don't fall through to throw
							}
							else
							{
								UE_LOG(LogTemp, Verbose, TEXT("[Attack] Attack action failed for %s"), *GetNameSafe(HeldEntry.Def));
							}
						}
						else
						{
							UE_LOG(LogTemp, Verbose, TEXT("[Attack] Hit actor %s does not implement IAttackable"), *GetNameSafe(HitActor));
						}
					}
				}
				else
				{
					UE_LOG(LogTemp, Verbose, TEXT("[Attack] No attackable target found in range"));
				}
			}
		}
	}

	// FALLBACK: If no physics object and no attack action, do nothing
	// (Original throw behavior for physics objects is already handled above)
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
	else
	{
		InventoryScreen->SetAnchorsInViewport(FAnchors(0.2f, 0.2f, 0.8f, 0.8f));
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
		
		// Close storage window if it's open
		if (StorageWindow && StorageWindow->IsOpen())
		{
			StorageWindow->Close();
			StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
		}
		
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

void AFirstPersonPlayerController::OpenInventoryWithStorage(UStorageComponent* StorageComp)
{
	// Lazy-create inventory widget if needed
	if (!InventoryScreen)
	{
		InventoryScreen = CreateWidget<UInventoryScreenWidget>(this, UInventoryScreenWidget::StaticClass());
		if (InventoryScreen)
		{
			// Position inventory screen to the right (60% of screen, starting at 40%)
			InventoryScreen->AddToViewport(900);
			InventoryScreen->SetAnchorsInViewport(FAnchors(0.4f, 0.2f, 0.9f, 0.8f));
			InventoryScreen->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			// Close the inventory when the widget requests it (Tab/I/Esc)
			InventoryScreen->OnRequestClose.AddDynamic(this, &AFirstPersonPlayerController::ToggleInventory);
			UE_LOG(LogTemp, Display, TEXT("[PC] Inventory widget created and added to viewport"));
		}
	}
	else
	{
		InventoryScreen->SetAnchorsInViewport(FAnchors(0.4f, 0.2f, 0.9f, 0.8f));
	}

	if (!InventoryScreen)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PC] OpenInventoryWithStorage: InventoryScreen is null (failed to create)"));
		return;
	}

	// Get player inventory
	UInventoryComponent* PlayerInv = nullptr;
	if (AFirstPersonCharacter* Char = Cast<AFirstPersonCharacter>(GetPawn()))
	{
		PlayerInv = Char->GetInventory();
	}

	// Open or update inventory screen
	const bool bCurrentlyOpen = InventoryScreen->IsOpen();
	if (!bCurrentlyOpen)
	{
		// Open inventory with storage reference (so left-click transfer works)
		InventoryScreen->Open(PlayerInv, StorageComp);
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
		UE_LOG(LogTemp, Display, TEXT("[PC] Inventory opened with storage; using UIOnly input"));
	}
	else
	{
		// Already open - just refresh
		InventoryScreen->Refresh();
		UE_LOG(LogTemp, Display, TEXT("[PC] Updated open inventory with storage"));
	}

	// Show and open storage window separately (to the left of inventory)
	if (StorageComp)
	{
		UStorageWindowWidget* StorageWin = GetStorageWindow();
		if (StorageWin)
		{
			// Position storage window to the left (40% of screen, starting at 5%)
			StorageWin->SetAnchorsInViewport(FAnchors(0.05f, 0.2f, 0.4f, 0.8f));
			StorageWin->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			
			// Get ItemDefinition from the storage component's owner (should be an AItemPickup)
			UItemDefinition* ContainerDef = nullptr;
			if (AActor* StorageOwner = StorageComp->GetOwner())
			{
				if (AItemPickup* Pickup = Cast<AItemPickup>(StorageOwner))
				{
					ContainerDef = Pickup->GetItemDef();
				}
			}
			
			// Open the storage window
			StorageWin->Open(StorageComp, ContainerDef);
			StorageWin->SetVisibility(ESlateVisibility::Visible);
			
			// Bind storage list left-click handler
			if (UStorageListWidget* StorageList = StorageWin->GetStorageList())
			{
				StorageList->OnItemLeftClicked.RemoveAll(InventoryScreen);
				StorageList->OnItemLeftClicked.AddDynamic(InventoryScreen, &UInventoryScreenWidget::HandleStorageItemLeftClick);
				UE_LOG(LogTemp, Display, TEXT("[PC] Storage window opened and StorageList bound"));
			}
			
			// Storage changed delegates are bound in StorageWindowWidget::Open()
		}
	}
	else
	{
		// Hide storage window if no storage
		if (StorageWindow)
		{
			StorageWindow->Close();
			StorageWindow->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

UStorageWindowWidget* AFirstPersonPlayerController::GetStorageWindow()
{
	// Lazy-create storage window widget
	if (!StorageWindow)
	{
		StorageWindow = CreateWidget<UStorageWindowWidget>(this, UStorageWindowWidget::StaticClass());
		if (StorageWindow)
		{
			StorageWindow->AddToViewport(901); // Higher z-order than inventory (900)
			// Position to the left of inventory screen (40% width, starting at 5%)
			StorageWindow->SetAnchorsInViewport(FAnchors(0.05f, 0.2f, 0.4f, 0.8f));
			StorageWindow->SetAlignmentInViewport(FVector2D(0.f, 0.f));
			StorageWindow->SetVisibility(ESlateVisibility::Collapsed); // Hidden by default
			UE_LOG(LogTemp, Display, TEXT("[PC] Storage window widget created and added to viewport"));
		}
	}
	return StorageWindow;
}

void AFirstPersonPlayerController::OnPickupPressed(const FInputActionValue& Value)
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

	// Reset state
	bInstantActionExecuted = false;
	bIsHoldingDrop = false;
	HoldDropTimer = 0.0f;

	// Start hold timer - will check what we're looking at in PlayerTick
	// If looking at pickup: hold it after progress bar fills
	// If looking at empty space and holding item: drop it after progress bar fills
	// Progress bar won't show until after tap threshold (0.25s) to avoid flicker on taps
	bIsHoldingDrop = true;
	// Don't show progress bar yet - wait for tap threshold to pass
}

void AFirstPersonPlayerController::OnPickupOngoing(const FInputActionValue& Value)
{
	// This is called every frame while R is held
	// Actual timer update happens in PlayerTick
}

void AFirstPersonPlayerController::OnPickupReleased(const FInputActionValue& Value)
{
	if (bInstantActionExecuted)
	{
		// Reset flag for next press
		bInstantActionExecuted = false;
		return;
	}

	AFirstPersonCharacter* C = Cast<AFirstPersonCharacter>(GetPawn());
	if (!C || bInventoryUIOpen)
	{
		// Cancel hold if no character or inventory open
		if (bIsHoldingDrop)
		{
			bIsHoldingDrop = false;
			HoldDropTimer = 0.0f;
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}
		}
		return;
	}

	// Check if this was a tap (very short press) vs a hold
	const float TapThreshold = 0.25f; // Consider it a tap if released within 0.25 seconds
	const bool bWasTap = (HoldDropTimer < TapThreshold);

	if (bIsHoldingDrop)
	{
		if (bWasTap)
		{
			// This was a tap - execute tap behavior
			// Hide progress bar first
			if (DropProgressBarWidget)
			{
				DropProgressBarWidget->SetVisible(false);
			}

			// Trace to see what player is looking at
			FVector CamLoc; FRotator CamRot;
			GetPlayerViewPoint(CamLoc, CamRot);
			const FVector Start = CamLoc;
			const FVector End = Start + CamRot.Vector() * 300.f;
			FHitResult Hit;
			FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemPickup), false);
			Params.AddIgnoredActor(C);
			
			if (GetWorld() && GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECollisionChannel::ECC_GameTraceChannel1, Params))
			{
				if (AItemPickup* Pickup = Cast<AItemPickup>(Hit.GetActor()))
				{
					// Tap on pickup: add to inventory (if it can be stored)
					UItemDefinition* Def = Pickup->GetItemDef();
					if (Def)
					{
						// Check if item can be stored
						if (!Def->bCanBeStored)
						{
							UE_LOG(LogTemp, Display, TEXT("[Pickup] Item %s cannot be stored (bCanBeStored is false)"), *Def->GetName());
							// Push message to message log
							if (UMessageLogSubsystem* MsgLog = GEngine ? GEngine->GetEngineSubsystem<UMessageLogSubsystem>() : nullptr)
							{
								MsgLog->PushMessage(FText::FromString(FString::Printf(TEXT("%s cannot be stored"), *Def->DisplayName.ToString())));
							}
						}
						else
						{
							UInventoryComponent* Inv = C->GetInventory();
							if (Inv)
							{
								FItemEntry Entry; Entry.Def = Def; Entry.ItemId = FGuid::NewGuid();
								if (Inv->TryAdd(Entry))
								{
									UE_LOG(LogTemp, Display, TEXT("[Pickup] Added %s to inventory"), *Def->GetName());
									Pickup->Destroy();
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
				}
			}
			else if (C->IsHoldingItem())
			{
				// Tap while holding item and not looking at anything: put item back in inventory
				C->PutHeldItemBack();
			}
		}
		else
		{
			// This was a hold that was canceled (released before timer completed)
			// Just cancel, don't do anything
		}

		// Reset state
		bIsHoldingDrop = false;
		HoldDropTimer = 0.0f;
		if (DropProgressBarWidget)
		{
			DropProgressBarWidget->SetVisible(false);
		}
	}
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
