#include "Dimensions/PortalDevice.h"
#include "Dimensions/DimensionManagerSubsystem.h"
#include "Dimensions/DimensionScanningSubsystem.h"
#include "Dimensions/DimensionCartridgeHelpers.h"
#include "Dimensions/ReturnPortal.h"
#include "Inventory/ItemPickup.h"
#include "Inventory/ItemTypes.h"
#include "Components/SaveableActorComponent.h"
#include "Player/FirstPersonCharacter.h"
#include "Save/SaveSystemSubsystem.h"
#include "Save/GameSaveData.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "EngineUtils.h"

APortalDevice::APortalDevice(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// Create root scene component
	USceneComponent* RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = RootSceneComponent;

	// Create cartridge socket component
	// Note: SocketTriggerBox should be added manually in Blueprint
	CartridgeSocket = CreateDefaultSubobject<UPhysicsObjectSocketComponent>(TEXT("CartridgeSocket"));
	CartridgeSocket->SetupAttachment(RootComponent);

	// Create portal trigger box
	PortalTriggerBox = CreateDefaultSubobject<UBoxComponent>(TEXT("PortalTriggerBox"));
	PortalTriggerBox->SetupAttachment(RootComponent);
	PortalTriggerBox->SetBoxExtent(FVector(100.0f, 100.0f, 200.0f));
	PortalTriggerBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	PortalTriggerBox->SetCollisionObjectType(ECC_WorldDynamic);
	PortalTriggerBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	PortalTriggerBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	PortalTriggerBox->SetGenerateOverlapEvents(true);
}

void APortalDevice::BeginPlay()
{
	Super::BeginPlay();

	// Bind to socket events
	if (CartridgeSocket)
	{
		CartridgeSocket->OnItemSocketed.AddDynamic(this, &APortalDevice::OnCartridgeInserted);
		CartridgeSocket->OnItemReleased.AddDynamic(this, &APortalDevice::OnCartridgeRemoved);
	}

	// Bind to portal trigger
	if (PortalTriggerBox)
	{
		PortalTriggerBox->OnComponentBeginOverlap.AddDynamic(this, &APortalDevice::OnPortalBeginOverlap);
	}

	// Bind to dimension manager delegates
	if (UDimensionManagerSubsystem* DimensionManager = GetDimensionManager())
	{
		DimensionManager->OnDimensionLoaded.AddDynamic(this, &APortalDevice::OnDimensionLoaded);
		DimensionManager->OnDimensionUnloaded.AddDynamic(this, &APortalDevice::OnDimensionUnloaded);
	}
}

void APortalDevice::OnCartridgeInserted(AItemPickup* Cartridge)
{
	if (!Cartridge)
	{
		return;
	}

	// Guard against auto-loading during save restoration
	// Only defer if: 1) We're loading a save (temp save exists), 2) Dimension is NOT currently loaded, 3) Cartridge matches saved loaded dimension
	UWorld* World = GetWorld();
	if (World)
	{
		UGameInstance* GameInstance = World->GetGameInstance();
		if (GameInstance)
		{
			USaveSystemSubsystem* SaveSystem = GameInstance->GetSubsystem<USaveSystemSubsystem>();
			UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
			
			// Only defer if dimension is NOT currently loaded (if it's loaded, we're not in save restoration)
			if (SaveSystem && SaveSystem->GetCurrentSaveData() && DimensionManager && !DimensionManager->IsDimensionLoaded())
			{
				// Check if we're loading from a temp save (indicates save restoration in progress)
				FString TempSlotName = TEXT("_TEMP_RESTORE_POSITION_");
				if (UGameSaveData* TempSave = Cast<UGameSaveData>(UGameplayStatics::LoadGameFromSlot(TempSlotName, 0)))
				{
					// We're in the middle of save restoration - check if this cartridge matches saved loaded dimension
					UGameSaveData* SaveData = SaveSystem->GetCurrentSaveData();
					if (SaveData && SaveData->LoadedDimensionInstanceId.IsValid())
					{
						UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(Cartridge);
						if (CartridgeData)
						{
							// Find the saved dimension instance data
							const FDimensionInstanceSaveData* DimSaveData = nullptr;
							for (const FDimensionInstanceSaveData& DimData : SaveData->DimensionInstances)
							{
								if (DimData.InstanceId == SaveData->LoadedDimensionInstanceId && 
									DimData.CartridgeId == CartridgeData->GetCartridgeId())
								{
									DimSaveData = &DimData;
									break;
								}
							}

							if (DimSaveData)
							{
								// This is a saved dimension that should be restored - delay loading
								// RestoreLoadedDimension will handle it
								UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Cartridge matches saved loaded dimension during save restoration - deferring to RestoreLoadedDimension"));
								return;
							}
						}
					}
				}
			}
		}
	}

	// Guard against double-loading: if a dimension is already loaded with the same cartridge ID, don't load again
	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (DimensionManager && DimensionManager->IsDimensionLoaded())
	{
		// Check if the cartridge being inserted has the same instance ID as the currently loaded dimension
		UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(Cartridge);
		if (CartridgeData && CartridgeData->InstanceData && CartridgeData->InstanceData->InstanceId.IsValid())
		{
			FGuid LoadedInstanceId = DimensionManager->GetCurrentInstanceId();
			if (LoadedInstanceId == CartridgeData->InstanceData->InstanceId)
			{
				UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Dimension instance %s is already loaded - skipping double load"), 
					*LoadedInstanceId.ToString());
				return;
			}
		}
	}

	// Extract cartridge data from item
	UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(Cartridge);
	if (!CartridgeData || !CartridgeData->IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDevice] Cartridge does not contain valid dimension data"));
		return;
	}

	// Open portal with this cartridge
	OpenPortal(CartridgeData->GetCartridgeId());
}

void APortalDevice::OnCartridgeRemoved(AItemPickup* Cartridge)
{
	// Close portal when cartridge is removed
	ClosePortal();
}

void APortalDevice::OpenPortal(FGuid CartridgeId)
{
	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (!DimensionManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Cannot open portal: DimensionManager not found"));
		return;
	}

	// If a dimension is already loaded, unload it first (this will save it)
	if (DimensionManager->IsDimensionLoaded())
	{
		DimensionManager->UnloadDimensionInstance();
		CurrentInstanceId = FGuid();
	}

	// Get cartridge data from socketed item
	AItemPickup* SocketedItem = CartridgeSocket ? CartridgeSocket->GetSocketedItem() : nullptr;
	if (!SocketedItem)
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Cannot open portal: No cartridge in socket"));
		return;
	}

	UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(SocketedItem);
	if (!CartridgeData || !CartridgeData->IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Cannot open portal: Invalid cartridge data"));
		return;
	}

	// Get dimension definition
	UDimensionDefinition* DimensionDef = CartridgeData->GetDimensionDefinition();
	if (!DimensionDef)
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Cannot open portal: Dimension definition is null"));
		return;
	}

	// Get spawn position
	FVector SpawnPosition = GetSpawnPosition();

	// Get instance ID from cartridge data (if it exists, this is a restored dimension)
	FGuid InstanceIdToUse = FGuid();
	if (CartridgeData->InstanceData && CartridgeData->InstanceData->InstanceId.IsValid())
	{
		InstanceIdToUse = CartridgeData->InstanceData->InstanceId;
		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Restoring dimension instance %s from cartridge"), *InstanceIdToUse.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Creating new dimension instance (no existing instance ID in cartridge)"));
	}

	// Load dimension instance (with existing InstanceId if available)
	bool bSuccess = DimensionManager->LoadDimensionInstance(CartridgeId, DimensionDef, SpawnPosition, InstanceIdToUse);
	if (bSuccess)
	{
		CurrentCartridgeId = CartridgeId;
		CurrentInstanceId = DimensionManager->GetCurrentInstanceId();
		
		// Update cartridge instance data with the instance ID (in case it was newly generated)
		if (CartridgeData->InstanceData)
		{
			CartridgeData->InstanceData->InstanceId = CurrentInstanceId;
			CartridgeData->InstanceData->CartridgeId = CartridgeId;
			CartridgeData->InstanceData->WorldPosition = SpawnPosition;
			
			// Save updated cartridge data back to item
			FItemEntry ItemEntry = SocketedItem->GetItemEntry();
			UDimensionCartridgeHelpers::SetCartridgeDataInItemEntry(ItemEntry, CartridgeData);
			SocketedItem->SetItemEntry(ItemEntry);
		}
		
		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Opened portal to dimension instance %s"), 
			*CurrentInstanceId.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Failed to load dimension instance"));
	}
}

void APortalDevice::ClosePortal()
{
	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (!DimensionManager)
	{
		return;
	}

	// Unload dimension (this will save it automatically)
	if (DimensionManager->IsDimensionLoaded())
	{
		DimensionManager->UnloadDimensionInstance();
	}

	CurrentInstanceId = FGuid();
	CurrentCartridgeId = FGuid();

	UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Closed portal"));
}

FVector APortalDevice::GetSpawnPosition() const
{
	if (SpawnMarker)
	{
		return SpawnMarker->GetSpawnPosition();
	}

	// Default: spawn at portal device location with offset
	return GetActorLocation() + FVector(0.0f, 0.0f, -1000.0f);
}

bool APortalDevice::IsPortalOpen() const
{
	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	return DimensionManager && DimensionManager->IsDimensionLoaded();
}

UDimensionManagerSubsystem* APortalDevice::GetDimensionManager() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GameInstance = World->GetGameInstance();
	if (!GameInstance)
	{
		return nullptr;
	}

	return GameInstance->GetSubsystem<UDimensionManagerSubsystem>();
}

void APortalDevice::OnPortalBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// Check if portal is open
	if (!IsPortalOpen())
	{
		return;
	}

	// Check if it's the player
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	APlayerController* PlayerController = Cast<APlayerController>(Pawn->GetController());
	if (!PlayerController)
	{
		return;
	}

	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (!DimensionManager)
	{
		return;
	}

	// Get dimension level
	ULevel* DimensionLevel = DimensionManager->GetDimensionLevel(CurrentInstanceId);
	if (!DimensionLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDevice] Cannot teleport: Dimension level not found"));
		return;
	}

	// Find player start in dimension level
	FVector TeleportLocation;
	FRotator TeleportRotation = FRotator::ZeroRotator;

	// Search for PlayerStart in the dimension level
	APlayerStart* PlayerStart = nullptr;
	for (TActorIterator<APlayerStart> ActorIterator(GetWorld()); ActorIterator; ++ActorIterator)
	{
		APlayerStart* Start = *ActorIterator;
		if (Start && Start->GetLevel() == DimensionLevel)
		{
			PlayerStart = Start;
			break;
		}
	}

	if (PlayerStart)
	{
		TeleportLocation = PlayerStart->GetActorLocation();
		// Get rotation from PlayerStart - use relative transform if world transform not updated yet
		FTransform PlayerStartTransform = PlayerStart->GetActorTransform();
		if (PlayerStartTransform.GetLocation().IsNearlyZero())
		{
			// World transform not updated yet, use relative transform
			if (USceneComponent* RootComp = PlayerStart->GetRootComponent())
			{
				PlayerStartTransform = RootComp->GetRelativeTransform();
			}
		}
		TeleportRotation = PlayerStartTransform.GetRotation().Rotator();
	}
	else
	{
		// Fallback: use dimension spawn position with offset
		FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
		TeleportLocation = InstanceInfo.WorldPosition + FVector(0.0f, 0.0f, 100.0f);
		TeleportRotation = FRotator::ZeroRotator;
	}

	// Teleport player to dimension
	Pawn->SetActorLocation(TeleportLocation, false, nullptr, ETeleportType::TeleportPhysics);
	Pawn->SetActorRotation(TeleportRotation);
	PlayerController->SetControlRotation(TeleportRotation);

	// Mark player as being in the dimension
	DimensionManager->SetPlayerDimensionInstanceId(CurrentInstanceId);

	UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Teleported player to dimension at location %s"), *TeleportLocation.ToString());
}

void APortalDevice::OnDimensionLoaded(FGuid InstanceId)
{
	// Only spawn return portal if this is our dimension
	if (InstanceId == CurrentInstanceId)
	{
		// Check if return portal already exists (loaded from save)
		if (!ReturnPortal || !IsValid(ReturnPortal))
		{
			SpawnReturnPortal();
		}
		else
		{
			// Return portal exists (loaded from save) - ensure exit location is set
			ReturnPortal->ExitLocation = PortalExitLocation;
			UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Restored return portal exit location: %s"), *PortalExitLocation.ToString());
		}
	}
}

void APortalDevice::OnDimensionUnloaded(FGuid InstanceId)
{
	// Only handle if this is our dimension
	if (InstanceId == CurrentInstanceId)
	{
		// Update cartridge with current instance data before clearing
		UpdateCartridgeWithInstanceData(InstanceId);
		
		RemoveReturnPortal();
	}
}

void APortalDevice::SpawnReturnPortal()
{
	// Don't spawn if return portal already exists
	if (ReturnPortal && IsValid(ReturnPortal))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (!DimensionManager)
	{
		return;
	}

	// Get dimension level
	ULevel* DimensionLevel = DimensionManager->GetDimensionLevel(CurrentInstanceId);
	if (!DimensionLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDevice] Cannot spawn return portal: Dimension level not found"));
		return;
	}

	// Get dimension world position for coordinate conversion
	FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
	FVector DimensionWorldPos = InstanceInfo.WorldPosition;

	// Find player start in dimension level
	// Use the level's Actors array directly instead of TActorIterator to ensure actors are accessible
	FVector SpawnLocation;
	FRotator SpawnRotation = FRotator::ZeroRotator;

	APlayerStart* PlayerStart = nullptr;
	if (DimensionLevel)
	{
		// Iterate through actors in the dimension level directly
		for (AActor* Actor : DimensionLevel->Actors)
		{
			APlayerStart* Start = Cast<APlayerStart>(Actor);
			if (Start)
			{
				PlayerStart = Start;
				UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Found PlayerStart in dimension level: %s at %s"), 
					*Start->GetName(), *Start->GetActorLocation().ToString());
				break;
			}
		}
	}

	if (!PlayerStart)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PortalDevice] PlayerStart not found in dimension level, using fallback position"));
	}

	if (PlayerStart)
	{
		// Always use root component's relative transform (local to level)
		// World transforms may not be updated yet when level first loads
		FTransform PlayerStartRelativeTransform;
		if (USceneComponent* RootComp = PlayerStart->GetRootComponent())
		{
			PlayerStartRelativeTransform = RootComp->GetRelativeTransform();
		}
		else
		{
			// Fallback to actor transform if no root component
			PlayerStartRelativeTransform = PlayerStart->GetActorTransform();
		}
		
		// Convert relative transform to world space by adding dimension world position
		FVector TeleportLocation = PlayerStartRelativeTransform.GetLocation() + DimensionWorldPos;
		FRotator TeleportRotation = PlayerStartRelativeTransform.GetRotation().Rotator();
		
		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] PlayerStart relative: %s, world pos: %s, calculated world: %s"), 
			*PlayerStartRelativeTransform.GetLocation().ToString(), 
			*DimensionWorldPos.ToString(),
			*TeleportLocation.ToString());
		
		// Get forward vector from player start rotation
		FVector ForwardVector = FRotationMatrix(TeleportRotation).GetUnitAxis(EAxis::X);
		
		// Calculate spawn location in world space (behind player start)
		float BehindDistance = 200.0f;
		FVector SpawnLocationWorld = TeleportLocation - (ForwardVector * BehindDistance);
		
		// Convert to level-relative coordinates (OverrideLevel expects local coordinates)
		SpawnLocation = SpawnLocationWorld - DimensionWorldPos;
		
		// Face back toward player start
		SpawnRotation = TeleportRotation;
		SpawnRotation.Yaw += 180.0f;
	}
	else
	{
		// Fallback: use dimension spawn position with offset (same as player teleport)
		FVector TeleportLocation = InstanceInfo.WorldPosition + FVector(0.0f, 0.0f, 100.0f);
		
		// Calculate spawn location in world space
		FVector SpawnLocationWorld = TeleportLocation - FVector(200.0f, 0.0f, 0.0f);
		
		// Convert to level-relative coordinates
		SpawnLocation = SpawnLocationWorld - DimensionWorldPos;
		
		SpawnRotation = FRotator(0.0f, 180.0f, 0.0f); // Face back
	}

	// Spawn return portal using the configured class
	if (!ReturnPortalClass)
	{
		ReturnPortalClass = AReturnPortal::StaticClass();
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.OverrideLevel = DimensionLevel;

	AReturnPortal* NewReturnPortal = World->SpawnActor<AReturnPortal>(ReturnPortalClass, SpawnLocation, SpawnRotation, SpawnParams);
	if (NewReturnPortal)
	{
		NewReturnPortal->ExitLocation = PortalExitLocation;
		ReturnPortal = NewReturnPortal;

		// Tag return portal with dimension instance ID
		if (USaveableActorComponent* SaveableComp = NewReturnPortal->FindComponentByClass<USaveableActorComponent>())
		{
			SaveableComp->SetDimensionInstanceId(CurrentInstanceId);
		}
		else
		{
			SaveableComp = NewObject<USaveableActorComponent>(NewReturnPortal);
			SaveableComp->RegisterComponent();
			SaveableComp->SetDimensionInstanceId(CurrentInstanceId);
		}

		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Spawned return portal at location %s"), *SpawnLocation.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[PortalDevice] Failed to spawn return portal"));
	}
}

void APortalDevice::RemoveReturnPortal()
{
	if (ReturnPortal && IsValid(ReturnPortal))
	{
		ReturnPortal->Destroy();
		ReturnPortal = nullptr;
		UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Removed return portal"));
	}
}

void APortalDevice::UpdateCartridgeWithInstanceData(FGuid InstanceId)
{
	// Get the socketed cartridge item
	AItemPickup* SocketedItem = CartridgeSocket ? CartridgeSocket->GetSocketedItem() : nullptr;
	if (!SocketedItem)
	{
		return;
	}

	// Get cartridge data from item
	UDimensionCartridgeData* CartridgeData = UDimensionCartridgeHelpers::GetCartridgeDataFromItem(SocketedItem);
	if (!CartridgeData || !CartridgeData->InstanceData)
	{
		return;
	}

	// Update instance data with current dimension instance ID
	CartridgeData->InstanceData->InstanceId = InstanceId;
	CartridgeData->InstanceData->CartridgeId = CurrentCartridgeId;

	// Get current dimension instance info for world position
	UDimensionManagerSubsystem* DimensionManager = GetDimensionManager();
	if (DimensionManager)
	{
		FDimensionInstanceInfo InstanceInfo = DimensionManager->GetCurrentInstanceInfo();
		if (InstanceInfo.InstanceId == InstanceId)
		{
			CartridgeData->InstanceData->WorldPosition = InstanceInfo.WorldPosition;
		}
	}

	// Save updated cartridge data back to item
	FItemEntry ItemEntry = SocketedItem->GetItemEntry();
	UDimensionCartridgeHelpers::SetCartridgeDataInItemEntry(ItemEntry, CartridgeData);
	SocketedItem->SetItemEntry(ItemEntry);

	UE_LOG(LogTemp, Log, TEXT("[PortalDevice] Updated cartridge with instance ID %s and stability %f"), 
		*InstanceId.ToString(), CartridgeData->InstanceData->Stability);
}
