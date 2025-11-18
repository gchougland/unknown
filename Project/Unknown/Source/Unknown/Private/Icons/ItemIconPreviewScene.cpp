#include "Icons/ItemIconPreviewScene.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/Texture.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetMathLibrary.h"

FItemIconPreviewScene::FItemIconPreviewScene()
{
}

FItemIconPreviewScene::~FItemIconPreviewScene()
{
    // Components/actor will be GC'd as they are UObjects owned by the world
    RenderTarget = nullptr;
    Capture = nullptr;
    StaticMeshComp = nullptr;
    SkeletalMeshComp = nullptr;
    CaptureActor = nullptr;
    World = nullptr;
}

bool FItemIconPreviewScene::EnsureInitialized(UWorld* InWorld)
{
    if (!InWorld)
    {
        return false;
    }
    World = InWorld;
    if (!CaptureActor.IsValid())
    {
        FActorSpawnParameters Params;
        Params.ObjectFlags |= RF_Transient;
        Params.bDeferConstruction = false;
        Params.Name = MakeUniqueObjectName(InWorld, AActor::StaticClass(), TEXT("ItemIconCaptureActor"));
        AActor* Actor = InWorld->SpawnActor<AActor>(Params);
        if (!Actor)
        {
            return false;
        }
        // Do not hide the actor; hidden actors may not render in a SceneCapture even with ShowOnly.
        Actor->SetActorHiddenInGame(false);
        // Root
        USceneComponent* Root = NewObject<USceneComponent>(Actor, TEXT("Root"));
        Root->SetMobility(EComponentMobility::Movable);
        Root->RegisterComponent();
        Actor->SetRootComponent(Root);

        // Static mesh host
        UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Actor, TEXT("IconMesh"));
        MeshComp->SetMobility(EComponentMobility::Movable);
        MeshComp->SetCastShadow(false);
        MeshComp->SetVisibility(true, true);
        MeshComp->SetupAttachment(Root);
        MeshComp->RegisterComponent();
        StaticMeshComp = MeshComp;

        // Capture component
        USceneCaptureComponent2D* Cap = NewObject<USceneCaptureComponent2D>(Actor, TEXT("SceneCapture2D"));
        Cap->SetupAttachment(Root);
        Cap->bCaptureEveryFrame = false;
        Cap->bCaptureOnMovement = false;
        // Capture base color to avoid lighting artifacts; combined with Unlit show flag ensures albedo fidelity
        Cap->CaptureSource = ESceneCaptureSource::SCS_BaseColor;
        Cap->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
        Cap->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;
        Cap->FOVAngle = 40.0f;
        // Stabilize exposure
        Cap->PostProcessSettings.bOverride_AutoExposureMethod = true;
        Cap->PostProcessSettings.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
        Cap->PostProcessSettings.bOverride_AutoExposureBias = true;
        Cap->PostProcessSettings.AutoExposureBias = 0.0f;
        // Avoid near-plane clipping on tiny assets
        Cap->bOverride_CustomNearClippingPlane = true;
        Cap->CustomNearClippingPlane = 1.0f;
        // Show flags: switch to Unlit view to capture base color reliably (prevents white, overbright results)
        Cap->ShowFlags.Lighting = 0;
        Cap->ShowFlags.SkyLighting = 0;
        Cap->ShowFlags.Tonemapper = 1;  // enable tonemapping for proper sRGB output
        Cap->ShowFlags.Atmosphere = 0;
        Cap->ShowFlags.VolumetricFog = 0;
        Cap->ShowFlags.Fog = 0;
        Cap->ShowFlags.Bloom = 0;
        Cap->ShowFlags.MotionBlur = 0;
        // Also turn bloom off explicitly in PP to be safe
        Cap->PostProcessSettings.bOverride_BloomIntensity = true;
        Cap->PostProcessSettings.BloomIntensity = 0.0f;
        // Neutral exposure
        Cap->PostProcessSettings.bOverride_AutoExposureBias = true;
        Cap->PostProcessSettings.AutoExposureBias = 0.0f;
        Cap->RegisterComponent();
        Capture = Cap;

        // Add neutral lighting (soft key + ambient)
        UDirectionalLightComponent* Dir = NewObject<UDirectionalLightComponent>(Actor, TEXT("IconDirLight"));
        Dir->SetupAttachment(Root);
        Dir->Intensity = 1500.0f; // conservative key intensity
        Dir->SetWorldRotation(FRotator(-35.f, 35.f, 0.f));
        Dir->CastShadows = false;
        Dir->RegisterComponent();
        DirLight = Dir;

        USkyLightComponent* Sky = NewObject<USkyLightComponent>(Actor, TEXT("IconSkyLight"));
        Sky->SetupAttachment(Root);
        Sky->Intensity = 0.1f;
        Sky->Mobility = EComponentMobility::Movable;
        Sky->bCaptureEmissiveOnly = false;
        Sky->RegisterComponent();
        SkyLight = Sky;

        CaptureActor = Actor;
    }
    return CaptureActor.IsValid() && Capture.IsValid() && StaticMeshComp.IsValid();
}

bool FItemIconPreviewScene::EnsureRenderTarget(int32 Resolution, bool bTransparentBackground, const FLinearColor& SolidColor)
{
    if (!Capture.IsValid())
    {
        return false;
    }
    if (!RenderTarget.IsValid() || RenderTarget->SizeX != Resolution || RenderTarget->SizeY != Resolution)
    {
        UTextureRenderTarget2D* RT = NewObject<UTextureRenderTarget2D>(Capture.Get());
        RT->RenderTargetFormat = RTF_RGBA8;
        RT->bAutoGenerateMips = false;
        RT->ClearColor = bTransparentBackground ? FLinearColor(0,0,0,0) : SolidColor;
        RT->InitCustomFormat(Resolution, Resolution, PF_B8G8R8A8, /*bForceLinearGamma*/ false);
        RT->UpdateResourceImmediate(true);
        RenderTarget = RT;
        Capture->TextureTarget = RT;
    }
    else
    {
        RenderTarget->ClearColor = bTransparentBackground ? FLinearColor(0,0,0,0) : SolidColor;
    }
    return true;
}

void FItemIconPreviewScene::ResetMeshComponents()
{
    if (StaticMeshComp.IsValid())
    {
        StaticMeshComp->SetStaticMesh(nullptr);
    }
}

void FItemIconPreviewScene::FitCameraToBounds(const FBoxSphereBounds& Bounds)
{
    if (!Capture.IsValid())
    {
        return;
    }

    const float FOVDegrees = Capture->FOVAngle;
    const float Radius = FMath::Max(15.0f, Bounds.SphereRadius); // clamp to avoid extreme close-ups
    const float FOVRadians = FMath::DegreesToRadians(FOVDegrees);
    const float Distance = Radius / FMath::Tan(FOVRadians * 0.5f) * 1.2f; // framing factor

    const FVector Target = Bounds.Origin;
    // 3/4 angle
    const FRotator Rot = FRotator(-20.f, 35.f, 0.f);
    const FVector Forward = Rot.Vector();
    const FVector CamLoc = Target - Forward * Distance;
    Capture->SetWorldLocation(CamLoc);
    Capture->SetWorldRotation(UKismetMathLibrary::FindLookAtRotation(CamLoc, Target));
    UE_LOG(LogTemp, Display, TEXT("[ItemIcons] FitCamera: FOV=%.1f Radius=%.2f Dist=%.2f Cam=%s Target=%s"),
        FOVDegrees, Radius, Distance, *CamLoc.ToString(), *Target.ToString());
}

bool FItemIconPreviewScene::CaptureStaticMesh(UStaticMesh* Mesh,
                                             const FTransform& ItemTransform,
                                             int32 Resolution,
                                             bool bTransparentBackground,
                                             const FLinearColor& SolidColor)
{
    if (!World.IsValid() || !EnsureInitialized(World.Get()) || !Mesh)
    {
        return false;
    }
    if (!EnsureRenderTarget(Resolution, bTransparentBackground, SolidColor))
    {
        return false;
    }

    ResetMeshComponents();
    StaticMeshComp->SetStaticMesh(Mesh);
    StaticMeshComp->SetRelativeTransform(ItemTransform);
    StaticMeshComp->SetVisibility(true, true);
    StaticMeshComp->SetHiddenInGame(false);
    // Improve material/texture readiness for capture
#if WITH_EDITORONLY_DATA
    StaticMeshComp->bForceMipStreaming = true;
#endif
    StaticMeshComp->SetForcedLodModel(0);
    StaticMeshComp->UpdateComponentToWorld();
    StaticMeshComp->MarkRenderStateDirty();
    const FBoxSphereBounds Bounds = StaticMeshComp->CalcBounds(StaticMeshComp->GetComponentTransform());
    const int32 NumMats = StaticMeshComp->GetNumMaterials();
    UE_LOG(LogTemp, Display, TEXT("[ItemIcons] CaptureStaticMesh: Mesh=%s Mats=%d BoundsOrigin=%s Extent=%s Radius=%.2f"),
        *Mesh->GetName(), NumMats, *Bounds.Origin.ToString(), *Bounds.BoxExtent.ToString(), Bounds.SphereRadius);

    // Attempt to force material textures resident to avoid white fallback
    for (int32 MatIdx = 0; MatIdx < NumMats; ++MatIdx)
    {
        if (UMaterialInterface* MI = StaticMeshComp->GetMaterial(MatIdx))
        {
            TArray<UTexture*> UsedTextures;
            MI->GetUsedTextures(UsedTextures, EMaterialQualityLevel::High, true, ERHIFeatureLevel::SM5, true);
            for (UTexture* Tex : UsedTextures)
            {
                if (UTexture2D* Tex2D = Cast<UTexture2D>(Tex))
                {
                    Tex2D->SetForceMipLevelsToBeResident(30.0f);
                    Tex2D->WaitForStreaming();
                }
            }
        }
    }
    FitCameraToBounds(Bounds);

    if (Capture.IsValid())
    {
        // Show only the item mesh to avoid any world/background leaks
        Capture->ClearShowOnlyComponents();
        Capture->ShowOnlyComponent(StaticMeshComp.Get());
        Capture->CaptureScene();
        return true;
    }
    return false;
}
