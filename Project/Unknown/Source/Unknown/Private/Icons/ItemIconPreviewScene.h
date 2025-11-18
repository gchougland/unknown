#pragma once

#include "CoreMinimal.h"

class UWorld;
class AActor;
class USceneCaptureComponent2D;
class UTextureRenderTarget2D;
class UStaticMesh;
class UStaticMeshComponent;
class USkeletalMesh;
class USkeletalMeshComponent;

/**
 * Lightweight helper that owns a hidden capture actor inside a game world and can render meshes to a RT.
 * This avoids editor-only PreviewScene dependencies and works at runtime.
 */
class FItemIconPreviewScene
{
public:
    FItemIconPreviewScene();
    ~FItemIconPreviewScene();

    // Ensure the capture rig exists in the given world (spawns hidden actor/components once)
    bool EnsureInitialized(UWorld* World);

    // Ensure RT of given resolution exists and is bound to capture component
    bool EnsureRenderTarget(int32 Resolution, bool bTransparentBackground, const FLinearColor& SolidColor);

    // Configure camera based on mesh bounds, apply an optional item transform, then capture. Returns true on success.
    bool CaptureStaticMesh(UStaticMesh* Mesh,
                           const FTransform& ItemTransform,
                           int32 Resolution,
                           bool bTransparentBackground,
                           const FLinearColor& SolidColor);

    // After a successful capture, access the RT to readback
    UTextureRenderTarget2D* GetRenderTarget() const { return RenderTarget.Get(); }

    // Accessors for components (primarily for advanced use or future skeletal support)
    USceneCaptureComponent2D* GetCapture() const { return Capture.Get(); }

private:
    void ResetMeshComponents();
    void FitCameraToBounds(const FBoxSphereBounds& Bounds);

private:
    TWeakObjectPtr<UWorld> World;
    TWeakObjectPtr<AActor> CaptureActor;
    TWeakObjectPtr<USceneCaptureComponent2D> Capture;
    TWeakObjectPtr<UTextureRenderTarget2D> RenderTarget;
    TWeakObjectPtr<UStaticMeshComponent> StaticMeshComp;
    TWeakObjectPtr<USkeletalMeshComponent> SkeletalMeshComp;
    TWeakObjectPtr<class UDirectionalLightComponent> DirLight;
    TWeakObjectPtr<class USkyLightComponent> SkyLight;
};
