#include "Dimensions/DimensionSpawnMarker.h"
#include "Components/BillboardComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

ADimensionSpawnMarker::ADimensionSpawnMarker(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;
	SetActorHiddenInGame(true);

	// Create a scene component as root (required for placement)
	USceneComponent* SceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
	RootComponent = SceneComponent;

#if WITH_EDITORONLY_DATA
	// Create billboard component for visual representation in editor
	if (!IsRunningCommandlet())
	{
		UBillboardComponent* Billboard = CreateDefaultSubobject<UBillboardComponent>(TEXT("Billboard"));
		Billboard->SetupAttachment(RootComponent);
		
		// Try to set a sprite (will use default if not found)
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> SpriteTexture;
			FName ID_DimensionMarker;
			FText NAME_DimensionMarker;
			FConstructorStatics()
				: SpriteTexture(TEXT("/Engine/EditorResources/S_Note"))
				, ID_DimensionMarker(TEXT("DimensionSpawnMarker"))
				, NAME_DimensionMarker(NSLOCTEXT("SpriteCategory", "DimensionSpawnMarker", "Dimension Spawn Marker"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;
		
		if (Billboard)
		{
			Billboard->Sprite = ConstructorStatics.SpriteTexture.Get();
			Billboard->SetRelativeScale3D(FVector(0.5f, 0.5f, 0.5f));
			Billboard->bIsScreenSizeScaled = true;
		}
	}
#endif
}

FVector ADimensionSpawnMarker::GetSpawnPosition() const
{
	return GetActorLocation() + SpawnOffset;
}

#if WITH_EDITOR
void ADimensionSpawnMarker::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void ADimensionSpawnMarker::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}
#endif
