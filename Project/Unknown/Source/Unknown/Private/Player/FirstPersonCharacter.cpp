#include "Player/FirstPersonCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/PhysicsInteractionComponent.h"
#include "Components/SpotLightComponent.h"
#include "Inventory/InventoryComponent.h"
#include "Inventory/HotbarComponent.h"
#include "Inventory/EquipmentComponent.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemTypes.h"
#include "Inventory/ItemDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Player/FirstPersonPlayerController.h"
#include "UI/InventoryScreenWidget.h"
#include "GameFramework/PlayerController.h"

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

 // Create equipment component and link to inventory in BeginPlay
 Equipment = CreateDefaultSubobject<UEquipmentComponent>(TEXT("Equipment"));

 bUseControllerRotationYaw = true; // typical for FPS
}

bool AFirstPersonCharacter::SelectHotbarSlot(int32 Index)
{
	if (!Hotbar || !Inventory)
	{
		return false;
	}
	
	// Store the currently held item entry before putting it back
	FItemEntry PreviouslyHeldEntry = HeldItemEntry;
	bool bWasHolding = IsHoldingItem();
	
	// If already holding an item, put it back first
	if (bWasHolding)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): Putting back currently held item"), Index);
		PutHeldItemBack();
	}
	
	// Select the slot - this will set the ActiveItemId for this slot
	const bool bSelected = Hotbar->SelectSlot(Index, Inventory);
	if (!bSelected)
	{
		return false;
	}
	
	// Get the ActiveItemId IMMEDIATELY after SelectSlot (before any refresh can change it)
	const FGuid ActiveId = Hotbar->GetActiveItemId();
	const UItemDefinition* ExpectedType = Hotbar->GetSlot(Index).AssignedType;
	UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): ActiveId=%s, ExpectedType=%s"), 
		Index, *ActiveId.ToString(EGuidFormats::DigitsWithHyphensInBraces), *GetNameSafe(ExpectedType));
	
	if (ActiveId.IsValid())
	{
		// Find the item by the ActiveId that was set by SelectSlot
		FItemEntry ItemToHold;
		bool bFound = false;
		for (const FItemEntry& E : Inventory->GetEntries())
		{
			if (E.ItemId == ActiveId)
			{
				ItemToHold = E;
				bFound = true;
				UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): Found item %s (ItemId: %s)"), 
					Index, *GetNameSafe(E.Def), *E.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
				
				// Verify the item matches the expected type
				if (ExpectedType && E.Def != ExpectedType)
				{
					UE_LOG(LogTemp, Error, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): TYPE MISMATCH! Expected %s, got %s"), 
						Index, *GetNameSafe(ExpectedType), *GetNameSafe(E.Def));
					// Don't hold if type mismatch
					return false;
				}
				break;
			}
		}
		
		if (bFound)
		{
			// Hold the item we found - log exactly what we're passing
			// Make a local copy to ensure we're not affected by any array modifications
			FItemEntry ItemToHoldCopy = ItemToHold;
			UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): About to call HoldItem with %s (ItemId: %s)"), 
				Index, *GetNameSafe(ItemToHoldCopy.Def), *ItemToHoldCopy.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			
			// Double-check the copy is correct before calling
			if (ItemToHoldCopy.Def != ExpectedType)
			{
				UE_LOG(LogTemp, Error, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): ItemToHoldCopy TYPE MISMATCH! Expected %s, got %s - ABORTING"), 
					Index, *GetNameSafe(ExpectedType), *GetNameSafe(ItemToHoldCopy.Def));
				return false;
			}
			
			HoldItem(ItemToHoldCopy);
			return true;
		}
		else
		{
			// Could not find the item by id anymore - this shouldn't happen if SelectSlot worked correctly
			UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): ActiveId %s not found in inventory"), 
				Index, *ActiveId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
			ReleaseHeldItem(false);
			return true;
		}
	}
	else
	{
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] SelectHotbarSlot(%d): No ActiveId (slot empty or no items of assigned type)"), Index);
	}
	// No active item in this slot: release any currently held
	if (bWasHolding)
	{
		ReleaseHeldItem(false);
	}
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
    // Link equipment to the inventory for capacity/effects handling
    if (Equipment && Inventory)
    {
        Equipment->Inventory = Inventory;
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
    UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] HoldItem: Called with %s (ItemId: %s)"), 
        *GetNameSafe(ItemEntry.Def), *ItemEntry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
    
    if (!ItemEntry.IsValid() || !ItemEntry.Def)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] HoldItem: Invalid item entry"));
        return false;
    }

    if (!Inventory)
    {
        UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] HoldItem: Inventory is null"));
        return false;
    }

    // Remove item from inventory BEFORE spawning pickup
    UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] HoldItem: Removing ItemId %s from inventory"), 
        *ItemEntry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
    if (!Inventory->RemoveById(ItemEntry.ItemId))
    {
        UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] HoldItem: Failed to remove item from inventory (ItemId: %s)"), 
            *ItemEntry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
        return false;
    }

    UWorld* World = GetWorld();
    if (!World)
    {
        // In some automation contexts there may be no active game world (headless unit tests).
        // Fall back to a logic-only bind so input/selection logic can still be validated.
        HeldItemEntry = ItemEntry;
        UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] HoldItem: World is null; binding entry only (no actor spawn)"));
        RefreshUIIfInventoryOpen();
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
    else if (HoldPoint)
    {
        SpawnTransform = HoldPoint->GetComponentTransform();
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
        // Rollback: try to add item back to inventory
        Inventory->TryAdd(ItemEntry);
        RefreshUIIfInventoryOpen();
        return false;
    }

    // Set the full item entry (includes metadata)
    HeldItemActor->SetItemEntry(ItemEntry);

    // Configure physics for held item: gravity OFF, interactable channel IGNORE
    UStaticMeshComponent* ItemMesh = HeldItemActor->Mesh;
    if (ItemMesh)
    {
        // Disable gravity while held
        ItemMesh->SetEnableGravity(false);
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

    // Refresh hotbar to update active item ID for the currently active slot
    if (Hotbar && Inventory)
    {
        Hotbar->RefreshHeldFromInventory(Inventory);
    }

    // Refresh UI if inventory screen is open
    RefreshUIIfInventoryOpen();

    UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] HoldItem: Holding %s (ItemId: %s)"), 
        *ItemEntry.Def->DisplayName.ToString(), *ItemEntry.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
    return true;
}

void AFirstPersonCharacter::ReleaseHeldItem(bool bTryPutBack)
{
	if (!HeldItemActor)
	{
		return;
	}
	
	if (bTryPutBack)
	{
		// Try to put the item back into inventory (or drop if full)
		if (!PutHeldItemBack())
		{
			// If PutHeldItemBack failed, just destroy the actor
			UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] ReleaseHeldItem: Failed to put item back, destroying"));
			HeldItemActor->Destroy();
			HeldItemActor = nullptr;
			HeldItemEntry = FItemEntry();
		}
	}
	else
	{
		// Just destroy the actor without trying to put it back (item was already removed from inventory)
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] ReleaseHeldItem: Releasing held item (already removed from inventory)"));
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
		HeldItemEntry = FItemEntry();
	}
}

bool AFirstPersonCharacter::PutHeldItemBack()
{
	if (!HeldItemActor || !HeldItemEntry.IsValid() || !Inventory)
	{
		return false;
	}

	// Get the item entry from the held actor (preserves metadata)
	FItemEntry EntryToReturn = HeldItemActor->GetItemEntry();
	if (!EntryToReturn.IsValid())
	{
		EntryToReturn = HeldItemEntry;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[FirstPersonCharacter] PutHeldItemBack: Returning %s (ItemId: %s)"), 
		*GetNameSafe(EntryToReturn.Def), *EntryToReturn.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));

	// Try to add back to inventory
	if (Inventory->TryAdd(EntryToReturn))
	{
		// Successfully added back - destroy the pickup
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] PutHeldItemBack: Successfully returned %s (ItemId: %s) to inventory"), 
			*GetNameSafe(EntryToReturn.Def), *EntryToReturn.ItemId.ToString(EGuidFormats::DigitsWithHyphensInBraces));
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
		HeldItemEntry = FItemEntry();
		
		// Don't refresh hotbar here - let SelectHotbarSlot handle it after selecting the new slot
		// Refreshing here could change ActiveItemId before SelectSlot completes
		
		// Refresh UI
		RefreshUIIfInventoryOpen();
		return true;
	}
	else
	{
		// Inventory is full - drop the item
		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] PutHeldItemBack: Inventory full, dropping %s"), 
			*GetNameSafe(EntryToReturn.Def));
		
		UWorld* World = GetWorld();
		if (!World)
		{
			UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] PutHeldItemBack: World is null, cannot drop"));
			return false;
		}

		FActorSpawnParameters Params;
		Params.Owner = this;
		Params.Instigator = this;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

		// Use unified drop helper
		AItemPickup* DroppedPickup = AItemPickup::DropItemToWorld(World, this, EntryToReturn, Params);
		if (!DroppedPickup)
		{
			UE_LOG(LogTemp, Error, TEXT("[FirstPersonCharacter] PutHeldItemBack: Failed to drop item, keeping it held"));
			return false;
		}

		// Clear held item state
		HeldItemActor->Destroy();
		HeldItemActor = nullptr;
		HeldItemEntry = FItemEntry();
		
		// Don't refresh hotbar here - let SelectHotbarSlot handle it after selecting the new slot
		
		// Refresh UI
		RefreshUIIfInventoryOpen();
		return true;
	}
}

void AFirstPersonCharacter::DropHeldItemAtLocation()
{
	if (!HeldItemActor || !HeldItemEntry.IsValid() || !HeldItemEntry.Def)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] DropHeldItemAtLocation: No held item to drop"));
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FirstPersonCharacter] DropHeldItemAtLocation: World is null"));
		return;
	}

	// Store entry before clearing state
	FItemEntry EntryToDrop = HeldItemActor->GetItemEntry();
	if (!EntryToDrop.IsValid())
	{
		EntryToDrop = HeldItemEntry;
	}

	// Get camera view point
	FVector CamLoc;
	FRotator CamRot;
	if (AController* PC = GetController())
	{
		PC->GetPlayerViewPoint(CamLoc, CamRot);
	}
	else
	{
		CamLoc = GetActorLocation();
		CamRot = GetActorRotation();
	}

	const FVector Forward = CamRot.Vector();
	const float MaxTraceDistance = 600.f;
	const FVector TraceEnd = CamLoc + Forward * MaxTraceDistance;

	// Perform trace to find drop location
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(ItemDrop), false);
	Params.AddIgnoredActor(this);
	if (HeldItemActor)
	{
		Params.AddIgnoredActor(HeldItemActor);
	}

	FVector DropLocation;
	FRotator DropRotation = FRotator(0.f, CamRot.Yaw, 0.f);
	bool bHit = false;

	if (World->LineTraceSingleByChannel(Hit, CamLoc, TraceEnd, ECC_Visibility, Params))
	{
		bHit = true;
		const float HitDistance = (Hit.ImpactPoint - CamLoc).Size();
		
		// If hit is far away, offset back to prevent clipping
		if (HitDistance > 400.f)
		{
			const float OffsetDistance = 30.f; // ~1 foot in cm
			DropLocation = Hit.ImpactPoint - Forward * OffsetDistance;
		}
		else
		{
			DropLocation = Hit.ImpactPoint;
		}
	}
	else
	{
		// No hit - drop at max distance
		DropLocation = TraceEnd;
	}

	// Create base transform at drop location
	const FTransform BaseTransform(DropRotation, DropLocation, FVector(1.f, 1.f, 1.f));
	
	// Apply DefaultDropTransform on top
	const FTransform FinalTransform = HeldItemEntry.Def->DefaultDropTransform * BaseTransform;

	// Spawn the pickup
	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = this;
	SpawnParams.Instigator = this;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	AItemPickup* DroppedPickup = World->SpawnActor<AItemPickup>(AItemPickup::StaticClass(), FinalTransform, SpawnParams);
	if (DroppedPickup)
	{
		// Set the full item entry (includes metadata)
		DroppedPickup->SetItemEntry(EntryToDrop);

		// Configure physics for dropped items: gravity ON, interactable channel BLOCK
		if (UStaticMeshComponent* ItemMesh = DroppedPickup->Mesh)
		{
			ItemMesh->SetSimulatePhysics(true);
			ItemMesh->SetEnableGravity(true);
			ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
			ItemMesh->SetCollisionObjectType(ECC_WorldDynamic);
			ItemMesh->SetCollisionResponseToAllChannels(ECR_Block);
			ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
		}

		UE_LOG(LogTemp, Display, TEXT("[FirstPersonCharacter] DropHeldItemAtLocation: Dropped %s at %s"), 
			*GetNameSafe(EntryToDrop.Def), *FinalTransform.GetLocation().ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[FirstPersonCharacter] DropHeldItemAtLocation: Failed to spawn pickup"));
		return;
	}

	// Clear held item state BEFORE destroying actor
	// Store the entry temporarily since we need it for the check below
	FItemEntry DroppedEntry = EntryToDrop;
	HeldItemEntry = FItemEntry();
	HeldItemActor->Destroy();
	HeldItemActor = nullptr;

	// IMPORTANT: The item was already removed from inventory when we held it,
	// so we should NOT try to put it back. The drop is complete.
	// Refresh UI
	RefreshUIIfInventoryOpen();
}

void AFirstPersonCharacter::RefreshUIIfInventoryOpen()
{
	if (AController* PC = GetController())
	{
		if (AFirstPersonPlayerController* FPPC = Cast<AFirstPersonPlayerController>(PC))
		{
			if (UInventoryScreenWidget* InventoryScreen = FPPC->GetInventoryScreen())
			{
				if (InventoryScreen->IsOpen())
				{
					InventoryScreen->RefreshInventoryView();
				}
			}
		}
	}
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
		// Don't try to put it back - it was already removed from inventory
		ReleaseHeldItem(false);
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
