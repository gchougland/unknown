// Basic First Person Character for TDD
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Inventory/ItemTypes.h"

class UCameraComponent;
class UPhysicsInteractionComponent;
class USpotLightComponent;
class UInventoryComponent;
class UHotbarComponent;
class USceneComponent;
class AItemPickup;
class UEquipmentComponent;

#include "FirstPersonCharacter.generated.h"

UCLASS()
class UNKNOWN_API AFirstPersonCharacter : public ACharacter
{
	GENERATED_BODY()
public:
	AFirstPersonCharacter(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// Sprint speed accessor for tests
	UFUNCTION(BlueprintPure, Category="Movement")
	float GetSprintSpeed() const { return SprintSpeed; }

	// TDD: declare sprint API (stubbed initially)
	UFUNCTION(BlueprintCallable, Category="Movement")
	void StartSprint();

	UFUNCTION(BlueprintCallable, Category="Movement")
	void StopSprint();

	// Toggle crouch behavior (hold-to-toggle style per acceptance criteria)
	UFUNCTION(BlueprintCallable, Category="Movement")
	void ToggleCrouch();

	// Toggle the flashlight (F key)
	UFUNCTION(BlueprintCallable, Category="Equipment")
	void ToggleFlashlight();

protected:
	virtual void BeginPlay() override;

	// Scene component used as a programmable hold point when no mesh/socket is available
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USceneComponent> HoldPoint;

	// Desired configured speeds
	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float WalkSpeed = 600.f;

	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float CrouchSpeed = 300.f;

	UPROPERTY(EditDefaultsOnly, Category="Movement")
	float SprintSpeed = 900.f;

	// Camera used for first-person view
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UCameraComponent> FirstPersonCamera;

	// Flashlight spotlight attached to the camera
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Equipment", meta=(AllowPrivateAccess="true"))
	TObjectPtr<USpotLightComponent> Flashlight;

	// Interaction component used for physics interactions
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Interaction", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UPhysicsInteractionComponent> PhysicsInteraction;

 // Inventory component (to be used by gameplay & UI)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UInventoryComponent> Inventory;

	// Hotbar component (1-9 selection and assignment)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
	TObjectPtr<UHotbarComponent> Hotbar;

	// Remember standing camera Z captured at runtime so uncrouch returns exactly
	UPROPERTY(Transient)
	float StandingCameraZ = 0.f;

	// Internal state used for toggle crouch in unit-test contexts where Crouch/UnCrouch may be no-op
	UPROPERTY(Transient)
	bool bCrouchToggled = false;

	// ACharacter hooks to keep camera height consistent with crouch state
	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;
	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:
	UFUNCTION(BlueprintPure, Category="Inventory")
	UInventoryComponent* GetInventory() const { return Inventory; }

 UFUNCTION(BlueprintPure, Category="Inventory")
 UHotbarComponent* GetHotbar() const { return Hotbar; }

 UFUNCTION(BlueprintPure, Category="Inventory")
 UEquipmentComponent* GetEquipment() const { return Equipment; }

	// Selects a hotbar slot [0..8] and updates held item from inventory
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool SelectHotbarSlot(int32 Index);

	// Hold an item from inventory (spawns actor and attaches to hand socket)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool HoldItem(const FItemEntry& ItemEntry);

	// Release the currently held item (removes from world)
	// If bTryPutBack is true, attempts to put item back in inventory or drop it
	// If false, just destroys the held actor (for cases where item was already removed from inventory)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	void ReleaseHeldItem(bool bTryPutBack = true);
	
	// Put the currently held item back into inventory (or drop if full)
	UFUNCTION(BlueprintCallable, Category="Inventory")
	bool PutHeldItemBack();

	// Check if an item is currently being held
	UFUNCTION(BlueprintPure, Category="Inventory")
	bool IsHoldingItem() const { return HeldItemActor != nullptr; }

	// Get the currently held item entry (if any)
	UFUNCTION(BlueprintPure, Category="Inventory")
	FItemEntry GetHeldItemEntry() const;

protected:
 // Currently held item actor (spawned and attached to socket)
 UPROPERTY(Transient)
 TObjectPtr<AItemPickup> HeldItemActor;

	// Currently held item entry data
	UPROPERTY(Transient)
	FItemEntry HeldItemEntry;

	// Socket name for attaching held items (default: Hand_R_Socket)
	UPROPERTY(EditDefaultsOnly, Category="Inventory")
	FName HandSocketName = TEXT("Hand_R_Socket");

	// Inventory removal handler to auto-release held item if its entry is removed
	UFUNCTION()
	void OnInventoryItemRemoved(const FGuid& RemovedItemId);

	// Inventory added handler to potentially populate active hotbar slot and hold the item
    UFUNCTION()
    void OnInventoryItemAdded(const FItemEntry& AddedItem);

    // Equipment component (manages equipping and effects)
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Inventory", meta=(AllowPrivateAccess="true"))
    TObjectPtr<UEquipmentComponent> Equipment;
    
private:
    // Helper to refresh UI if inventory screen is open
    void RefreshUIIfInventoryOpen();
};
