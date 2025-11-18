#include "Player/FirstPersonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Components/SpotLightComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"

AFirstPersonCharacter::AFirstPersonCharacter(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Create a simple first-person camera
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetRootComponent());
	FirstPersonCamera->bUsePawnControlRotation = true;
	FirstPersonCamera->SetRelativeLocation(FVector(0.f, 0.f, BaseEyeHeight));

	// Create a flashlight (spotlight) attached to the camera
	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	if (Flashlight)
	{
		Flashlight->SetupAttachment(FirstPersonCamera);
		Flashlight->SetRelativeLocation(FVector(10.f, 0.f, -5.f));
		Flashlight->Intensity = 200.f;
		Flashlight->AttenuationRadius = 2000.f;
		Flashlight->InnerConeAngle = 15.f;
		Flashlight->OuterConeAngle = 45.f;
		Flashlight->bUseInverseSquaredFalloff = false; // more controllable for a flashlight feel
		Flashlight->SetUseTemperature(true);
		Flashlight->Temperature = 4000.f;
		Flashlight->SetVisibility(false);
	}

 // Create physics interaction component
	PhysicsInteraction = CreateDefaultSubobject<UPhysicsInteractionComponent>(TEXT("PhysicsInteraction"));

	// Create a generic hold point so we can attach held items even without a mesh/socket
	HoldPoint = CreateDefaultSubobject<USceneComponent>(TEXT("HoldPoint"));
	if (HoldPoint)
	{
		// Attach to camera if available so it follows the view; else root
		USceneComponent* Parent = FirstPersonCamera ? static_cast<USceneComponent*>(FirstPersonCamera) : GetRootComponent();
		HoldPoint->SetupAttachment(Parent);
		// Default offset slightly in front of camera
		HoldPoint->SetRelativeLocation(FVector(30.f, 12.f, -10.f));
	}

	// Configure default movement speeds per acceptance criteria
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (Move)
	{
		Move->MaxWalkSpeed = WalkSpeed;            // 600 uu/s
		Move->MaxWalkSpeedCrouched = CrouchSpeed;  // 300 uu/s
		Move->GetNavAgentPropertiesRef().bCanCrouch = true;
	}

	// Create inventory component
	Inventory = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	// Create hotbar component
	Hotbar = CreateDefaultSubobject<UHotbarComponent>(TEXT("Hotbar"));

 bUseControllerRotationYaw = true; // typical for FPS
}

bool AFirstPersonCharacter::SelectHotbarSlot(int32 Index)
{
	if (!Hotbar || !Inventory)
	{
		return false;
	}
	const bool bSelected = Hotbar->SelectSlot(Index, Inventory);
	if (!bSelected)
	{
		return false;
	}
	// After selection, try to hold the active item for this slot
	const FGuid ActiveId = Hotbar->GetActiveItemId();
	if (ActiveId.IsValid())
	{
		for (const FItemEntry& E : Inventory->GetEntries())
		{
			if (E.ItemId == ActiveId)
			{
				HoldItem(E);
				return true;
			}
		}
		// Could not find the item by id anymore
		ReleaseHeldItem();
		return true;
	}
	// No active item in this slot: release any currently held
	ReleaseHeldItem();
	return true;
}

void AFirstPersonCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (FirstPersonCamera)
	{
		StandingCameraZ = FirstPersonCamera->GetRelativeLocation().Z;
	}
	// Bind to inventory events so we can keep held/active state in sync
	if (Inventory)
	{
		Inventory->OnItemRemoved.AddDynamic(this, &AFirstPersonCharacter::OnInventoryItemRemoved);
		Inventory->OnItemAdded.AddDynamic(this, &AFirstPersonCharacter::OnInventoryItemAdded);
	}
}

void AFirstPersonCharacter::StartSprint()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = SprintSpeed;
	}
}

void AFirstPersonCharacter::StopSprint()
{
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}
}

void AFirstPersonCharacter::ToggleCrouch()
{
	UCharacterMovementComponent* Move = GetCharacterMovement();
	if (!Move)
	{
		return;
	}

	// Flip internal toggle state
	bCrouchToggled = !bCrouchToggled;

	// Apply speeds
	if (bCrouchToggled)
	{
		Move->MaxWalkSpeed = CrouchSpeed;
	}
	else
	{
		Move->MaxWalkSpeed = WalkSpeed;
	}

	// If we have a world (PIE/runtime), use engine crouch to reduce capsule and let crouch callbacks adjust camera
	if (GetWorld())
	{
		if (bCrouchToggled)
		{
			Crouch();
		}
		else
		{
			UnCrouch();
		}
	}
}

void AFirstPersonCharacter::ToggleFlashlight()
{
	if (Flashlight)
	{
		Flashlight->SetVisibility(!Flashlight->IsVisible());
	}
}

void AFirstPersonCharacter::OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnStartCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (FirstPersonCamera)
	{
		// Compute crouched camera Z from the captured standing Z so we always return to the exact same height
		const float EyeDelta = BaseEyeHeight - CrouchedEyeHeight; // positive value (e.g., 64 -> 32 = 32)
		FVector Rel = FirstPersonCamera->GetRelativeLocation();
		Rel.Z = StandingCameraZ - EyeDelta;
		FirstPersonCamera->SetRelativeLocation(Rel);
	}
}

void AFirstPersonCharacter::OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust)
{
	Super::OnEndCrouch(HalfHeightAdjust, ScaledHalfHeightAdjust);
	if (FirstPersonCamera)
	{
		FVector Rel = FirstPersonCamera->GetRelativeLocation();
		Rel.Z = StandingCameraZ;
		FirstPersonCamera->SetRelativeLocation(Rel);
	}
}

bool AFirstPersonCharacter::HoldItem(const FItemEntry& ItemEntry)
{
    if (!ItemEntry.IsValid() || !ItemEntry.Def)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] HoldItem: Invalid item entry"));
        return false;
    }

    // Release any currently held item first
    ReleaseHeldItem();

    UWorld* World = GetWorld();
    if (!World)
    {
        // In some automation contexts there may be no active game world (headless unit tests).
        // Fall back to a logic-only bind so input/selection logic can still be validated.
        HeldItemEntry = ItemEntry;
        UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] HoldItem: World is null; binding entry only (no actor spawn)"));
        return true;
    }

 // Try to use the skeletal mesh/socket if available; otherwise we'll attach to HoldPoint
	USkeletalMeshComponent* CharacterMesh = GetMesh();
	const bool bHasMesh = (CharacterMesh != nullptr);
	const bool bSocketExists = (bHasMesh && CharacterMesh->DoesSocketExist(HandSocketName));
	if (!bSocketExists)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] HoldItem: Socket '%s' missing; will use HoldPoint"), *HandSocketName.ToString());
	}

	// Spawn the item pickup actor
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FTransform SpawnTransform = FTransform::Identity;
	if (bSocketExists)
	{
		SpawnTransform = CharacterMesh->GetSocketTransform(HandSocketName);
	}
	else
	{
		// Fallback: spawn at character location with slight offset
		SpawnTransform.SetLocation(GetActorLocation() + GetActorForwardVector() * 50.f);
	}

	HeldItemActor = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), SpawnTransform, SpawnParams);
	if (!HeldItemActor)
	{
		UE_LOG(LogTemp, Error, TEXT("[FirstPersonCharacter] HoldItem: Failed to spawn AItemPickup"));
		return false;
	}

	// Set the item definition
	HeldItemActor->SetItemDef(ItemEntry.Def);

	// Configure physics for held item
	UStaticMeshComponent* ItemMesh = HeldItemActor->Mesh;
	if (ItemMesh)
	{
		// Enable gravity while held (as per user requirement)
		ItemMesh->SetEnableGravity(true);
		// Disable physics simulation - we'll attach it instead
		ItemMesh->SetSimulatePhysics(false);
		// Set collision to query only (so it doesn't interfere with movement)
		ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		// Make sure it doesn't collide with the character
		ItemMesh->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		// Do not let the held item block the Interactable trace channel
		ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Ignore);
		// Also disable overlap generation to avoid spurious interactions
		ItemMesh->SetGenerateOverlapEvents(false);
	}

	// Attach to socket or HoldPoint/root component
	if (bSocketExists)
	{
		HeldItemActor->AttachToComponent(CharacterMesh, FAttachmentTransformRules::SnapToTargetNotIncludingScale, HandSocketName);
	}
	else if (HoldPoint)
	{
		HeldItemActor->AttachToComponent(HoldPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}
	else
	{
		// Fallback: attach to root component
		HeldItemActor->AttachToComponent(GetRootComponent(), FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	}

	// Apply per-item default hold transform relative to the attachment parent
	if (ItemEntry.Def)
	{
		HeldItemActor->SetActorRelativeTransform(ItemEntry.Def->DefaultHoldTransform);
	}

	// Store the item entry (retain exact ItemId binding)
	HeldItemEntry = ItemEntry;

    UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] HoldItem: Holding %s"), *ItemEntry.Def->DisplayName.ToString());
    return true;
}

void AFirstPersonCharacter::ReleaseHeldItem()
{
	if (HeldItemActor)
	{
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] ReleaseHeldItem: Releasing held item"));
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
	}
	HeldItemEntry = FItemEntry();
}

FItemEntry AFirstPersonCharacter::GetHeldItemEntry() const
{
	return HeldItemEntry;
}

void AFirstPersonCharacter::OnInventoryItemRemoved(const FGuid& RemovedItemId)
{
	if (!RemovedItemId.IsValid())
	{
		return;
	}
	if (HeldItemEntry.ItemId == RemovedItemId)
	{
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] OnInventoryItemRemoved: Currently held item was removed; releasing visual"));
		ReleaseHeldItem();
	}
	// Ensure hotbar active id remains valid if selection is on a type that still exists
	if (Hotbar && Inventory)
	{
		Hotbar->RefreshHeldFromInventory(Inventory);
	}
}

void AFirstPersonCharacter::OnInventoryItemAdded(const FItemEntry& AddedItem)
{
    // Keep this handler light and test-friendly; avoid doing heavy work here.
    UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] OnInventoryItemAdded: %s"), AddedItem.Def ? *AddedItem.Def->GetName() : TEXT("<null>"));

    // Requirement: When the player acquires a brand new item type (none of this type existed before),
    // auto-assign that item type to the first empty hotbar slot if available.
    if (!Inventory || !Hotbar)
    {
        return;
    }

    if (!AddedItem.Def)
    {
        return;
    }

    // Since this callback is fired after the item was added, CountByDef == 1 means this is the first of its type.
    const int32 NewCountForType = Inventory->CountByDef(AddedItem.Def);

    // If there is already a slot assigned for this item type (even if greyed out), do NOT create a new assignment.
    // This allows re-picking an item to reactivate its existing hotbar slot instead of occupying a new one.
    int32 ExistingAssignedIndex = INDEX_NONE;
    const int32 NumSlots = Hotbar->GetNumSlots();
    for (int32 Index = 0; Index < NumSlots; ++Index)
    {
        if (Hotbar->GetSlot(Index).AssignedType == AddedItem.Def)
        {
            ExistingAssignedIndex = Index;
            break;
        }
    }

    if (NewCountForType == 1)
    {
        // First instance of this type added to inventory.
        if (ExistingAssignedIndex == INDEX_NONE)
        {
            // Find the first empty hotbar slot (AssignedType == nullptr) and assign the new item type there.
            for (int32 Index = 0; Index < NumSlots; ++Index)
            {
                if (Hotbar->GetSlot(Index).AssignedType == nullptr)
                {
                    const bool bAssigned = Hotbar->AssignSlot(Index, AddedItem.Def);
                    if (bAssigned)
                    {
                        UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] Auto-assigned '%s' to hotbar slot %d"), *AddedItem.Def->GetName(), Index);
                    }
                    break; // Stop at the first attempted empty slot
                }
            }
        }
        else
        {
            // Slot already assigned for this type; nothing to reassign. We will just refresh the held id below.
            UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] Reusing existing hotbar slot %d for '%s'"), ExistingAssignedIndex, *AddedItem.Def->GetName());
        }
    }

    // Ensure the hotbar's active item id is consistent with the new inventory state.
    // This fixes a race where the UI might refresh before the slot has a valid ActiveItemId.
    Hotbar->RefreshHeldFromInventory(Inventory);
}
