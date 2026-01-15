#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PhysicsObjectSocketComponent.h"
#include "Components/BoxComponent.h"
#include "Dimensions/DimensionSpawnMarker.h"
#include "Dimensions/DimensionCartridgeData.h"
#include "Dimensions/ReturnPortal.h"
#include "PortalDevice.generated.h"

class AItemPickup;
class UDimensionManagerSubsystem;
class UDimensionScanningSubsystem;

/**
 * Portal device actor that accepts dimension cartridges and opens portals to dimensions.
 */
UCLASS(BlueprintType)
class UNKNOWN_API APortalDevice : public AActor
{
	GENERATED_BODY()

public:
	APortalDevice(const FObjectInitializer& ObjectInitializer);

	virtual void BeginPlay() override;

	// Socket component for cartridge insertion
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<UPhysicsObjectSocketComponent> CartridgeSocket;

	// Portal trigger box (teleports player to dimension)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<UBoxComponent> PortalTriggerBox;

	// Editor-assigned spawn marker (or null to use default location)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	TObjectPtr<ADimensionSpawnMarker> SpawnMarker;

	// Where the player exits the dimension (in main world)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	FVector PortalExitLocation = FVector::ZeroVector;

	// Class to use for return portal (can be overridden in Blueprint)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Portal")
	TSubclassOf<AReturnPortal> ReturnPortalClass = AReturnPortal::StaticClass();

	// Currently inserted cartridge ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	FGuid CurrentCartridgeId;

	// Active dimension instance ID
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	FGuid CurrentInstanceId;

	// Return portal spawned in the dimension (null if not spawned)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Portal")
	TObjectPtr<AReturnPortal> ReturnPortal;

	// Open portal to dimension using cartridge
	UFUNCTION(BlueprintCallable, Category="Portal")
	void OpenPortal(FGuid CartridgeId);

	// Close portal and unload dimension
	UFUNCTION(BlueprintCallable, Category="Portal")
	void ClosePortal();

	// Get spawn position from marker or default location
	UFUNCTION(BlueprintPure, Category="Portal")
	FVector GetSpawnPosition() const;

	// Check if portal is currently open
	UFUNCTION(BlueprintPure, Category="Portal")
	bool IsPortalOpen() const;

protected:
	// Handle cartridge insertion
	UFUNCTION()
	void OnCartridgeInserted(AItemPickup* Cartridge);

	// Handle cartridge removal
	UFUNCTION()
	void OnCartridgeRemoved(AItemPickup* Cartridge);

	// Handle portal trigger overlap
	UFUNCTION()
	void OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// Handle dimension loaded delegate
	UFUNCTION()
	void OnDimensionLoaded(FGuid InstanceId);

	// Handle dimension unloaded delegate
	UFUNCTION()
	void OnDimensionUnloaded(FGuid InstanceId);

	// Spawn return portal in dimension
	void SpawnReturnPortal();

	// Remove return portal
	void RemoveReturnPortal();

	// Updates the cartridge's instance data with the current dimension instance data
	void UpdateCartridgeWithInstanceData(FGuid InstanceId);

private:
	// Get dimension manager subsystem
	UDimensionManagerSubsystem* GetDimensionManager() const;
};
