#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "GameplayTagContainer.h"
#include "PhysicsObjectSocketComponent.generated.h"

class UBoxComponent;
class AItemPickup;
class UPrimitiveComponent;

// Delegates for socket events
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemSocketed, AItemPickup*, SocketedItem);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemReleased, AItemPickup*, ReleasedItem);

/**
 * Component that creates a socket area for physics objects (ItemPickup actors).
 * When a compatible item enters the trigger box, it smoothly snaps into place
 * and has physics disabled. Items can be released by the player pressing E.
 * The component's transform defines where items will be socketed.
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class UNKNOWN_API UPhysicsObjectSocketComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UPhysicsObjectSocketComponent(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// Collision box for socket trigger area (user must create and assign this)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Socket")
	TObjectPtr<UBoxComponent> SocketTriggerBox;

	// Tags that ItemDefinitions must have for items to be accepted by this socket
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Socket")
	FGameplayTagContainer AcceptedItemTags;

	// Speed at which items snap to socket position (units per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Socket", meta=(ClampMin="0.0"))
	float SnapSpeed = 500.0f;

	// Speed at which items rotate to socket orientation (degrees per second)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Socket", meta=(ClampMin="0.0"))
	float SnapRotationSpeed = 360.0f;

	// Delegates
	UPROPERTY(BlueprintAssignable, Category="Socket")
	FOnItemSocketed OnItemSocketed;

	UPROPERTY(BlueprintAssignable, Category="Socket")
	FOnItemReleased OnItemReleased;

	// Get the currently socketed item (if any)
	UFUNCTION(BlueprintPure, Category="Socket")
	AItemPickup* GetSocketedItem() const { return SocketedItem.Get(); }

	// Check if a specific item is currently socketed
	UFUNCTION(BlueprintPure, Category="Socket")
	bool IsItemSocketed(AItemPickup* Item) const;

	// Manually release the socketed item
	UFUNCTION(BlueprintCallable, Category="Socket")
	void ReleaseItem();


	// Static helper to find socket component that has a specific item socketed
	static UPhysicsObjectSocketComponent* FindSocketWithItem(AItemPickup* Item, UWorld* World);

protected:
	// Overlap event handlers
	UFUNCTION()
	void OnSocketBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	UFUNCTION()
	void OnSocketEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	// Currently socketed item
	TWeakObjectPtr<AItemPickup> SocketedItem;

	// Track the item that was manually released (to prevent reattachment until it leaves and re-enters)
	TWeakObjectPtr<AItemPickup> ManuallyReleasedItem;

	// Saved physics state for socketed item
	bool bSavedSimulatePhysics = false;
	bool bSavedGravityEnabled = false;
};

